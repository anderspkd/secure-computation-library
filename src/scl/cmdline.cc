/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "scl/util/cmdline.h"

#include <vector>

using namespace scl;

bool util::ProgramOptions::Parser::isArg(std::string_view name) const {
  return std::any_of(m_args.begin(), m_args.end(), [&](auto a) {
    return a.name == name;
  });
}

bool util::ProgramOptions::Parser::isFlag(std::string_view name) const {
  return std::any_of(m_flags.begin(), m_flags.end(), [&](auto f) {
    return f.name == name;
  });
}

namespace {

bool name(std::string_view opt_name, std::string_view& name) {
  if (opt_name[0] != '-') {
    return false;
  }
  name = opt_name.substr(1, opt_name.size());
  return true;
}

template <typename ARG_OR_FLAG>
bool hasDuplicates(const std::vector<ARG_OR_FLAG>& opts) {
  for (std::size_t i = 0; i < opts.size(); i++) {
    const std::string_view n = opts[i].name;
    for (std::size_t j = i + 1; j < opts.size(); j++) {
      if (n == opts[j].name) {
        return true;
      }
    }
  }

  return false;
}

}  // namespace

using ParseRet = std::variant<util::ProgramOptions, std::string_view>;

ParseRet util::ProgramOptions::Parser::parseArguments(int argc, char** argv) {
  if (hasDuplicates(m_args)) {
    return "duplicate argument definition";
  }

  if (hasDuplicates(m_flags)) {
    return "duplicate flag definition";
  }

  m_program_name = argv[0];
  std::vector<std::string_view> cmd_args(argv + 1, argv + argc);

  const auto help_needed = std::any_of(cmd_args.begin(),
                                       cmd_args.end(),
                                       [](auto e) { return e == "-help"; });
  if (help_needed) {
    return "";
  }

  std::unordered_map<std::string_view, std::string_view> args;
  std::for_each(m_args.begin(), m_args.end(), [&args](const auto arg) {
    if (arg.default_value.has_value()) {
      args[arg.name] = arg.default_value.value();
    }
  });

  std::unordered_map<std::string_view, bool> flags;
  std::size_t i = 0;
  while (i < cmd_args.size()) {
    std::string_view arg_name;
    if (!name(cmd_args[i++], arg_name)) {
      return "argument must begin with '-'";
    }

    if (isArg(arg_name)) {
      if (i == cmd_args.size()) {
        return "invalid argument";
      }
      args[arg_name] = cmd_args[i++];
    } else if (isFlag(arg_name)) {
      flags[arg_name] = true;
    } else {
      return "encountered unknown argument";
    }
  }

  // check if we got everything
  std::string_view error_msg;
  forEachRequired(m_args, [&](const auto arg) {
    if (args.find(arg.name) == args.end()) {
      error_msg = "missing required argument";
    }
  });

  if (error_msg.empty()) {
    return ProgramOptions(args, flags);
  }
  return error_msg;
}

void util::ProgramOptions::Parser::argListShort(
    std::ostream& stream,
    std::string_view program_name) const {
  stream << "Usage: " << program_name << " ";
  forEachRequired(m_args, [&stream](const auto arg) {
    stream << "-" << arg.name << " " << arg.type_hint << " ";
  });

  stream << "[options ...]" << std::endl;
}

std::string getPadding(std::size_t lead) {
  const static std::size_t padding = 20;
  const static std::size_t min_padding = 5;
  const auto psz = lead >= padding + min_padding ? min_padding : padding - lead;
  return std::string(psz, ' ');
}

void writeArg(std::ostream& stream, const util::ProgramArg& arg) {
  stream << " -" << arg.name << " '" << arg.type_hint << "'";
  if (!arg.description.empty()) {
    const auto pad_str = getPadding(arg.name.size() + arg.type_hint.size() + 5);
    stream << pad_str << arg.description << ".";
  }
  if (arg.default_value.has_value()) {
    stream << " [default=" << arg.default_value.value() << "]";
  }
  stream << std::endl;
}

void writeFlag(std::ostream& stream, const util::ProgramFlag& flag) {
  stream << " -" << flag.name;
  if (!flag.description.empty()) {
    const auto pad_str = getPadding(flag.name.size() + 2);
    stream << pad_str << flag.description << ".";
  }
  stream << std::endl;
}

template <typename IT>
bool hasRequired(IT begin, IT end) {
  return std::any_of(begin, end, [](const auto a) { return a.is_required; });
}

template <typename IT>
bool hasOptional(IT begin, IT end) {
  return std::any_of(begin, end, [](const auto a) { return !a.is_required; });
}

void util::ProgramOptions::Parser::argListLong(std::ostream& stream) const {
  if (!m_description.empty()) {
    stream << std::endl << m_description << std::endl;
  }
  stream << std::endl;

  const auto has_req_arg = hasRequired(m_args.begin(), m_args.end());

  if (has_req_arg) {
    stream << "Required arguments" << std::endl;
    forEachRequired(m_args, [&stream](const auto a) { writeArg(stream, a); });
    stream << std::endl;
  }

  if (hasOptional(m_args.begin(), m_args.end())) {
    stream << "Optional arguments" << std::endl;

    forEachOptional(m_args, [&stream](const auto a) { writeArg(stream, a); });
    stream << std::endl;
  }

  if (!m_flags.empty()) {
    stream << "Flags" << std::endl;
    std::for_each(m_flags.begin(), m_flags.end(), [&stream](const auto a) {
      writeFlag(stream, a);
    });
    stream << std::endl;
  }
}

void util::ProgramOptions::Parser::printHelp(std::string_view error_msg) {
  bool error = !error_msg.empty();

  if (error) {
    std::cerr << "ERROR: " << error_msg << std::endl;
  }

  if (!m_program_name.empty()) {
    argListShort(std::cout, m_program_name);
  }
  argListLong(std::cout);
}
