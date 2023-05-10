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

#ifndef SCL_UTIL_CMDLINE_H
#define SCL_UTIL_CMDLINE_H

#include <algorithm>
#include <iostream>
#include <list>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace scl::util {

/**
 * @brief Simple command line argument parser.
 *
 * ProgramOptions allows defining and parsing options for a program in a limited
 * manner using a builder pattern. For example:
 *
 * @code
 * auto p = ProgramOptions::Parser("some description")
 *            .Add(ProgramArg::Required("foo", "int", "foo description"))
 *            .Add(ProgramArg::Optional("bar", "bool", "123"))
 *            .Add(ProgramFlag("flag"))
 *            .Parse(argc, argv);
 * @endcode
 *
 * The above snippet will parse the <code>argv</code> argument vector passed to
 * a program looking for arguments <code>-foo value</code> and
 * <code>flag</code>. The <code>bar</code> is optional and if not explicitly
 * supplied, gets the default value <code>"123"</code>.
 */
class ProgramOptions {
 public:
  class Parser;

  /**
   * @brief Check if some argument has been provided.
   * @param name the name of the argument.
   * @return true if the argument was set, false otherwise.
   */
  bool Has(std::string_view name) const {
    return mArgs.find(name) != mArgs.end();
  }

  /**
   * @brief Check if a flag has been set.
   * @param name the name of the flag.
   * @return true if the flag was set, false otherwise.
   */
  bool FlagSet(std::string_view name) const {
    return mFlags.find(name) != mFlags.end();
  }

  /**
   * @brief Get the raw value of an argument.
   * @param name the name of the argument.
   * @return the value of the argument, as is.
   */
  std::string_view Get(std::string_view name) const {
    return mArgs.at(name);
  }

  /**
   * @brief Get the value of an argument with conversion.
   * @tparam T the type to convert the argument to.
   * @param name the name of the argument.
   * @return the value of the argument after conversion.
   *
   * Specializations exist for this function for <code>bool</code>,
   * <code>int</code> and <code>std::size_t</code>. It is possible to provide
   * custom specializations that can be used to turn a string into any kind of
   * object.
   */
  template <typename T>
  T Get(std::string_view name) const;

 private:
  ProgramOptions(
      const std::unordered_map<std::string_view, std::string_view>& args,
      const std::unordered_map<std::string_view, bool>& flags)
      : mArgs(args), mFlags(flags){};

  std::unordered_map<std::string_view, std::string_view> mArgs;
  std::unordered_map<std::string_view, bool> mFlags;
};

/**
 * @brief Specialization of CmdArgs::Get for <code>bool</code>.
 */
template <>
inline bool ProgramOptions::Get<bool>(std::string_view name) const {
  const auto v = mArgs.at(name);
  return v == "1" || v == "true";
}

/**
 * @brief Specialization for CmdArgs::Get for <code>int</code>.
 */
template <>
inline int ProgramOptions::Get<int>(std::string_view name) const {
  return std::stoi(mArgs.at(name).data());
}

/**
 * @brief Specialization of CmdArgs::Get for <code>std::size_t</code>.
 */
template <>
inline std::size_t ProgramOptions::Get<std::size_t>(
    std::string_view name) const {
  return std::stoul(mArgs.at(name).data());
}

/**
 * @brief An command-line argument definition.
 */
struct ProgramArg {
  /**
   * @brief Create a required command-line argument.
   * @param name the name.
   * @param type_hint a string describing the expected type. E.g., "int".
   * @param description a short description.
   */
  static ProgramArg Required(std::string_view name,
                             std::string_view type_hint,
                             std::string_view description = "") {
    return ProgramArg{true, name, type_hint, description, {}};
  }

  /**
   * @brief Create an optional command-line argument.
   * @param name the name.
   * @param type_hint a string describing the expected type. E.g., "int".
   * @param default_value an optional default value.
   * @param description a short description.
   */
  static ProgramArg Optional(std::string_view name,
                             std::string_view type_hint,
                             std::optional<std::string_view> default_value,
                             std::string_view description = "") {
    return ProgramArg{false, name, type_hint, description, default_value};
  }

