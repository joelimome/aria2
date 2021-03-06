/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2010 Tatsuhiro Tsujikawa
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
#include "OptionHandlerImpl.h"

#include <cassert>
#include <cstdio>
#include <utility>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iterator>
#include <vector>

#include "util.h"
#include "DlAbortEx.h"
#include "prefs.h"
#include "Option.h"
#include "fmt.h"
#include "A2STR.h"
#include "Request.h"
#include "a2functional.h"
#include "message.h"
#include "File.h"
#include "FileEntry.h"
#include "a2io.h"

namespace aria2 {

NullOptionHandler::~NullOptionHandler() {}

bool NullOptionHandler::canHandle(const std::string& optName) { return true; }

void NullOptionHandler::parse(Option& option, const std::string& arg) {}

bool NullOptionHandler::hasTag(const std::string& tag) const { return false; }

void NullOptionHandler::addTag(const std::string& tag) {}

std::string NullOptionHandler::toTagString() const { return A2STR::NIL; }

const std::string& NullOptionHandler::getName() const { return A2STR::NIL; }

const std::string& NullOptionHandler::getDescription() const
{
  return A2STR::NIL;
}

const std::string& NullOptionHandler::getDefaultValue() const
{
  return A2STR::NIL;
}

std::string NullOptionHandler::createPossibleValuesString() const
{
  return A2STR::NIL;
}

bool NullOptionHandler::isHidden() const
{
  return true;
}

void NullOptionHandler::hide() {}

OptionHandler::ARG_TYPE NullOptionHandler::getArgType() const
{
  return OptionHandler::NO_ARG;
}

int NullOptionHandler::getOptionID() const
{
  return id_;
}

void NullOptionHandler::setOptionID(int id)
{
  id_ = id;
}

char NullOptionHandler::getShortName() const
{
  return 0;
}

BooleanOptionHandler::BooleanOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 OptionHandler::ARG_TYPE argType,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           argType, shortName)
{}

BooleanOptionHandler::~BooleanOptionHandler() {}

void BooleanOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  if(optarg == "true" ||
     ((argType_ == OptionHandler::OPT_ARG ||
       argType_ == OptionHandler::NO_ARG)
      && optarg.empty())) {
    option.put(optName_, A2_V_TRUE);
  } else if(optarg == "false") {
    option.put(optName_, A2_V_FALSE);
  } else {
    std::string msg = optName_;
    strappend(msg, " ", _("must be either 'true' or 'false'."));
    throw DL_ABORT_EX(msg);
  }
}

std::string BooleanOptionHandler::createPossibleValuesString() const
{
  return "true, false";
}

IntegerRangeOptionHandler::IntegerRangeOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 int32_t min, int32_t max,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    min_(min),
    max_(max)
{}

IntegerRangeOptionHandler::~IntegerRangeOptionHandler() {}

void IntegerRangeOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  IntSequence seq = util::parseIntRange(optarg);
  while(seq.hasNext()) {
    int32_t v = seq.next();
    if(v < min_ || max_ < v) {
      std::string msg = optName_;
      strappend(msg, " ", _("must be between %s and %s."));
      throw DL_ABORT_EX
        (fmt(msg.c_str(), util::itos(min_).c_str(),
             util::itos(max_).c_str()));
    }
    option.put(optName_, optarg);
  }
}

std::string IntegerRangeOptionHandler::createPossibleValuesString() const
{
  return util::itos(min_)+"-"+util::itos(max_);
}

NumberOptionHandler::NumberOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 int64_t min,
 int64_t max,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    min_(min),
    max_(max)
{}

NumberOptionHandler::~NumberOptionHandler() {}

void NumberOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  int64_t num = util::parseLLInt(optarg);
  parseArg(option, num);
}

