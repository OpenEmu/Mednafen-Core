/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* SSFLoader.h:
**  Copyright (C) 2015-2016 Mednafen Team
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

#ifndef __MDFN_SSFLOADER_H
#define __MDFN_SSFLOADER_H

#include <mednafen/PSFLoader.h>
#include <mednafen/MemoryStream.h>

namespace Mednafen
{

class SSFLoader : public PSFLoader
{
 public:

 SSFLoader(VirtualFS* vfs, const std::string& dir_path, Stream* fp);
 virtual ~SSFLoader();

 static bool TestMagic(Stream* fp);

 virtual void HandleEXE(Stream* fp, bool ignore_pcsp = false) override;

 PSFTags tags;

 MemoryStream RAM_Data;
};

}
#endif