  /**
   * @brief Whether this argument is required.
   */
  bool required;

  /**
   * @brief The name of this argument.
   */
  std::string_view name;

  /**
   * @brief A type hint. Only used as part of the description.
   */
  std::string_view type_hint;

  /**
   * @brief A short description of this argument.
   */
  std::string_view description;

  /**
   * @brief A default value. Ignored if \p required is true.
   */
  std::optional<std::string_view> default_value;
};

/**
 * @brief A command-line argument flag definition.
 */
struct ProgramFlag {
  /**
   * @brief Create a flag argument.
   * @param name the name of the flag.
   * @param description a description.
   */
  ProgramFlag(std::string_view name, std::string_view description = "")
      : name(name), description(description) {}

  /**
   * @brief The name.
   */
  std::string_view name;

  /**
   * @brief A short descruption.
   */
  std::string_view description;
};

/**
 * @brief Argument parser.
 *
 * The parser accepts argument defintions (through the Add functions) and
 * parses the arguments provided to the main function into a CmdArgs object.
 */
class ProgramOptions::Parser {
 public:
  /**
   * @brief Create a command-line argument parser.
   * @param description a short description of the program.
   */
  Parser(std::string_view description = "") : mDescription(description) {}

  /**
   * @brief Define an argument.
   * @param def an argument definition.
   */
  Parser& Add(const ProgramArg& def) {
    if (Exists(def)) {
      PrintHelp("duplicate argument definition");
    }
    mArgs.emplace_back(def);
    return *this;
  }

  /**
   * @brief Define a flag argument.
   * @param flag a flag definition.
   */
  Parser& Add(const ProgramFlag& flag) {
    if (Exists(flag)) {
      PrintHelp("duplicate argument definition");
    }
    mFlags.emplace_back(flag);
    return *this;
  }

  /**
   * @brief Parse arguments.
   * @param argc the number of arguments.
   * @param argv the arguments.
   *
   * The \p argc and \p argv are assumed to be the inputs to a programs main
   * function.
   */
  ProgramOptions Parse(int argc, char* argv[]);

  /**
   * @brief Print a help string to stdout.
   */
  void Help() const {
    ArgListLong(std::cout);
  }

 private:
  template <typename T>
  bool Exists(const T& arg_or_flag) const;
  void ArgListShort(std::ostream& stream, std::string_view program_name) const;
  void ArgListLong(std::ostream& stream) const;

  bool IsArg(std::string_view name) const;
  bool IsFlag(std::string_view name) const;

  template <typename P>
  void ForEachOptional(const std::list<ProgramArg>& list, P pred) const {
    std::for_each(list.begin(), list.end(), [&](const auto e) {
      if (!e.required) {
        pred(e);
      }
    });
  }

  template <typename P>
  void ForEachRequired(const std::list<ProgramArg>& list, P pred) const {
    std::for_each(list.begin(), list.end(), [&](const auto e) {
      if (e.required) {
        pred(e);
      }
    });
  }

  void PrintHelp(std::string_view error_msg = "");

  std::string_view mDescription;
  std::string_view mProgramName;

  std::list<ProgramArg> mArgs;
  std::list<ProgramFlag> mFlags;
};

template <typename T>
bool ProgramOptions::Parser::Exists(const T& arg_or_flag) const {
  const auto exists_a = std::any_of(mArgs.begin(), mArgs.end(), [&](auto a) {
    return a.name == arg_or_flag.name;
  });
  if (exists_a) {
    return true;
  }

  const auto exists_f = std::any_of(mFlags.begin(), mFlags.end(), [&](auto a) {
    return a.name == arg_or_flag.name;
  });
  return exists_f;
}

}  // namespace scl::util

#endif  // SCL_UTIL_CMDLINE_H
