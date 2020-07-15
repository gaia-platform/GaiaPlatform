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

#include "code_writer.hpp"

void code_writer::operator+=(std::string text) {
  if (!m_ignore_indent && !text.empty()) append_indent(m_stream);

  while (true) {
    auto begin = text.find("{{");
    if (begin == std::string::npos) { break; }

    auto end = text.find("}}");
    if (end == std::string::npos || end < begin) { break; }

    // Write all the text before the first {{ into the stream.
    m_stream.write(text.c_str(), begin);

    // The key is between the {{ and }}.
    const std::string key = text.substr(begin + 2, end - begin - 2);

    // Find the value associated with the key.  If it exists, write the
    // value into the stream, otherwise write the key itself into the stream.
    auto iter = m_value_map.find(key);
    if (iter != m_value_map.end()) {
      const std::string &value = iter->second;
      m_stream << value;
    } else {
      m_stream << key;
    }

    // Update the text to everything after the }}.
    text = text.substr(end + 2);
  }
  if (!text.empty() && text.back() == '\\') {
    text.pop_back();
    m_ignore_indent = true;
    m_stream << text;
  } else {
    m_ignore_indent = false;
    m_stream << text << std::endl;
  }
}

void code_writer::append_indent(std::stringstream &stream) {
  int lvl = m_cur_indent_lvl;
  while (lvl--) {
    stream.write(m_pad.c_str(), static_cast<std::streamsize>(m_pad.size()));
  }
}
