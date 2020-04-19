#include "driver.h"

void Mednafen::MDFND_OutputNotice(MDFN_NoticeType t, const char* s) noexcept {}

void Mednafen::MDFND_OutputInfo(const char *s) noexcept {}

void Mednafen::MDFND_MidSync(EmulateSpecStruct *espec, const unsigned flags) {}

void Mednafen::MDFND_MediaSetNotification(uint32 drive_idx, uint32 state_idx, uint32 media_idx, uint32 orientation_idx) {}

bool Mednafen::MDFND_CheckNeedExit(void) { return false; }

void Mednafen::MDFND_NetplayText(const char* text, bool NetEcho) {}
void Mednafen::MDFND_NetplaySetHints(bool active, bool behind, uint32 local_players_mask) {}

void Mednafen::MDFND_SetStateStatus(StateStatusStruct *status) noexcept {}
