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
#ifndef D_DISK_ADAPTOR_H
#define D_DISK_ADAPTOR_H

#include "BinaryStream.h"

#include <string>
#include <vector>

#include "TimeA2.h"

namespace aria2 {

class FileEntry;
class FileAllocationIterator;

class DiskAdaptor:public BinaryStream {
private:
  std::vector<SharedHandle<FileEntry> > fileEntries_;

  bool fallocate_;
public:
  DiskAdaptor();
  virtual ~DiskAdaptor();

  virtual void openFile() = 0;

  virtual void closeFile() = 0;

  virtual void openExistingFile() = 0;

  virtual void initAndOpenFile() = 0;

  virtual bool fileExists() = 0;

  virtual uint64_t size() = 0;

  template<typename InputIterator>
  void setFileEntries(InputIterator first, InputIterator last)
  {
    fileEntries_.assign(first, last);
  }

  const std::vector<SharedHandle<FileEntry> >& getFileEntries() const
  {
    return fileEntries_;
  }

  virtual SharedHandle<FileAllocationIterator> fileAllocationIterator() = 0;

  virtual void enableDirectIO() {}

  virtual void disableDirectIO() {}

  virtual void enableReadOnly() {}

  virtual void disableReadOnly() {}

  virtual bool isReadOnlyEnabled() const { return false; }

  // Assumed each file length is stored in fileEntries or DiskAdaptor knows it.
  // If each actual file's length is larger than that, truncate file to that
  // length.
  // Call one of openFile/openExistingFile/initAndOpenFile before calling this
  // function.
  virtual void cutTrailingGarbage() = 0;

  // Returns the number of files, the actime and modtime of which are
  // successfully changed.
  virtual size_t utime(const Time& actime, const Time& modtime) = 0;

  void enableFallocate()
  {
    fallocate_ = true;
  }

  void disableFallocate()
  {
    fallocate_ = false;
  }

  bool doesFallocate() const
  {
    return fallocate_;
  }
};

typedef SharedHandle<DiskAdaptor> DiskAdaptorHandle;

} // namespace aria2

#endif // D_DISK_ADAPTOR_H
