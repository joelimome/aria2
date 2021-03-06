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
#ifndef D_METALINK_PARSER_CONTROLLER_H
#define D_METALINK_PARSER_CONTROLLER_H

#include "common.h"

#include <string>
#include <utility>
#include <vector>

#include "SharedHandle.h"

namespace aria2 {

class Metalinker;
class MetalinkEntry;
class MetalinkResource;
class MetalinkMetaurl;
class Signature;

#ifdef ENABLE_MESSAGE_DIGEST
class Checksum;
class ChunkChecksum;
#endif // ENABLE_MESSAGE_DIGEST

class MetalinkParserController {
private:
  SharedHandle<Metalinker> metalinker_;

  SharedHandle<MetalinkEntry> tEntry_;

  SharedHandle<MetalinkResource> tResource_;

  SharedHandle<MetalinkMetaurl> tMetaurl_;
#ifdef ENABLE_MESSAGE_DIGEST
  SharedHandle<Checksum> tChecksum_;

  SharedHandle<ChunkChecksum> tChunkChecksumV4_; // Metalink4Spec

  std::vector<std::string> tempChunkChecksumsV4_; // Metalink4Spec

  SharedHandle<ChunkChecksum> tChunkChecksum_; // Metalink3Spec

  std::vector<std::pair<size_t, std::string> > tempChunkChecksums_;//Metalink3Spec

  std::pair<size_t, std::string> tempHashPair_; // Metalink3Spec

#endif // ENABLE_MESSAGE_DIGEST

  SharedHandle<Signature> tSignature_;
public:
  MetalinkParserController();

  ~MetalinkParserController();

  const SharedHandle<Metalinker>& getResult() const
  {
    return metalinker_;
  }

  void newEntryTransaction();

  void setFileNameOfEntry(const std::string& filename);

  void setFileLengthOfEntry(uint64_t length);

  void setVersionOfEntry(const std::string& version);

  void setLanguageOfEntry(const std::string& language);

  void setOSOfEntry(const std::string& os);

  void setMaxConnectionsOfEntry(int maxConnections);

  void commitEntryTransaction();

  void cancelEntryTransaction();

  void newResourceTransaction();

  void setURLOfResource(const std::string& url);

  void setTypeOfResource(const std::string& type);

  void setLocationOfResource(const std::string& location);

  void setPriorityOfResource(int priority);

  void setMaxConnectionsOfResource(int maxConnections);

  void commitResourceTransaction();

  void cancelResourceTransaction();

  void newChecksumTransaction();

  void setTypeOfChecksum(const std::string& type);

  void setHashOfChecksum(const std::string& md);

  void commitChecksumTransaction();

  void cancelChecksumTransaction();

  void newChunkChecksumTransactionV4(); // Metalink4Spec

  void setTypeOfChunkChecksumV4(const std::string& type); // Metalink4Spec

  void setLengthOfChunkChecksumV4(size_t length); // Metalink4Spec

  void addHashOfChunkChecksumV4(const std::string& md); // Metalink4Spec

  void commitChunkChecksumTransactionV4(); // Metalink4Spec

  void cancelChunkChecksumTransactionV4(); // Metalink4Spec

  void newChunkChecksumTransaction(); // Metalink3Spec

  void setTypeOfChunkChecksum(const std::string& type); // Metalink3Spec

  void setLengthOfChunkChecksum(size_t length); // Metalink3Spec

  void addHashOfChunkChecksum(size_t order, const std::string& md);// Metalink3Spec

  void createNewHashOfChunkChecksum(size_t order); // Metalink3Spec

  void setMessageDigestOfChunkChecksum(const std::string& md); // Metalink3Spec

  void addHashOfChunkChecksum(); // Metalink3Spec

  void commitChunkChecksumTransaction(); // Metalink3Spec

  void cancelChunkChecksumTransaction(); // Metalink3Spec

  void newSignatureTransaction();

  void setTypeOfSignature(const std::string& type);

  void setFileOfSignature(const std::string& file);

  void setBodyOfSignature(const std::string& body);

  void commitSignatureTransaction();

  void cancelSignatureTransaction();

  void newMetaurlTransaction();

  void setURLOfMetaurl(const std::string& url);

  void setMediatypeOfMetaurl(const std::string& mediatype);

  void setPriorityOfMetaurl(int priority);

  void setNameOfMetaurl(const std::string& name);

  void commitMetaurlTransaction();

  void cancelMetaurlTransaction();
};

} // namespace aria2

#endif // D_METALINK_PARSER_CONTROLLER_H
