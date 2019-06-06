/* Mednafen - Multi-system Emulator
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mednafen.h"

#include <trio/trio.h>

#include "driver.h"
#include "state.h"
#include "general.h"
#include "video.h"
#include "netplay.h"
#include "movie.h"
#include "state.h"

#include "FileStream.h"

namespace Mednafen
{

enum
{
 MOVIE_STOPPED = 0,
 MOVIE_PLAYING = 1,
 MOVIE_RECORDING = 2
};

static unsigned ActiveMovieMode = MOVIE_STOPPED;
static int ActiveSlotNumber;	// Negative for no slot in use/fname specified directly.
static FileStream* ActiveMovieStream = NULL;

static int CurrentMovie = 0;
static int RecentlySavedMovie = -1;
static int MovieStatus[10];

static void HandleMovieError(const std::exception &e)
{
 if(ActiveMovieStream)
 {
  delete ActiveMovieStream;
  ActiveMovieStream = NULL;
 }

 if(ActiveSlotNumber >= 0)
 {
  MovieStatus[ActiveSlotNumber] = 0;

  if(ActiveMovieMode == MOVIE_PLAYING)
   MDFN_Notify(MDFN_NOTICE_ERROR, _("Movie %u playback failed."), ActiveSlotNumber);
  else
   MDFN_Notify(MDFN_NOTICE_ERROR, _("Movie %u recording failed."), ActiveSlotNumber);
 }

 MDFN_Notify(MDFN_NOTICE_ERROR, _("Movie error: %s"), e.what());
 ActiveMovieMode = MOVIE_STOPPED;
 ActiveSlotNumber = -1;
}

bool MDFNMOV_IsPlaying(void) noexcept
{
 return(ActiveMovieMode == MOVIE_PLAYING);
}

bool MDFNMOV_IsRecording(void) noexcept
{
 return(ActiveMovieMode == MOVIE_RECORDING);
}

void MDFNI_SaveMovie(char *fname, const MDFN_Surface *surface, const MDFN_Rect *DisplayRect, const int32 *LineWidths)
{
 try
 {
  if(!MDFNGameInfo->StateAction)
  {
   throw MDFN_Error(0, _("Module \"%s\" doesn't support save states."), MDFNGameInfo->shortname);
  }

  if(MDFNnetplay && (MDFNGameInfo->SaveStateAltersState == true))
  {
   throw MDFN_Error(0, _("Module %s is not compatible with manual movie save starting/stopping during netplay."), MDFNGameInfo->shortname);
  }

  if(ActiveMovieMode == MOVIE_PLAYING)	/* Can't interrupt playback.*/
  {
   throw MDFN_Error(0, _("Can't record movie during movie playback."));
  }

  if(ActiveMovieMode == MOVIE_RECORDING)	/* Stop saving. */
  {
   MDFNMOV_Stop();
   return;  
  }

  ActiveMovieMode = MOVIE_RECORDING;
  ActiveSlotNumber = fname ? -1 : CurrentMovie;

  ActiveMovieStream = new FileStream(fname ? std::string(fname) : MDFN_MakeFName(MDFNMKF_MOVIE, CurrentMovie, 0), FileStream::MODE_WRITE);

  //
  // Save save state first.
  //
  MDFNSS_SaveSM(ActiveMovieStream, false, surface, DisplayRect, LineWidths);
  ActiveMovieStream->flush(); 	    // Flush output so that previews will still work right while
			    	    // the movie is being recorded.

  MDFN_Notify(MDFN_NOTICE_STATUS, _("Movie recording started."));
  MovieStatus[ActiveSlotNumber] = 1;
  RecentlySavedMovie = ActiveSlotNumber;
 }
 catch(std::exception &e)
 {
  HandleMovieError(e);
 }
}

void MDFNMOV_Stop(void) noexcept
{
 const unsigned PAMM = ActiveMovieMode;

 if(ActiveMovieMode != MOVIE_STOPPED)
 {
  if(ActiveMovieMode == MOVIE_RECORDING)
  {
   MDFNMOV_RecordState();
   //MovieStatus[current - 1] = 1;
   //RecentlySavedMovie = current - 1;
  }

  if(ActiveMovieStream)
  {
   delete ActiveMovieStream;
   ActiveMovieStream = NULL;
  }

  ActiveMovieMode = MOVIE_STOPPED;
  ActiveSlotNumber = -1;

  if(PAMM == MOVIE_PLAYING)
   MDFN_Notify(MDFN_NOTICE_STATUS, _("Movie playback stopped."));
  else if(PAMM == MOVIE_RECORDING)
   MDFN_Notify(MDFN_NOTICE_STATUS, _("Movie recording stopped."));
 }
}

void MDFNI_LoadMovie(char *fname)
{
 try
 {
  if(!MDFNGameInfo->StateAction)
  {
   throw MDFN_Error(0, _("Module \"%s\" doesn't support save states."), MDFNGameInfo->shortname);
  }

  if(MDFNnetplay)
  {
   throw MDFN_Error(0, _("Can't play movies during netplay."));
  }

  if(ActiveMovieMode == MOVIE_RECORDING)	/* Can't interrupt recording. */
  {
   throw MDFN_Error(0, _("Can't play movie during movie recording."));
  }

  if(ActiveMovieMode == MOVIE_PLAYING)		/* Stop playback. */
  {
   MDFNMOV_Stop();
   return;  
  }

  ActiveMovieMode = MOVIE_PLAYING;

  if(fname)
   ActiveSlotNumber = -1;
  else
  {
   ActiveSlotNumber = CurrentMovie;
   MovieStatus[CurrentMovie] = 1;
  }

  ActiveMovieStream = new FileStream(fname ? std::string(fname) : MDFN_MakeFName(MDFNMKF_MOVIE, CurrentMovie, 0), FileStream::MODE_READ);

  //
  //
  //
  MDFNSS_LoadSM(ActiveMovieStream, false);

  MDFN_Notify(MDFN_NOTICE_STATUS, _("Movie playback started."));
 }
 catch(std::exception &e)
 {
  HandleMovieError(e);
 }
}

