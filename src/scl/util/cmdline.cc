/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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

namespace {}  // namespace

bool scl::util::ProgramOptions::Parser::IsArg(std::string_view name) const {
  return std::any_of(mArgs.begin(), mArgs.end(), [&](auto a) {
    return a.name == name;
  });
}

bool scl::util::ProgramOptions::Parser::IsFlag(std::string_view name) const {
  return std::any_of(mFlags.begin(), mFlags.end(), [&](auto f) {
    return f.name == name;
  });
}

namespace {

bool Name(std::string_view opt_name, std::string_view& name) {
  if (opt_name[0] != '-') {
    return false;
  }
  name = opt_name.substr(1, opt_name.size());
  return true;
}

}  // namespace

scl::util::ProgramOptions scl::util::ProgramOptions::Parser::Parse(
    int argc,
    char** argv) {
  mProgramName = argv[0];
  std::vector<std::string_view> cmd_args(argv + 1, argv + argc);

  const auto help_needed = std::any_of(cmd_args.begin(),
                                       cmd_args.end(),
                                       [](auto e) { return e == "-help"; });
  if (help_needed) {
    PrintHelp();
  }

  std::unordered_map<std::string_view, std::string_view> args;
  std::for_each(mArgs.begin(), mArgs.end(), [&args](const auto arg) {
    if (arg.default_value.has_value()) {
      args[arg.name] = arg.default_value.value();
    }
  });

  std::unordered_map<std::string_view, bool> flags;
  std::size_t i = 0;
  while (i < cmd_args.size()) {
    std::string_view name;
    if (!Name(cmd_args[i++], name)) {
      PrintHelp("argument must begin with '-'");
    }

    if (IsArg(name)) {
      if (i == cmd_args.size()) {
        PrintHelp("invalid argument");
      }
      args[name] = cmd_args[i++];
    } else if (IsFlag(name)) {
      flags[name] = true;
    } else {
      PrintHelp("encountered unknown argument");
    }
  }

  // check if we got everything
  ForEachRequired(mArgs, [&](const auto arg) {
    if (args.find(arg.name) == args.end()) {
      PrintHelp("missing required argument");
    }
  });

  return ProgramOptions(args, flags);
}

void scl::util::ProgramOptions::Parser::ArgListShort(
    std::ostream& stream,
    std::string_view program_name) const {
  stream << "Usage: " << program_name << " ";
  ForEachRequired(mArgs, [&stream](const auto arg) {
    stream << "-" << arg.name << " " << arg.type_hint << " ";
  });

  stream << "[options ...]" << std::endl;
}

std::string GetPadding(std::size_t lead) {
  const static std::size_t padding = 20;
  const static std::size_t min_padding = 5;
  const auto psz = lead >= padding + min_padding ? min_padding : padding - lead;
  return std::string(psz, ' ');
}

void WriteArg(std::ostream& stream, const scl::util::ProgramArg& arg) {
  stream << " -" << arg.name << " '" << arg.type_hint << "'";
  if (!arg.description.empty()) {
    const auto pad_str = GetPadding(arg.name.size() + arg.type_hint.size() + 5);
    stream << pad_str << arg.description << ". ";
  }
  if (arg.default_value.has_value()) {
    stream << " [default=" << arg.default_value.value() << "]";
  }
  stream << std::endl;
}

void WriteFlag(std::ostream& stream, const scl::util::ProgramFlag& flag) {
  stream << " -" << flag.name;
  if (!flag.description.empty()) {
    const auto pad_str = GetPadding(flag.name.size() + 2);
    stream << pad_str << flag.description << ". ";
  }
  stream << std::endl;
}

template <typename It>
bool HasRequired(It begin, It end) {
  return std::any_of(begin, end, [](const auto a) { return a.required; });
}

template <typename It>
bool HasOptional(It begin, It end) {
  return std::any_of(begin, end, [](const auto a) { return !a.required; });
}

void scl::util::ProgramOptions::Parser::ArgListLong(
    std::ostream& stream) const {
  if (!mDescription.empty()) {
    stream << std::endl << mDescription << std::endl;
  }
  stream << std::endl;

  const auto has_req_arg = HasRequired(mArgs.begin(), mArgs.end());

  if (has_req_arg) {
    stream << "Required arguments" << std::endl;
    ForEachRequired(mArgs, [&stream](const auto a) { WriteArg(stream, a); });
    stream << std::endl;
  }

  if (HasOptional(mArgs.begin(), mArgs.end())) {
    stream << "Optional Arguments" << std::endl;

    ForEachOptional(mArgs, [&stream](const auto a) { WriteArg(stream, a); });
    stream << std::endl;
  }

  if (!mFlags.empty()) {
    stream << "Flags" << std::endl;
    std::for_each(mFlags.begin(), mFlags.end(), [&stream](const auto a) {
      WriteFlag(stream, a);
    });
    stream << std::endl;
  }
}

void scl::util::ProgramOptions::Parser::PrintHelp(std::string_view error_msg) {
  bool error = !error_msg.empty();

  if (error) {
    std::cerr << "ERROR: " << error_msg << std::endl;
  }

  if (!mProgramName.empty()) {
    ArgListShort(std::cout, mProgramName);
  }
  ArgListLong(std::cout);

#ifdef SCL_UTIL_NO_EXIT_ON_ERROR

  throw std::runtime_error(error ? "bad" : "good");

#else

  std::exit(error ? 1 : 0);

#endif
}
