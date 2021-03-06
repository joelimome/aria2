/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#ifndef D_OPTION_H
#define D_OPTION_H

#include "common.h"
#include <string>
#include <map>

namespace aria2 {

class Option {
private:
  std::map<std::string, std::string> table_;
public:
  Option();
  ~Option();

  void put(const std::string& name, const std::string& value);
  // Returns true if name is defined. Otherwise returns false.
  // Note that even if the value is a empty string, this method returns true.
  bool defined(const std::string& name) const;
  // Returns true if name is not defined or the value is a empty string.
  // Otherwise returns false.
  bool blank(const std::string& name) const;
  const std::string& get(const std::string& name) const;
  int32_t getAsInt(const std::string& name) const;
  int64_t getAsLLInt(const std::string& name) const;
  bool getAsBool(const std::string& name) const;
  double getAsDouble(const std::string& name) const;
  
  void remove(const std::string& name);

  void clear();

  std::map<std::string, std::string>::const_iterator begin() const;

  std::map<std::string, std::string>::const_iterator end() const;
};

} // namespace aria2

#endif // D_OPTION_H
