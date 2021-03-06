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
#include "DHTRoutingTableDeserializer.h"

#include <cstring>
#include <cassert>
#include <istream>
#include <utility>

#include "DHTNode.h"
#include "DHTConstants.h"
#include "bittorrent_helper.h"
#include "DlAbortEx.h"
#include "Logger.h"
#include "a2netcompat.h"
#include "fmt.h"
#include "util.h"
#include "array_fun.h"

namespace aria2 {

DHTRoutingTableDeserializer::DHTRoutingTableDeserializer(int family):
  family_(family) {}

DHTRoutingTableDeserializer::~DHTRoutingTableDeserializer() {}

namespace {
void readBytes(unsigned char* buf, size_t buflen,
               std::istream& in, size_t readlen)
{
  assert(readlen <= buflen);
  in.read(reinterpret_cast<char*>(buf), readlen);
}
} // namespace

#define CHECK_STREAM(in, length)                                        \
  if(in.gcount() != length) {                                           \
    throw DL_ABORT_EX                                                   \
      (fmt("Failed to load DHT routing table. cause:%s",                \
           "Unexpected EOF"));                                          \
  }                                                                     \
  if(!in) {                                                             \
    throw DL_ABORT_EX("Failed to load DHT routing table.");             \
  }

void DHTRoutingTableDeserializer::deserialize(std::istream& in)
{
  char header[8];
  memset(header, 0, sizeof(header));
  // magic
  header[0] = 0xa1u;
  header[1] = 0xa2u;
  // format ID
  header[2] = 0x02u;
  // version
  header[6] = 0;
  header[7] = 0x03u;

  char headerCompat[8];
  memset(headerCompat, 0, sizeof(headerCompat));
  // magic
  headerCompat[0] = 0xa1u;
  headerCompat[1] = 0xa2u;
  // format ID
  headerCompat[2] = 0x02u;
  // version
  headerCompat[6] = 0;
  headerCompat[7] = 0x02u;

  char zero[18];
  memset(zero, 0, sizeof(zero));

  int version;

  // If you change the code to read more than the size of buf, then
  // expand the buf size here.
  array_wrapper<unsigned char, 255> buf;

  // header
  readBytes(buf, buf.size(), in, 8);
  CHECK_STREAM(in, 8);
  if(memcmp(header, buf, 8) == 0) {
    version = 3;
  } else if(memcmp(headerCompat, buf, 8) == 0) {
    version = 2;
  } else {
    throw DL_ABORT_EX
      (fmt("Failed to load DHT routing table. cause:%s",
           "bad header"));
  }
  
  uint32_t temp32;
  uint64_t temp64;
  // time
  if(version == 2) {
    in.read(reinterpret_cast<char*>(&temp32), sizeof(temp32));
    CHECK_STREAM(in, sizeof(temp32));
    serializedTime_.setTimeInSec(ntohl(temp32));
    // 4bytes reserved
    readBytes(buf, buf.size(), in, 4);
    CHECK_STREAM(in, 4);
  } else {
    in.read(reinterpret_cast<char*>(&temp64), sizeof(temp64));
    CHECK_STREAM(in, sizeof(temp64));
    serializedTime_.setTimeInSec(ntoh64(temp64));
  }
  
  // localnode
  // 8bytes reserved
  readBytes(buf, buf.size(), in, 8);
  CHECK_STREAM(in, 8);
  // localnode ID
  readBytes(buf, buf.size(), in, DHT_ID_LENGTH);
  CHECK_STREAM(in, DHT_ID_LENGTH);
  SharedHandle<DHTNode> localNode(new DHTNode(buf));
  // 4bytes reserved
  readBytes(buf, buf.size(), in, 4);
  CHECK_STREAM(in, 4);

  // number of nodes
  in.read(reinterpret_cast<char*>(&temp32), sizeof(temp32));
  CHECK_STREAM(in, sizeof(temp32));
  uint32_t numNodes = ntohl(temp32);
  // 4bytes reserved
  readBytes(buf, buf.size(), in, 4);
  CHECK_STREAM(in, 4);

  std::vector<SharedHandle<DHTNode> > nodes;
  // nodes
  const int compactlen = bittorrent::getCompactLength(family_);
  for(size_t i = 0; i < numNodes; ++i) {
    // 1byte compact peer info length
    uint8_t peerInfoLen;
    in >> peerInfoLen;
    if(peerInfoLen != compactlen) {
      // skip this entry
      readBytes(buf, buf.size(), in, 7+48);
      CHECK_STREAM(in, 7+48);
      continue;
    }
    // 7bytes reserved
    readBytes(buf, buf.size(), in, 7);
    CHECK_STREAM(in, 7);
    // compactlen bytes compact peer info
    readBytes(buf, buf.size(), in, compactlen);
    CHECK_STREAM(in, compactlen);
    if(memcmp(zero, buf, compactlen) == 0) {
      // skip this entry
      readBytes(buf, buf.size(), in, 48-compactlen);
      CHECK_STREAM(in, 48-compactlen);
      continue;
    }
    std::pair<std::string, uint16_t> peer =
      bittorrent::unpackcompact(buf, family_);
    if(peer.first.empty()) {
      // skip this entry
      readBytes(buf, buf.size(), in, 48-compactlen);
      CHECK_STREAM(in, 48-compactlen);
      continue;
    }
    // 24-compactlen bytes reserved
    readBytes(buf, buf.size(), in, 24-compactlen);
    // node ID
    readBytes(buf, buf.size(), in, DHT_ID_LENGTH);
    CHECK_STREAM(in, DHT_ID_LENGTH);

    SharedHandle<DHTNode> node(new DHTNode(buf));
    node->setIPAddress(peer.first);
    node->setPort(peer.second);
    // 4bytes reserved
    readBytes(buf, buf.size(), in, 4);
    CHECK_STREAM(in, 4);

    nodes.push_back(node);
  }
  localNode_ = localNode;
  nodes_ = nodes;
}

} // namespace aria2