void MDFNMOV_ProcessInput(uint8 *PortData[], uint32 PortLen[], int NumPorts) noexcept
{
 try
 {
  if(ActiveMovieMode == MOVIE_STOPPED)
   return;	/* Not playback nor recording. */

  if(ActiveMovieMode == MOVIE_PLAYING)	/* Playback */
  {
   int t;

   while((t = ActiveMovieStream->get_char()) >= 0 && t)
   {
    if(t == MDFNNPCMD_LOADSTATE)
     MDFNSS_LoadSM(ActiveMovieStream, false);
    else if(t == MDFNNPCMD_SET_MEDIA)
    {
     uint8 buf[4 * 4];
     ActiveMovieStream->read(buf, sizeof(buf));
     MDFN_UntrustedSetMedia(MDFN_de32lsb(&buf[0]), MDFN_de32lsb(&buf[4]), MDFN_de32lsb(&buf[8]), MDFN_de32lsb(&buf[12]));
    }
    else
     MDFN_DoSimpleCommand(t);
   }

   if(t < 0)	// EOF
   {
    MDFNMOV_Stop();
    return; 
   }

   for(int p = 0; p < NumPorts; p++)
   {
    if(PortData[p])
     ActiveMovieStream->read(PortData[p], PortLen[p]);
   }
  }
  else			/* Recording */
  {
   ActiveMovieStream->put_u8(0);

   for(int p = 0; p < NumPorts; p++)
   {
    if(PortData[p])
     ActiveMovieStream->write(PortData[p], PortLen[p]);
   }
  }
 }
 catch(std::exception &e)
 {
  HandleMovieError(e);
 }
}

void MDFNMOV_AddCommand(uint8 cmd, uint32 data_len, uint8* data) noexcept
{
 // Return if not recording a movie
 if(ActiveMovieMode != MOVIE_RECORDING)
  return;

 try
 {
  ActiveMovieStream->put_u8(cmd);

  if(data_len > 0)
   ActiveMovieStream->write(data, data_len);
 }
 catch(std::exception &e)
 {
  HandleMovieError(e);
 }
}

void MDFNMOV_RecordState(void) noexcept
{
 try
 {
  ActiveMovieStream->put_u8(MDFNNPCMD_LOADSTATE);
  MDFNSS_SaveSM(ActiveMovieStream, false);
 }
 catch(std::exception &e)
 {
  HandleMovieError(e);
 }
}

void MDFNMOV_StateAction(StateMem* sm, const unsigned load)
{
 if(!ActiveMovieStream)
  return;

 uint64 fpos = ActiveMovieStream->tell();
 SFORMAT StateRegs[] =
 {
  SFVAR(fpos),
  SFEND
 };

 MDFNSS_StateAction(sm, load, true, StateRegs, "MEDNAFEN_MOVIE");

 if(load)
 {
  ActiveMovieStream->seek(fpos, SEEK_SET);

  if(ActiveMovieMode == MOVIE_RECORDING)
   ActiveMovieStream->truncate(fpos);
 }
}

void MDFNMOV_CheckMovies(void)
{
	int64 last_time = 0;

        if(!MDFNGameInfo->StateAction) 
         return;

	for(int ssel = 0; ssel < 10; ssel++)
        {
         MovieStatus[ssel] = false;

	 try
	 {
	  VirtualFS::FileInfo finfo;

	  NVFS.finfo(MDFN_MakeFName(MDFNMKF_MOVIE, ssel, 0), &finfo);
	  //
	  MovieStatus[ssel] = true;
	  if(finfo.mtime_us > last_time)
	  {
	   RecentlySavedMovie = ssel;
	   last_time = finfo.mtime_us;
 	  }
	 }
	 catch(...)
	 {

	 }
        }

        CurrentMovie = 0;
}

void MDFNI_SelectMovie(int w)
{
 if(w == -1)
 { 
  return; 
 }
 MDFNI_SelectState(-1);

 try
 {
  CurrentMovie = w;

  std::unique_ptr<StateStatusStruct> status(new StateStatusStruct());

  memset(status.get(), 0, sizeof(StateStatusStruct));
  memcpy(status->status, MovieStatus, 10 * sizeof(int));

  status->current = CurrentMovie;
  status->current_movie = 0;

  if(ActiveMovieMode == MOVIE_RECORDING)
   status->current_movie = 1 + ActiveSlotNumber;
  else if(ActiveMovieMode == MOVIE_PLAYING)
   status->current_movie = -1 - ActiveSlotNumber;

  status->recently_saved = RecentlySavedMovie;

  MDFNSS_GetStateInfo(MDFN_MakeFName(MDFNMKF_MOVIE, CurrentMovie, NULL), status.get());
  MDFND_SetMovieStatus(status.release());
 }
 catch(std::exception& e)
 {
  MDFN_Notify(MDFN_NOTICE_WARNING, "%s", e.what());
  MDFND_SetMovieStatus(NULL);
 }
}

}
