/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* Stream.h:
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

// TODO?: BufferedStream, no virtual functions, yes inline functions, constructor takes a Stream* argument.

#ifndef __MDFN_STREAM_H
#define __MDFN_STREAM_H

#include <mednafen/types.h>

namespace Mednafen
{
/*
 The data read into the pointer passed to read*() functions should be considered undefined if the function throws
 or propagates an exception.
*/

class Stream
{
 public:

 Stream();
 virtual ~Stream();

 enum
 {
  ATTRIBUTE_READABLE = 	1U <<  0,
  ATTRIBUTE_WRITEABLE =	1U <<  1,
  ATTRIBUTE_SEEKABLE =	1U <<  2,	// Indicates that Stream is capable of being seeked, regardless of how performant seeking is.

  ATTRIBUTE_SLOW_SEEK =	1U <<  3,	// Indicates that seeking(particularly backwards) is slow, and should be avoided if at all possible.
  ATTRIBUTE_SLOW_SIZE =	1U <<  4,	// Indicates that size() is slow, and should be avoided if at all possible.  May be cleared
					// after a successful call to size() if the class caches the determined size.

  ATTRIBUTE_INMEM_FAST = 1U << 5,	// Indicates the stream's underlying data is in memory or synthesizable from data in memory with low computational complexity,
					// and that reads and seeks are both very fast.
 };
 virtual uint64 attributes(void) = 0;

 //
 // Throw an exception if stream is not fast-seekable; exists to allow for class-specific generic but helpful
 // error messages(such as perhaps "MeowZip file is missing a seek index.").
 //
 virtual void require_fast_seekable(void);

 virtual uint8 *map(void) noexcept;
				// Map the entirety of the stream data into the address space of the process, if possible, and return a pointer.
				// (the returned pointer must be cached, and returned on any subsequent calls to map() without an unmap()
				// in-between, to facilitate a sort of "feature-testing", to determine if an alternative like "MemoryStream"
				// should be used).
				//
				// If the mapping fails for whatever reason, return NULL rather than throwing an exception.
				//
				// For code using this functionality, ensure usage of map_size() instead of size(), unless you're only using a specific derived
				// class like MemoryStream() where the value returned by size() won't change unexpectedly due to outside factors.

 virtual uint64 map_size(void) noexcept;
				// The size of the memory mapping area, point to which returned by map().
				//
				// Returns 0 on supported, or if no mapping currently exists.

 virtual void unmap(void) noexcept;
				// Unmap the stream data from the address space.  (Possibly invalidating the pointer returned from map()).
				// (must automatically be called, if necessary, from the destructor).
				//
				// If the data can't be "unmapped" as such because it was never mmap()'d or similar in the first place(such as with MemoryStream),
				// then this will be a nop.

 virtual uint64 read(void *data, uint64 count, bool error_on_eos = true) = 0;
 virtual void write(const void *data, uint64 count) = 0;

 virtual void truncate(uint64 length) = 0;	// Should have ftruncate()-like semantics; but avoid using it to extend files.

 virtual void seek(int64 offset, int whence = SEEK_SET) = 0;
 inline void rewind(void)
 {
  seek(0, SEEK_SET);
 }
 virtual uint64 tell(void) = 0;
 virtual uint64 size(void) = 0;	// May implicitly call flush() if the stream is writeable.
 virtual void flush(void) = 0;
 virtual void close(void) = 0;	// Flushes(in the case of writeable streams) and closes the stream.
				// Necessary since this operation can fail(running out of disk space, for instance),
				// and throw an exception in the destructor would be a Bad Idea(TM).
				//
				// Manually calling this function isn't strictly necessary, but recommended when the
				// stream is writeable; it will be called automatically from the destructor, with any
				// exceptions thrown caught and logged.

 //
 // Utility functions(TODO):
 //
 INLINE uint8 get_u8(void)
 {
  uint8 ret;

  read(&ret, sizeof(ret));

  return ret;
 }

 INLINE void put_u8(uint8 c)
 {
  write(&c, sizeof(c));
 }


 template<typename T>
 INLINE T get_NE(void)
 {
  T ret;

  read(&ret, sizeof(ret));

  return ret;
 }


