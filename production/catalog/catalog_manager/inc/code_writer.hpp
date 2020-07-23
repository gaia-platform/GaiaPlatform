/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

/*
 * Copyright 2016 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Utility class to assist in generating code through use of text templates.
//
// Example code:
//   code_writer code("\t");
//   code.set_value("NAME", "Foo");
//   code += "void {{NAME}}() { printf("%s", "{{NAME}}"); }";
//   code.set_value("NAME", "Bar");
//   code += "void {{NAME}}() { printf("%s", "{{NAME}}"); }";
//   std::cout << code.to_string() << std::endl;
//
// Output:
//  void Foo() { printf("%s", "Foo"); }
//  void Bar() { printf("%s", "Bar"); }

#include <string>
#include <sstream>
#include <map>

class code_writer {
 public:
  code_writer(std::string pad = std::string())
      : m_pad(pad), m_cur_indent_lvl(0), m_ignore_indent(false) {}

  // Clears the current "written" code.
  void clear() {
    m_stream.str("");
    m_stream.clear();
  }

  // Associates a key with a value.  All subsequent calls to operator+=, where
  // the specified key is contained in {{ and }} delimiters will be replaced by
  // the given value.
  void set_value(const std::string &key, const std::string &value) {
    m_value_map[key] = value;
  }

  std::string get_value(const std::string &key) const {
    const auto it = m_value_map.find(key);
    return it == m_value_map.end() ? "" : it->second;
  }

  // Appends the given text to the generated code as well as a newline
  // character.  Any text within {{ and }} delimeters is replaced by values
  // previously stored in the code_writer by calling set_value above.  The newline
  // will be suppressed if the text ends with the \\ character.
  void operator+=(std::string text);

  // Returns the current contents of the code_writer as a std::string.
  std::string to_string() const { return m_stream.str(); }

  // Increase indent level for writing code
  void increment_indent_level() { m_cur_indent_lvl++; }
  // Decrease indent level for writing code
  void decrement_indent_level() {
    if (m_cur_indent_lvl) m_cur_indent_lvl--;
  }

 private:
  std::map<std::string, std::string> m_value_map;
  std::stringstream m_stream;
  std::string m_pad;
  int m_cur_indent_lvl;
  bool m_ignore_indent;

  // Add indent padding (tab or space) based on indent level
  void append_indent(std::stringstream &stream);
};
