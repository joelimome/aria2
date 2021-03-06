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
#include "BitfieldMan.h"

#include <cassert>
#include <cstring>

#include "array_fun.h"
#include "bitfield.h"

using namespace aria2::expr;

namespace aria2 {

BitfieldMan::BitfieldMan(size_t blockLength, uint64_t totalLength)
  :blockLength_(blockLength),
   totalLength_(totalLength),
   bitfieldLength_(0),
   blocks_(0),
   filterEnabled_(false),
   bitfield_(0),
   useBitfield_(0),
   filterBitfield_(0),
   cachedNumMissingBlock_(0),
   cachedNumFilteredBlock_(0),
   cachedCompletedLength_(0),
   cachedFilteredCompletedLength_(0),
   cachedFilteredTotalLength_(0)
{
  if(blockLength_ > 0 && totalLength_ > 0) {
    blocks_ = totalLength_/blockLength_+(totalLength_%blockLength_ ? 1 : 0);
    bitfieldLength_ = blocks_/8+(blocks_%8 ? 1 : 0);
    bitfield_ = new unsigned char[bitfieldLength_];
    useBitfield_ = new unsigned char[bitfieldLength_];
    memset(bitfield_, 0, bitfieldLength_);
    memset(useBitfield_, 0, bitfieldLength_);
    updateCache();
  }
}

BitfieldMan::BitfieldMan(const BitfieldMan& bitfieldMan)
  :blockLength_(bitfieldMan.blockLength_),
   totalLength_(bitfieldMan.totalLength_),
   bitfieldLength_(bitfieldMan.bitfieldLength_),
   blocks_(bitfieldMan.blocks_),
   filterEnabled_(bitfieldMan.filterEnabled_),
   bitfield_(new unsigned char[bitfieldLength_]),
   useBitfield_(new unsigned char[bitfieldLength_]),
   filterBitfield_(0),
   cachedNumMissingBlock_(0),
   cachedNumFilteredBlock_(0),
   cachedCompletedLength_(0),
   cachedFilteredCompletedLength_(0),
   cachedFilteredTotalLength_(0)
{
  memcpy(bitfield_, bitfieldMan.bitfield_, bitfieldLength_);
  memcpy(useBitfield_, bitfieldMan.useBitfield_, bitfieldLength_);
  if(filterEnabled_) {
    filterBitfield_ = new unsigned char[bitfieldLength_];
    memcpy(filterBitfield_, bitfieldMan.filterBitfield_, bitfieldLength_);
  }
  updateCache();
}

BitfieldMan& BitfieldMan::operator=(const BitfieldMan& bitfieldMan)
{
  if(this != &bitfieldMan) {
    blockLength_ = bitfieldMan.blockLength_;
    totalLength_ = bitfieldMan.totalLength_;
    blocks_ = bitfieldMan.blocks_;
    bitfieldLength_ = bitfieldMan.bitfieldLength_;
    filterEnabled_ = bitfieldMan.filterEnabled_;

    delete [] bitfield_;
    bitfield_ = new unsigned char[bitfieldLength_];
    memcpy(bitfield_, bitfieldMan.bitfield_, bitfieldLength_);

    delete [] useBitfield_;
    useBitfield_ = new unsigned char[bitfieldLength_];
    memcpy(useBitfield_, bitfieldMan.useBitfield_, bitfieldLength_);

    delete [] filterBitfield_;
    if(filterEnabled_) {
      filterBitfield_ = new unsigned char[bitfieldLength_];
      memcpy(filterBitfield_, bitfieldMan.filterBitfield_, bitfieldLength_);
    } else {
      filterBitfield_ = 0;
    }

    updateCache();
  }
  return *this;
}

BitfieldMan::~BitfieldMan() {
  delete [] bitfield_;
  delete [] useBitfield_;
  delete [] filterBitfield_;
}

size_t BitfieldMan::getLastBlockLength() const
{
  return totalLength_-blockLength_*(blocks_-1);
}

size_t BitfieldMan::getBlockLength(size_t index) const
{
  if(index == blocks_-1) {
    return getLastBlockLength();
  } else if(index < blocks_-1) {
    return getBlockLength();
  } else {
    return 0;
  }
}

bool BitfieldMan::hasMissingPiece
(const unsigned char* peerBitfield, size_t length) const
{
  if(bitfieldLength_ != length) {
    return false;
  }
  bool retval = false;
  for(size_t i = 0; i < bitfieldLength_; ++i) {
    unsigned char temp = peerBitfield[i] & ~bitfield_[i];
    if(filterEnabled_) {
      temp &= filterBitfield_[i];
    }
    if(temp&0xffu) {
      retval = true;
      break;
    }
  }
  return retval;
}

bool BitfieldMan::getFirstMissingUnusedIndex(size_t& index) const
{
  if(filterEnabled_) {
    return bitfield::getFirstMissingIndex
      (index, ~array(bitfield_)&~array(useBitfield_)&array(filterBitfield_),
       blocks_);
  } else {
    return bitfield::getFirstMissingIndex
      (index, ~array(bitfield_)&~array(useBitfield_), blocks_);
  }
}

size_t BitfieldMan::getFirstNMissingUnusedIndex
(std::vector<size_t>& out, size_t n) const
{
  if(filterEnabled_) {
    return bitfield::getFirstNMissingIndex
      (std::back_inserter(out), n,
       ~array(bitfield_)&~array(useBitfield_)&array(filterBitfield_), blocks_);
  } else {
    return bitfield::getFirstNMissingIndex
      (std::back_inserter(out), n,
       ~array(bitfield_)&~array(useBitfield_), blocks_);
  }
}

bool BitfieldMan::getFirstMissingIndex(size_t& index) const
{
  if(filterEnabled_) {
    return bitfield::getFirstMissingIndex
      (index, ~array(bitfield_)&array(filterBitfield_), blocks_);
  } else {
    return bitfield::getFirstMissingIndex(index, ~array(bitfield_), blocks_);
  }
}

namespace {
template<typename Array>
size_t getStartIndex(size_t index, const Array& bitfield, size_t blocks) {
  while(index < blocks && bitfield::test(bitfield, blocks, index)) {
    ++index;
  }
  if(blocks <= index) {
    return blocks;
  } else {
    return index;
  }
}
} // namespace

namespace {
template<typename Array>
size_t getEndIndex(size_t index, const Array& bitfield, size_t blocks) {
  while(index < blocks && !bitfield::test(bitfield, blocks, index)) {
    ++index;
  }
  return index;
}
} // namespace

namespace {
template<typename Array>
bool getSparseMissingUnusedIndex
(size_t& index,
 size_t minSplitSize,
 const Array& bitfield,
 const unsigned char* useBitfield,
 size_t blockLength_,
 size_t blocks)
{
  BitfieldMan::Range maxRange;
  BitfieldMan::Range currentRange;
  size_t nextIndex = 0;
  while(nextIndex < blocks) {
    currentRange.startIndex =
      getStartIndex(nextIndex, bitfield, blocks);
    if(currentRange.startIndex == blocks) {
      break;
    }
    currentRange.endIndex =
      getEndIndex(currentRange.startIndex, bitfield, blocks);

    if(currentRange.startIndex > 0) {
      if(bitfield::test(useBitfield, blocks, currentRange.startIndex-1)) {
        currentRange.startIndex = currentRange.getMidIndex();
      }
    }
    // If range is equal, choose a range where its startIndex-1 is
    // set.
    if(maxRange < currentRange ||
       (maxRange == currentRange &&
        maxRange.startIndex > 0 && currentRange.startIndex > 0 &&
        (!bitfield::test(bitfield, blocks, maxRange.startIndex-1) ||
         bitfield::test(useBitfield, blocks, maxRange.startIndex-1))
        &&
        bitfield::test(bitfield, blocks, currentRange.startIndex-1) &&
        !bitfield::test(useBitfield, blocks, currentRange.startIndex-1))) {
      maxRange = currentRange;
    }
    nextIndex = currentRange.endIndex;
      
  }
  if(maxRange.getSize()) {
    if(maxRange.startIndex == 0) {
      index = 0;
      return true;
    } else {
      if((!bitfield::test(useBitfield, blocks, maxRange.startIndex-1) &&
          bitfield::test(bitfield, blocks, maxRange.startIndex-1)) ||
         ((uint64_t)(maxRange.endIndex-maxRange.startIndex)*blockLength_
          >= minSplitSize)) {
        index = maxRange.startIndex;
        return true;
      } else {
        return false;
      }
    }
  } else {
    return false;
  }
}
} // namespace

bool BitfieldMan::getSparseMissingUnusedIndex
(size_t& index,
 size_t minSplitSize,
 const unsigned char* ignoreBitfield,
 size_t ignoreBitfieldLength) const
{
  if(filterEnabled_) {
    return aria2::getSparseMissingUnusedIndex
      (index, minSplitSize,
       array(ignoreBitfield)|~array(filterBitfield_)|
       array(bitfield_)|array(useBitfield_),
       useBitfield_, blockLength_, blocks_);
  } else {
    return aria2::getSparseMissingUnusedIndex
      (index, minSplitSize,
       array(ignoreBitfield)|array(bitfield_)|array(useBitfield_),
       useBitfield_, blockLength_, blocks_);
  }
}

namespace {
template<typename Array>
bool copyBitfield(unsigned char* dst, const Array& src, size_t blocks)
{
  unsigned char bits = 0;
  size_t len = (blocks+7)/8;
  for(size_t i = 0; i < len-1; ++i) {
    dst[i] = src[i];
    bits |= dst[i];
  }
  dst[len-1] = src[len-1]&bitfield::lastByteMask(blocks);
  bits |= dst[len-1];
  return bits != 0;
}
} // namespace

bool BitfieldMan::getAllMissingIndexes(unsigned char* misbitfield, size_t len)
  const
{
  assert(len == bitfieldLength_);
  if(filterEnabled_) {
    return copyBitfield
      (misbitfield, ~array(bitfield_)&array(filterBitfield_), blocks_);
  } else {
    return copyBitfield(misbitfield, ~array(bitfield_), blocks_);
  }
}

bool BitfieldMan::getAllMissingIndexes(unsigned char* misbitfield, size_t len,
                                       const unsigned char* peerBitfield,
                                       size_t peerBitfieldLength) const
{
  assert(len == bitfieldLength_);
  if(bitfieldLength_ != peerBitfieldLength) {
    return false;
  }
  if(filterEnabled_) {
    return copyBitfield
      (misbitfield,
       ~array(bitfield_)&array(peerBitfield)&array(filterBitfield_),
       blocks_);
  } else {
    return copyBitfield
      (misbitfield, ~array(bitfield_)&array(peerBitfield),
       blocks_);
  }
}

bool BitfieldMan::getAllMissingUnusedIndexes(unsigned char* misbitfield,
                                             size_t len,
                                             const unsigned char* peerBitfield,
                                             size_t peerBitfieldLength) const
{
  assert(len == bitfieldLength_);
  if(bitfieldLength_ != peerBitfieldLength) {
    return false;
  }
  if(filterEnabled_) {
    return copyBitfield
      (misbitfield,
       ~array(bitfield_)&~array(useBitfield_)&array(peerBitfield)&
       array(filterBitfield_),
       blocks_);
  } else {
    return copyBitfield
      (misbitfield,
       ~array(bitfield_)&~array(useBitfield_)&array(peerBitfield),
       blocks_);
  }
}

size_t BitfieldMan::countMissingBlock() const {
  return cachedNumMissingBlock_;
}

size_t BitfieldMan::countMissingBlockNow() const {
  if(filterEnabled_) {
    array_ptr<unsigned char> temp(new unsigned char[bitfieldLength_]);
    for(size_t i = 0; i < bitfieldLength_; ++i) {
      temp[i] = bitfield_[i]&filterBitfield_[i];
    }
    size_t count =  bitfield::countSetBit(filterBitfield_, blocks_)-
      bitfield::countSetBit(temp, blocks_);
    return count;
  } else {
    return blocks_-bitfield::countSetBit(bitfield_, blocks_);
  }
}

size_t BitfieldMan::countFilteredBlockNow() const {
  if(filterEnabled_) {
    return bitfield::countSetBit(filterBitfield_, blocks_);
  } else {
    return 0;
  }
}

bool BitfieldMan::setBitInternal(unsigned char* bitfield, size_t index, bool on) {
  if(blocks_ <= index) { return false; }
  unsigned char mask = 128 >> (index%8);
  if(on) {
    bitfield[index/8] |= mask;
  } else {
    bitfield[index/8] &= ~mask;
  }
  return true;
}

bool BitfieldMan::setUseBit(size_t index) {
  return setBitInternal(useBitfield_, index, true);
}

bool BitfieldMan::unsetUseBit(size_t index) {
  return setBitInternal(useBitfield_, index, false);
}

bool BitfieldMan::setBit(size_t index) {
  bool b = setBitInternal(bitfield_, index, true);
  updateCache();
  return b;
}

bool BitfieldMan::unsetBit(size_t index) {
  bool b = setBitInternal(bitfield_, index, false);
  updateCache();
  return b;
}

bool BitfieldMan::isFilteredAllBitSet() const {
  if(filterEnabled_) {
    for(size_t i = 0; i < bitfieldLength_; ++i) {
      if((bitfield_[i]&filterBitfield_[i]) != filterBitfield_[i]) {
        return false;
      }
    }
    return true;
  } else {
    return isAllBitSet();
  }
}

namespace {
bool testAllBitSet
(const unsigned char* bitfield, size_t length, size_t blocks)
{
  if(length == 0) {
    return true;
  }
  for(size_t i = 0; i < length-1; ++i) {
    if(bitfield[i] != 0xffu) {
      return false;
    }
  }
  return bitfield[length-1] == bitfield::lastByteMask(blocks);
}
} // namespace

bool BitfieldMan::isAllBitSet() const
{
  return testAllBitSet(bitfield_, bitfieldLength_, blocks_);
}

bool BitfieldMan::isAllFilterBitSet() const
{
  if(!filterBitfield_) {
    return false;
  }
  return testAllBitSet(filterBitfield_, bitfieldLength_, blocks_);
}

bool BitfieldMan::isBitSet(size_t index) const
{
  return bitfield::test(bitfield_, blocks_, index);
}

bool BitfieldMan::isUseBitSet(size_t index) const
{
  return bitfield::test(useBitfield_, blocks_, index);
}

void BitfieldMan::setBitfield(const unsigned char* bitfield, size_t bitfieldLength) {
  if(bitfieldLength_ != bitfieldLength) {
    return;
  }
  memcpy(bitfield_, bitfield, bitfieldLength_);
  memset(useBitfield_, 0, bitfieldLength_);
  updateCache();
}

void BitfieldMan::clearAllBit() {
  memset(bitfield_, 0, bitfieldLength_);
  updateCache();
}

void BitfieldMan::setAllBit() {
  for(size_t i = 0; i < blocks_; ++i) {
    setBitInternal(bitfield_, i, true);
  }
  updateCache();
}

void BitfieldMan::clearAllUseBit() {
  memset(useBitfield_, 0, bitfieldLength_);
  updateCache();
}

void BitfieldMan::setAllUseBit() {
  for(size_t i = 0; i < blocks_; ++i) {
    setBitInternal(useBitfield_, i, true);
  }
}

bool BitfieldMan::setFilterBit(size_t index) {
  return setBitInternal(filterBitfield_, index, true);
}

void BitfieldMan::ensureFilterBitfield()
{
  if(!filterBitfield_) {
    filterBitfield_ = new unsigned char[bitfieldLength_];
    memset(filterBitfield_, 0, bitfieldLength_);
  }
}

void BitfieldMan::addFilter(uint64_t offset, uint64_t length) {
  ensureFilterBitfield();
  if(length > 0) {
    size_t startBlock = offset/blockLength_;
    size_t endBlock = (offset+length-1)/blockLength_;
    for(size_t i = startBlock; i <= endBlock && i < blocks_; i++) {
      setFilterBit(i);
    }
  }
  updateCache();
}

void BitfieldMan::removeFilter(uint64_t offset, uint64_t length) {
  ensureFilterBitfield();
  if(length > 0) {
    size_t startBlock = offset/blockLength_;
    size_t endBlock = (offset+length-1)/blockLength_;
    for(size_t i = startBlock; i <= endBlock && i < blocks_; i++) {
      setBitInternal(filterBitfield_, i, false);
    }
  }
  updateCache();
}

void BitfieldMan::addNotFilter(uint64_t offset, uint64_t length)
{
  ensureFilterBitfield();
  if(length > 0 && blocks_ > 0) {
    size_t startBlock = offset/blockLength_;
    if(blocks_ <= startBlock) {
      startBlock = blocks_;
    }
    size_t endBlock = (offset+length-1)/blockLength_;
    for(size_t i = 0; i < startBlock; ++i) {
      setFilterBit(i);
    }
    for(size_t i = endBlock+1; i < blocks_; ++i) {
      setFilterBit(i);
    }
  }
  updateCache();
}

void BitfieldMan::enableFilter() {
  ensureFilterBitfield();
  filterEnabled_ = true;
  updateCache();
}

void BitfieldMan::disableFilter() {
  filterEnabled_ = false;
  updateCache();
}

void BitfieldMan::clearFilter() {
  if(filterBitfield_) {
    delete [] filterBitfield_;
    filterBitfield_ = 0;
  }
  filterEnabled_ = false;
  updateCache();
}

uint64_t BitfieldMan::getFilteredTotalLengthNow() const {
  if(!filterBitfield_) {
    return 0;
  }
  size_t filteredBlocks = bitfield::countSetBit(filterBitfield_, blocks_);
  if(filteredBlocks == 0) {
    return 0;
  }
  if(bitfield::test(filterBitfield_, blocks_, blocks_-1)) {
    return ((uint64_t)filteredBlocks-1)*blockLength_+getLastBlockLength();
  } else {
    return ((uint64_t)filteredBlocks)*blockLength_;
  }
}

uint64_t BitfieldMan::getCompletedLength(bool useFilter) const {
  unsigned char* temp;
  if(useFilter) {
    temp = new unsigned char[bitfieldLength_];
    for(size_t i = 0; i < bitfieldLength_; ++i) {
      temp[i] = bitfield_[i];
      if(filterEnabled_) {
        temp[i] &= filterBitfield_[i];
      }
    }
  } else {
    temp = bitfield_;
  }
  size_t completedBlocks = bitfield::countSetBit(temp, blocks_);
  uint64_t completedLength = 0;
  if(completedBlocks == 0) {
    completedLength = 0;
  } else {
    if(bitfield::test(temp, blocks_, blocks_-1)) {
      completedLength = ((uint64_t)completedBlocks-1)*blockLength_+getLastBlockLength();
    } else {
      completedLength = ((uint64_t)completedBlocks)*blockLength_;
    }
  }
  if(useFilter) {
    delete [] temp;
  }
  return completedLength;
}

uint64_t BitfieldMan::getCompletedLengthNow() const {
  return getCompletedLength(false);
}

uint64_t BitfieldMan::getFilteredCompletedLengthNow() const {
  return getCompletedLength(true);
}

void BitfieldMan::updateCache()
{
  cachedNumMissingBlock_ = countMissingBlockNow();
  cachedNumFilteredBlock_ = countFilteredBlockNow();
  cachedFilteredTotalLength_ = getFilteredTotalLengthNow();
  cachedCompletedLength_ = getCompletedLengthNow();
  cachedFilteredCompletedLength_ = getFilteredCompletedLengthNow();
}

bool BitfieldMan::isBitRangeSet(size_t startIndex, size_t endIndex) const
{
  for(size_t i =  startIndex; i <= endIndex; ++i) {
    if(!isBitSet(i)) {
      return false;
    }
  }
  return true;
}

void BitfieldMan::unsetBitRange(size_t startIndex, size_t endIndex)
{
  for(size_t i = startIndex; i <= endIndex; ++i) {
    unsetBit(i);
  }
  updateCache();
}

void BitfieldMan::setBitRange(size_t startIndex, size_t endIndex)
{
  for(size_t i = startIndex; i <= endIndex; ++i) {
    setBit(i);
  }
  updateCache();
}

bool BitfieldMan::isBitSetOffsetRange(uint64_t offset, uint64_t length) const
{
  if(length <= 0) {
    return false;
  }
  if(totalLength_ <= offset) {
    return false;
  }
  if(totalLength_ < offset+length) {
    length = totalLength_-offset;
  }
  size_t startBlock = offset/blockLength_;
  size_t endBlock = (offset+length-1)/blockLength_;
  for(size_t i = startBlock; i <= endBlock; i++) {
    if(!isBitSet(i)) {
      return false;
    }
  }
  return true;
}

uint64_t BitfieldMan::getMissingUnusedLength(size_t startingIndex) const
{
  if(startingIndex < 0 || blocks_ <= startingIndex) {
    return 0;
  }
  uint64_t length = 0;
  for(size_t i = startingIndex; i < blocks_; ++i) {
    if(isBitSet(i) || isUseBitSet(i)) {
      break;
    }
    length += getBlockLength(i);
  }
  return length;
}

BitfieldMan::Range::Range(size_t startIndex, size_t endIndex)
 :
  startIndex(startIndex),
  endIndex(endIndex)
{}
  
size_t BitfieldMan::Range::getSize() const
{
  return endIndex-startIndex;
}

size_t BitfieldMan::Range::getMidIndex() const
{
  return (endIndex-startIndex)/2+startIndex;
}

bool BitfieldMan::Range::operator<(const Range& range) const
{
  return getSize() < range.getSize();
}
    
bool BitfieldMan::Range::operator==(const Range& range) const
{
  return getSize() == range.getSize();
}

} // namespace aria2
