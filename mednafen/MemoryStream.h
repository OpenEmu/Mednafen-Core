/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* MemoryStream.h:
**  Copyright (C) 2012-2021 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 Notes:
	For performance reasons(like in the state rewinding code), we should try to make sure map()
	returns a pointer that is aligned to at least what malloc()/realloc() provides.
	(And maybe forcefully align it to at least 16 bytes in the future)
*/

#ifndef __MDFN_MEMORYSTREAM_H
#define __MDFN_MEMORYSTREAM_H

#include "Stream.h"

namespace Mednafen
{

class MemoryStream : public Stream
{
 public:

 MemoryStream();
 MemoryStream(uint64 alloc_hint, int alloc_hint_is_size = false);	// Pass -1 instead of 1 for alloc_hint_is_size to skip initialization of the memory.
 MemoryStream(Stream *stream, uint64 size_limit = (uint64)-1);
				// Will create a MemoryStream equivalent of the contents of "stream", and then "delete stream".
				// Will only work if stream->tell() == 0, or if "stream" is seekable.
				// stream will be deleted even if this constructor throws.
				//
				// Will throw an exception if the initial size() of the MemoryStream would be greater than size_limit(useful for when passing
				// in GZFileStream streams).

 MemoryStream(const MemoryStream &zs);
 MemoryStream & operator=(const MemoryStream &zs);

 virtual ~MemoryStream() override;

 virtual uint64 attributes(void) override;

 virtual uint8 *map(void) noexcept override;
 virtual uint64 map_size(void) noexcept override;
 virtual void unmap(void) noexcept override;

 virtual uint64 read(void *data, uint64 count, bool error_on_eos = true) override;
 virtual void write(const void *data, uint64 count) override;
 virtual void truncate(uint64 length) override;
 virtual void seek(int64 offset, int whence) override;
 virtual uint64 tell(void) override;
 virtual uint64 size(void) override;
 virtual void flush(void) override;
 virtual void close(void) override;

 virtual int get_line(std::string &str) override;

 // 'ls' points to the first character in the line, 'lb' points to one after the last character in the line.
 // ls == lb for empty lines, so be sure to handle that case.
 //
 // Unlike Stream::get_line(), will treat a line terminated with <CR><LF> as one line and return the value 0x0D0A.
 //
 // Using the returned pointers after calling any functions on the MemoryStream object, including the destructor,
 // will result in undefined behavior.
 //
 // The memory pointed to by 'ls' may be read and/or written to up to but not including 'lb', as 'lb' may
 // point one past the end of the allocated memory in the case of a line terminated by the end being reached.
 // Since the pointers are directly to the underlying allocated memory in the MemoryStream object, writing
 // will modify the state of the MemoryStream object.
 INLINE int get_line_mem(char** ls, const char** lb)
 {
  char* mp = (char*)data_buffer + std::min<uint64>(data_buffer_size, position);
  char* sp = mp;
  char* const bp = (char*)data_buffer + data_buffer_size;
  const uint32 tsv = (1U << (31 - '\r')) | (1U << (31 - '\n')) | (1U << (31 - 0));

  while(mp != bp)
  {
   const uint8 c = *mp;

   if(c >= 0x10)
   {
    mp++;
    continue;
   }

   if((int32)(tsv << c) < 0)
   {
    int ret = *mp;

    *ls = sp;
    *lb = mp;
    mp++;

    if(mp != bp && ret == '\r' && *mp == '\n')
    {
     ret = (ret << 8) | *mp;
     mp++;
    }

    position = mp - (char*)data_buffer;

    return ret;
   }
   mp++;
  }

  position = data_buffer_size;

  if(mp != sp)
  {
   *ls = sp;
   *lb = mp;

   return 256;
  }

  return -1;
 }

 void shrink_to_fit(void) noexcept;	// Minimizes alloced memory.

 void mswin_utf8_convert_kludge(void);

 // No methods on the object may be called externally(other than the destructor) after steal_malloced_ptr()
 INLINE void* steal_malloced_ptr(void)
 {
  void* ret = data_buffer;

  data_buffer = nullptr;
  data_buffer_size = 0;
  data_buffer_alloced = 0;
  position = 0;

  return ret;
 }

 private:
 uint8 *data_buffer;
 uint64 data_buffer_size;
 uint64 data_buffer_alloced;

 uint64 position;

 void grow_if_necessary(uint64 new_required_size, uint64 hole_end);
};

}
#endif
