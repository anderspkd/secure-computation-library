/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#ifndef SCL_CMDLINE_H
#define SCL_CMDLINE_H

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace scl {

/**
 * @brief Container for arguments and flags parsed by ProgramOptions::Parser.
 *
 * ProgramOptions holds the result after parsing the stuff in <code>argv</code>;
 * typically, this would be options, flags and so on.
 *
 * The interface of ProgramOptions is pretty intuitive
 * @code
 * ProgramOptions opts = ...  //
 *
 * // check if "-foo 123" was passed in argv
 * opts.has("foo");
 * int foo = opts.get<int>("foo");
 * assert(foo == 123);
 *
 * // the non templated version of get simply returns the argument as
 * // an std::string_view
 * std::string_view foo_str = opts.get("foo");
 * assert(foo_str == "123");
 *
 * // check if "-some_flag" was passed in argv
 * opts.flagSet("some_flag");
 * @endcode
 *
 * ProgramOptions::get stores argument values internally as
 * strings. Specialization is, of course, allowed.
 *
 * @code
 * struct FooStruct {
 *   int x;
 *   int y;
 * };
 *
 * template <>
 * FooStruct scl::ProgramOptions::get<FooStruct>(std::string_view name) const {
 *   FooStruct r;
 *   const auto arg_val = m_args.at(name);
 *   // turn the string arg_val into a FooStruct
 *   return r;
 * }
 *
 * // this will enable us to write
 * FooStruct x = opts.get<FooStruct>();
 * @endcode
 *
 * Specializations exist for <code>int</code>, <code>std::size_t</code> and
 * <code>bool</code>. For the latter, the strings "1" and "true" are treated as
 * <code>true</code>, while everything else is treated as <code>false</code>.
 */
class ProgramOptions {
 public:
  class Parser;

  /**
   * @brief Check if some argument has been provided.
   */
  bool has(std::string_view name) const {
    return m_args.find(name) != m_args.end();
  }

  /**
   * @brief Check if a flag has been set.
   */
  bool flagSet(std::string_view name) const {
    return m_flags.find(name) != m_flags.end();
  }

  /**
   * @brief Get the raw value of an argument.
   */
  std::string_view get(std::string_view name) const {
    return m_args.at(name);
  }

  /**
   * @brief Get the value of an argument with conversion.
   */
  template <typename T>
  T get(std::string_view name) const;

 private:
  ProgramOptions(
      const std::unordered_map<std::string_view, std::string_view>& args,
      const std::unordered_map<std::string_view, bool>& flags)
      : m_args(args), m_flags(flags) {};

  std::unordered_map<std::string_view, std::string_view> m_args;
  std::unordered_map<std::string_view, bool> m_flags;
};

template <>
bool ProgramOptions::get<bool>(std::string_view name) const;

template <>
int ProgramOptions::get<int>(std::string_view name) const;

template <>
std::size_t ProgramOptions::get<std::size_t>(std::string_view name) const;

/**
 * @brief An command-line argument definition.
 */
struct ProgramArg {
  /**
   * @brief Create a required command-line argument.
   *
   * This creates a required program argument. Adding a ProgramArg to a
   * ProgramOptions::Parser through this call with a \p name value of
   * <code>"something"</code> means that the caller of our program must supply
   * <code>-something</code> when calling our program. The type hint is purely
   * cosmetic.
   */
  static ProgramArg required(std::string_view name,
                             std::string_view type_hint,
                             std::string_view description = "") {
    return ProgramArg{true, name, type_hint, description, {}};
  }

  /**
   * @brief Create an optional command-line argument.
   *
   * This creates an optional program argument. In case the argument is not
   * supplied by the caller of our program, the \p default_value will be used.
   */
  static ProgramArg optional(std::string_view name,
                             std::string_view type_hint,
                             std::optional<std::string_view> default_value,
                             std::string_view description = "") {
    return ProgramArg{false, name, type_hint, description, default_value};
  }

  /**
   * @brief Whether this argument is required.
   */
  bool is_required;

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
 * @brief Argument parser for command-line options.
 *
 * Parser provides a builder for constructing a ProgramOptions object based on
 * the stuff in <code>argv</code>.
 *
 * Parser permits the user to create three different types of program arguments:
 * - ProgramArg::required creates an option which accepts one argument, and
 * which must be provided for the program to Parser::parse to work.
 * - ProgramArg::optional creates an option which accepts one argument, and
 * which is optional.
 * - ProgramFlag creates a "flag", or toggle argument.
 *
 * @code
 * // example.cc
 * #include <scl/cmdline.h>
 * #include <iostream>
 *
 * using namespace scl;
 *
 * int main(int argc, char** argv) {
 *   auto parser = ProgramOptions::Parser("super awesome program")
 *                     .add(ProgramArg::required("foo", "int", "foo"))
 *                     .add(ProgramArg::optional("bar", "bool", "true", "bar"))
 *                     .add(ProgramFlag("baz", "baz"));
 *
 *   auto opts = parser.parse(argc, argv);
 *
 *   std::cout << "foo = " << opts.get<int>("foo") << "\n";
 *   std::cout << "bar = " << std::boolalpha << opts.get<bool>("bar") << "\n";
 *   std::cout << "baz set? " << std::boolalpha << opts.flagSet("baz") << "\n";
 * }
 * @endcode
 *
 * \code{.unparsed}
 * $ g++ example.cc -lscl
 * $ ./a.out
 * ERROR: missing required argument
 * Usage: ./a.out -foo int [options ...]
 *
 * super awesome program
 *
 * Required arguments
 *  -foo 'int'         foo.
 *
 * Optional arguments
 *  -bar 'bool'        bar. [default=true]
 *
 * Flags
 *  -baz               baz.
 *
 * $ ./a.out -help
 * Usage: ./a.out -foo int [options ...]
 *
 * super awesome program
 *
 * Required arguments
 *  -foo 'int'         foo.
 *
 * Optional arguments
 *  -bar 'bool'        bar. [default=true]
 *
 * Flags
 *  -baz               baz.
 *
 * $ ./a.out -foo 42
 * foo = 42
 * bar = true
 * baz set? false
 * $ ./a.out -foo 100 -bar false -baz
 * foo = 100
 * bar = false
 * baz set? true
 * $
 * \endcode
 */
class ProgramOptions::Parser {
 public:
  /**
   * @brief Create a command-line argument parser.
   */
  Parser(std::string_view description = "") : m_description(description) {}

  /**
   * @brief Define an argument.
   */
  Parser& add(const ProgramArg& def) {
    m_args.emplace_back(def);
    return *this;
  }

  /**
   * @brief Define a flag argument.
   */
  Parser& add(const ProgramFlag& flag) {
    m_flags.emplace_back(flag);
    return *this;
  }

  /**
   * @brief Parse arguments.
   */
  std::variant<ProgramOptions, std::string_view> parseArguments(int argc,
                                                                char* argv[]);

  /**
   * @brief Parse arguments.
   */
  ProgramOptions parse(int argc, char* argv[], bool exit_on_error = true) {
    auto opts = parseArguments(argc, argv);
    if (opts.index() == 0) {
      return std::get<ProgramOptions>(opts);
    }
    auto error_msg = std::get<std::string_view>(opts);
    printHelp(error_msg);
    if (exit_on_error) {
      std::exit(error_msg.empty() ? 0 : 1);
    } else {
      throw std::runtime_error(error_msg.empty() ? "no error" : "error");
    }
  }

  /**
   * @brief Print a help string to stdout.
   */
  void help() const {
    argListLong(std::cout);
  }

 private:
  std::string_view m_description;
  std::string_view m_program_name;

  std::vector<ProgramArg> m_args;
  std::vector<ProgramFlag> m_flags;

  template <typename T>
  bool exists(const T& arg_or_flag) const;
  void argListShort(std::ostream& stream, std::string_view program_name) const;
  void argListLong(std::ostream& stream) const;

  bool isArg(std::string_view name) const;
  bool isFlag(std::string_view name) const;

  template <typename P>
  void forEachOptional(const std::vector<ProgramArg>& list, P pred) const {
    std::for_each(list.begin(), list.end(), [&](const auto e) {
      if (!e.is_required) {
        pred(e);
      }
    });
  }

  template <typename P>
  void forEachRequired(const std::vector<ProgramArg>& list, P pred) const {
    std::for_each(list.begin(), list.end(), [&](const auto e) {
      if (e.is_required) {
        pred(e);
      }
    });
  }

  void printHelp(std::string_view error_msg = "");
};

template <typename T>
bool ProgramOptions::Parser::exists(const T& arg_or_flag) const {
  const auto exists_a = std::any_of(m_args.begin(), m_args.end(), [&](auto a) {
    return a.name == arg_or_flag.name;
  });
  if (exists_a) {
    return true;
  }

  const auto exists_f =
      std::any_of(m_flags.begin(), m_flags.end(), [&](auto a) {
        return a.name == arg_or_flag.name;
      });
  return exists_f;
}

}  // namespace scl

#endif  // SCL_CMDLINE_H