void NumberOptionHandler::parseArg(Option& option, int64_t number)
{
  if((min_ == -1 || min_ <= number) && (max_ ==  -1 || number <= max_)) {
    option.put(optName_, util::itos(number));
  } else {
    std::string msg = optName_;
    msg += " ";
    if(min_ == -1 && max_ != -1) {
      msg += fmt(_("must be smaller than or equal to %s."),
                 util::itos(max_).c_str());
    } else if(min_ != -1 && max_ != -1) {
      msg += fmt(_("must be between %s and %s."),
                 util::itos(min_).c_str(),
                 util::itos(max_).c_str());
    } else if(min_ != -1 && max_ == -1) {
      msg += fmt(_("must be greater than or equal to %s."),
                 util::itos(min_).c_str());
    } else {
      msg += _("must be a number.");
    }
    throw DL_ABORT_EX(msg);
  }
}

std::string NumberOptionHandler::createPossibleValuesString() const
{
  std::string values;
  if(min_ == -1) {
    values += "*";
  } else {
    values += util::itos(min_);
  }
  values += "-";
  if(max_ == -1) {
    values += "*";
  } else {
    values += util::itos(max_);
  }
  return values;
}

UnitNumberOptionHandler::UnitNumberOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 int64_t min,
 int64_t max,
 char shortName)
  : NumberOptionHandler(optName, description, defaultValue, min, max,
                        shortName)
{}

UnitNumberOptionHandler::~UnitNumberOptionHandler() {}

void UnitNumberOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  int64_t num = util::getRealSize(optarg);
  NumberOptionHandler::parseArg(option, num);
}

FloatNumberOptionHandler::FloatNumberOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 double min,
 double max,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    min_(min),
    max_(max)
{}

FloatNumberOptionHandler::~FloatNumberOptionHandler() {}

void FloatNumberOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  double number = strtod(optarg.c_str(), 0);
  if((min_ < 0 || min_ <= number) && (max_ < 0 || number <= max_)) {
    option.put(optName_, optarg);
  } else {
    std::string msg = optName_;
    msg += " ";
    if(min_ < 0 && max_ >= 0) {
      msg += fmt(_("must be smaller than or equal to %.1f."), max_);
    } else if(min_ >= 0 && max_ >= 0) {
      msg += fmt(_("must be between %.1f and %.1f."), min_, max_);
    } else if(min_ >= 0 && max_ < 0) {
      msg += fmt(_("must be greater than or equal to %.1f."), min_);
    } else {
      msg += _("must be a number.");
    }
    throw DL_ABORT_EX(msg);
  }
}

std::string FloatNumberOptionHandler::createPossibleValuesString() const
{
  std::string valuesString;
  if(min_ < 0) {
    valuesString += "*";
  } else {
    char buf[11];
    snprintf(buf, sizeof(buf), "%.1f", min_);
    valuesString += buf;
  }
  valuesString += "-";
  if(max_ < 0) {
    valuesString += "*";
  } else {
    char buf[11];
    snprintf(buf, sizeof(buf), "%.1f", max_);
    valuesString += buf;
  }
  return valuesString;
}

DefaultOptionHandler::DefaultOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::string& possibleValuesString,
 OptionHandler::ARG_TYPE argType,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue, argType,
                           shortName),
    possibleValuesString_(possibleValuesString)
{}

DefaultOptionHandler::~DefaultOptionHandler() {}

void DefaultOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  option.put(optName_, optarg);
}

std::string DefaultOptionHandler::createPossibleValuesString() const
{
  return possibleValuesString_;
}

CumulativeOptionHandler::CumulativeOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::string& delim,
 const std::string& possibleValuesString,
 OptionHandler::ARG_TYPE argType,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue, argType,
                           shortName),
    delim_(delim),
    possibleValuesString_(possibleValuesString)
{}

CumulativeOptionHandler::~CumulativeOptionHandler() {}

void CumulativeOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  std::string value = option.get(optName_);
  strappend(value, optarg, delim_);
  option.put(optName_, value);
}

std::string CumulativeOptionHandler::createPossibleValuesString() const
{
  return possibleValuesString_;
}

IndexOutOptionHandler::IndexOutOptionHandler
(const std::string& optName,
 const std::string& description,
 char shortName)
  : NameMatchOptionHandler(optName, description, NO_DEFAULT_VALUE,
                           OptionHandler::REQ_ARG, shortName)
{}

IndexOutOptionHandler::~IndexOutOptionHandler() {}

void IndexOutOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  // See optarg is in the fomrat of "INDEX=PATH"
  util::parseIndexPath(optarg);
  std::string value = option.get(optName_);
  strappend(value, optarg, "\n");
  option.put(optName_, value);
}

std::string IndexOutOptionHandler::createPossibleValuesString() const
{
  return "INDEX=PATH";
}

ParameterOptionHandler::ParameterOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::vector<std::string>& validParamValues,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    validParamValues_(validParamValues)
{}

ParameterOptionHandler::ParameterOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::string& validParamValue,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName)
{
  validParamValues_.push_back(validParamValue);
}

ParameterOptionHandler::ParameterOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::string& validParamValue1,
 const std::string& validParamValue2,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName)
{
  validParamValues_.push_back(validParamValue1);
  validParamValues_.push_back(validParamValue2);
}

ParameterOptionHandler::ParameterOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::string& validParamValue1,
 const std::string& validParamValue2,
 const std::string& validParamValue3,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName)
{
  validParamValues_.push_back(validParamValue1);
  validParamValues_.push_back(validParamValue2);
  validParamValues_.push_back(validParamValue3);
}
   
ParameterOptionHandler::~ParameterOptionHandler() {}

void ParameterOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  std::vector<std::string>::const_iterator itr =
    std::find(validParamValues_.begin(), validParamValues_.end(), optarg);
  if(itr == validParamValues_.end()) {
    std::string msg = optName_;
    strappend(msg, " ", _("must be one of the following:"));
    if(validParamValues_.size() == 0) {
      msg += "''";
    } else {
      for(std::vector<std::string>::const_iterator itr =
            validParamValues_.begin(), eoi = validParamValues_.end();
          itr != eoi; ++itr) {
        strappend(msg, "'", *itr, "' ");
      }
    }
    throw DL_ABORT_EX(msg);
  } else {
    option.put(optName_, optarg);
  }
}

std::string ParameterOptionHandler::createPossibleValuesString() const
{
  std::stringstream s;
  std::copy(validParamValues_.begin(), validParamValues_.end(),
            std::ostream_iterator<std::string>(s, ", "));
  return util::strip(s.str(), ", ");
}

HostPortOptionHandler::HostPortOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 const std::string& hostOptionName,
 const std::string& portOptionName,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    hostOptionName_(hostOptionName),
    portOptionName_(portOptionName)
{}

HostPortOptionHandler::~HostPortOptionHandler() {}

void HostPortOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  std::string uri = "http://";
  uri += optarg;
  Request req;
  if(!req.setUri(uri)) {
    throw DL_ABORT_EX(_("Unrecognized format"));
  }
  option.put(optName_, optarg);
  setHostAndPort(option, req.getHost(), req.getPort());
}

void HostPortOptionHandler::setHostAndPort
(Option& option, const std::string& hostname, uint16_t port)
{
  option.put(hostOptionName_, hostname);
  option.put(portOptionName_, util::uitos(port));
}

std::string HostPortOptionHandler::createPossibleValuesString() const
{
  return "HOST:PORT";
}

HttpProxyUserOptionHandler::HttpProxyUserOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName)
{}

void HttpProxyUserOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  if(util::endsWith(optName_, "-user")) {
    const std::string proxyPref = optName_.substr(0, optName_.size()-5);
    const std::string& olduri = option.get(proxyPref);
    if(!olduri.empty()) {
      Request req;
      bool b = req.setUri(olduri);
      assert(b);
      std::string uri = "http://";
      if(!optarg.empty()) {
        uri += util::percentEncode(optarg);
      }
      if(req.hasPassword()) {
        uri += A2STR::COLON_C;
        uri += util::percentEncode(req.getPassword());
      }
      if(uri.size() > 7) {
        uri += "@";
      }
      strappend(uri, req.getHost(),A2STR::COLON_C,util::uitos(req.getPort()));
      option.put(proxyPref, uri);
    }
  }
  option.put(optName_, optarg);
}

std::string HttpProxyUserOptionHandler::createPossibleValuesString() const
{
  return "";
}

HttpProxyPasswdOptionHandler::HttpProxyPasswdOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName)
{}

void HttpProxyPasswdOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  if(util::endsWith(optName_, "-passwd")) {
    const std::string proxyPref = optName_.substr(0, optName_.size()-7);
    const std::string& olduri = option.get(proxyPref);
    if(!olduri.empty()) {
      Request req;
      bool b = req.setUri(olduri);
      assert(b);
      std::string uri = "http://";
      if(!req.getUsername().empty()) {
        uri += util::percentEncode(req.getUsername());
      }
      uri += A2STR::COLON_C;
      if(!optarg.empty()) {
        uri += util::percentEncode(optarg);
      }
      if(uri.size() > 7) {
        uri += "@";
      }
      strappend(uri, req.getHost(), A2STR::COLON_C,util::itos(req.getPort()));
      option.put(proxyPref, uri);
    }
  }
  option.put(optName_, optarg);
}

std::string HttpProxyPasswdOptionHandler::createPossibleValuesString() const
{
  return "";
}

HttpProxyOptionHandler::HttpProxyOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    proxyUserPref_(optName_+"-user"),
    proxyPasswdPref_(optName_+"-passwd")
{}

HttpProxyOptionHandler::~HttpProxyOptionHandler() {}

void HttpProxyOptionHandler::parseArg(Option& option, const std::string& optarg)
{
  if(optarg.empty()) {
    option.put(optName_, optarg);
  } else {
    Request req;
    std::string uri;
    if(util::startsWith(optarg, "http://") ||
       util::startsWith(optarg, "https://") ||
       util::startsWith(optarg, "ftp://")) {
      uri = optarg;
    } else {
      uri = "http://";
      uri += optarg;
    }
    if(!req.setUri(uri)) {
      throw DL_ABORT_EX(_("unrecognized proxy format"));
    }
    uri = "http://";
    if(req.getUsername().empty()) {
      if(option.defined(proxyUserPref_)) {
        uri += util::percentEncode(option.get(proxyUserPref_));
      }
    } else {
      uri += util::percentEncode(req.getUsername());
    }
    if(!req.hasPassword()) {
      if(option.defined(proxyPasswdPref_)) {
        uri += A2STR::COLON_C;
        uri += util::percentEncode(option.get(proxyPasswdPref_));
      }
    } else {
      uri += A2STR::COLON_C;
      uri += util::percentEncode(req.getPassword());
    }
    if(uri.size() > 7) {
      uri += "@";
    }
    strappend(uri, req.getHost(), A2STR::COLON_C, util::uitos(req.getPort()));
    option.put(optName_, uri);
  }
}

std::string HttpProxyOptionHandler::createPossibleValuesString() const
{
  return "[http://][USER:PASSWORD@]HOST[:PORT]";
}

LocalFilePathOptionHandler::LocalFilePathOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 bool acceptStdin,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName),
    acceptStdin_(acceptStdin)
{}

void LocalFilePathOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  if(acceptStdin_ && optarg == "-") {
    option.put(optName_, DEV_STDIN);
  } else {
    File f(optarg);
    if(!f.exists() || f.isDir()) {
      throw DL_ABORT_EX(fmt(MSG_NOT_FILE, optarg.c_str()));
    }
    option.put(optName_, optarg);
  }
}
  
std::string LocalFilePathOptionHandler::createPossibleValuesString() const
{
  if(acceptStdin_) {
    return PATH_TO_FILE_STDIN;
  } else {
    return PATH_TO_FILE;
  }
}

PrioritizePieceOptionHandler::PrioritizePieceOptionHandler
(const std::string& optName,
 const std::string& description,
 const std::string& defaultValue,
 char shortName)
  : NameMatchOptionHandler(optName, description, defaultValue,
                           OptionHandler::REQ_ARG, shortName)
{}

void PrioritizePieceOptionHandler::parseArg
(Option& option, const std::string& optarg)
{
  // Parse optarg against empty FileEntry list to detect syntax
  // error.
  std::vector<size_t> result;
  util::parsePrioritizePieceRange
    (result, optarg, std::vector<SharedHandle<FileEntry> >(), 1024);
  option.put(optName_, optarg);
}

std::string PrioritizePieceOptionHandler::createPossibleValuesString() const
{
  return "head[=SIZE], tail[=SIZE]";
}

} // namespace aria2