 template<typename T>
 INLINE T get_RE(void)
 {
  uint8 tmp[sizeof(T)];
  union
  {
   T ret;
   uint8 ret_u8[sizeof(T)];
  };

  read(tmp, sizeof(tmp));

  for(unsigned i = 0; i < sizeof(T); i++)
   ret_u8[i] = tmp[sizeof(T) - 1 - i];

  return ret;
 }

 template<typename T>
 INLINE void put_NE(T c)
 {
  write(&c, sizeof(c));
 }

 template<typename T>
 INLINE void put_RE(T c)
 {
  uint8 tmp[sizeof(T)];

  for(unsigned i = 0; i < sizeof(T); i++)
   tmp[i] = ((uint8 *)&c)[sizeof(T) - 1 - i];

  write(tmp, sizeof(tmp));
 }

 template<typename T>
 INLINE T get_LE(void)
 {
  #ifdef LSB_FIRST
  return get_NE<T>();
  #else
  return get_RE<T>();
  #endif
 }

 template<typename T>
 INLINE void put_LE(T c)
 {
  #ifdef LSB_FIRST
  return put_NE<T>(c);
  #else
  return put_RE<T>(c);
  #endif
 }

 template<typename T>
 INLINE T get_BE(void)
 {
  #ifndef LSB_FIRST
  return get_NE<T>();
  #else
  return get_RE<T>();
  #endif
 }

 template<typename T>
 INLINE void put_BE(T c)
 {
  #ifndef LSB_FIRST
  return put_NE<T>(c);
  #else
  return put_RE<T>(c);
  #endif
 }

 INLINE void put_string(const char* str)
 {
  write(str, strlen(str));
 }

 uint64 get_string_append(std::string* str, uint64 count, bool error_on_eos = true);
 INLINE uint64 get_string(std::string* str, uint64 count, bool error_on_eos = true)
 {
  str->clear();
  return get_string_append(str, count, error_on_eos);
 }

 // Reads a line into "str", overwriting its contents; returns the line-end char('\n' or '\r' or '\0'), or 256 on EOF and
 // data has been read into "str", and -1 on EOF when no data has been read into "str".
 // The line-end char won't be added to "str".
 // It's up to the caller to handle extraneous empty lines caused by DOS-format text lines(\r\n).
 // ("str" is passed by reference for the possibility of improved performance by reusing alloced memory for the std::string, though part
 //  of it would be up to the STL implementation).
 // Implemented as virtual so that a higher-performance version can be implemented if possible(IE with MemoryStream)
 virtual int get_line(std::string &str);

 virtual void print_format(const char *format, ...) MDFN_FORMATSTR(gnu_printf, 2, 3);

 void put_line(const std::string& str);
 void put_line(const char* s);


#if 0
 int scanf(const char *format, ...) MDFN_FORMATSTR(gnu_scanf, 2, 3);
 void put_string(const char *str);
 void put_string(const std::string &str);
#endif

 bool read_utf8_bom(void);
 void write_utf8_bom(void);

 //
 // Read until end-of-stream(or count), discarding any read data, and returns the amount of data "read".
 //  (Useful for detecting and printing warnings about extra garbage data without needing to call size(),
 //   which can be problematic for some types of Streams).
 uint64 read_discard(uint64 count = (uint64)-1);

 //
 // Reads stream starting at the current stream position(as returned by tell()), into memory allocated with malloc() and realloc(), and
 // sets *data_out to a pointer to the memory(which the caller will need to free() at some point).
 //
 // *data_out is only an output.
 //
 // If size_limit is/will be exceeded, an exception will be thrown, and *data_out will not be written to.
 //
 // Will return the amount of data read.
 //
 // If the returned value is 0, *data_out will still be a valid non-NULL pointer.
 //
 uint64 alloc_and_read(void** data_out, uint64 size_limit = (uint64)-1);
};

//
//
//
/*
class StreamPosFilter final : public Stream
{
 public:
 StreamPosFilter(std::shared_ptr<Stream> s_);

 virtual uint64 read(void *data, uint64 count, bool error_on_eos = true) override;
 virtual void write(const void *data, uint64 count) override;
 virtual void seek(int64 offset, int whence) override;
 virtual uint64 tell(void) override;
 virtual uint64 size(void) override;
 virtual void close(void) override;
 virtual uint64 attributes(void) override;
 virtual void truncate(uint64 length) override;
 virtual void flush(void) override;

 private:

 uint64 pos;
 std::shared_ptr<Stream> s;
};
*/
}
#endif
