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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
#include "DHTUtil.h"
#include "MessageDigestHelper.h"
#include <cassert>
#include <fstream>
#include <iomanip>
#ifdef HAVE_LIBGCRYPT
# include <gcrypt.h>
#elif HAVE_LIBSSL
# include <openssl/rand.h>
# include "SimpleRandomizer.h"
#endif // HAVE_LIBSSL

namespace aria2 {

void DHTUtil::generateRandomKey(unsigned char* key)
{
  unsigned char bytes[40];
  generateRandomData(bytes, sizeof(bytes));
  MessageDigestHelper::digest(key, 20, MessageDigestContext::SHA1, bytes, sizeof(bytes));
}

void DHTUtil::generateRandomData(unsigned char* data, size_t length)
{
#ifdef HAVE_LIBGCRYPT
  gcry_randomize(data, length, GCRY_STRONG_RANDOM);
#elif HAVE_LIBSSL
  if(RAND_bytes(data, length) != 1) {
    for(size_t i = 0; i < length; ++i) {
      data[i] = SimpleRandomizer::getInstance()->getRandomNumber(UINT8_MAX+1);
    }
  }
#else
  std::ifstream i("/dev/urandom", std::ios::binary);
  i.read(data, length);
#endif // HAVE_LIBSSL
}

void DHTUtil::flipBit(unsigned char* data, size_t length, size_t bitIndex)
{
  size_t byteIndex = bitIndex/8;
  assert(byteIndex <= length);
  unsigned char mask = 128 >> (bitIndex%8);
  data[byteIndex] ^= mask;
}

} // namespace aria2