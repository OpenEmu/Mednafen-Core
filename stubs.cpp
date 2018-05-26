#include "mednafen/types.h"
#include "mednafen/git.h"
#include "mednafen/mednafen.h"
#include "mednafen/mednafen-driver.h"
#include "thread.h"

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <dispatch/dispatch.h>

void MDFND_Sleep(unsigned int time)
{
    usleep(time * 1000);
}

void MDFND_DispMessage(char *str)
{
    //std::cerr << str;
}

void MDFND_Message(const char *str)
{
    std::cerr << str;
}

void MDFND_MidSync(const EmulateSpecStruct *)
{}

void MDFND_PrintError(const char* err)
{
    std::cerr << err;
}

void MDFND_OutputNotice(MDFN_NoticeType t, const char* s) noexcept
{}

void MDFND_MediaSetNotification(uint32 drive_idx, uint32 state_idx, uint32 media_idx, uint32 orientation_idx)
{}

void MDFND_NetplayText(const char* text, bool NetEcho)
{}

MDFN_Thread *MDFND_CreateThread(int (*fn)(void *), void *data)
{
    return (MDFN_Thread*)sthread_create((void (*)(void*))fn, data);
}

void MDFND_SetMovieStatus(StateStatusStruct *) {}
void MDFND_SetStateStatus(StateStatusStruct *) {}

void MDFND_WaitThread(MDFN_Thread *thr, int *val)
{
    sthread_join((sthread_t*)thr);

    if(val)
    {
        *val = 0;
        std::cerr << "WaitThread relies on return value." << std::endl;
    }
}

void MDFND_KillThread(MDFN_Thread *)
{
    std::cerr << "Killing a thread is a BAD IDEA!" << std::endl;
}

MDFN_Mutex *MDFND_CreateMutex()
{
    return (MDFN_Mutex*)slock_new();
}

void MDFND_DestroyMutex(MDFN_Mutex *lock)
{
    slock_free((slock_t*)lock);
}

int MDFND_LockMutex(MDFN_Mutex *lock)
{
    slock_lock((slock_t*)lock);
    return 0;
}

int MDFND_UnlockMutex(MDFN_Mutex *lock)
{
    slock_unlock((slock_t*)lock);
    return 0;
}

MDFN_Cond* MDFND_CreateCond(void)
{
    return (MDFN_Cond*)scond_new();
}

void MDFND_DestroyCond(MDFN_Cond* cond)
{
    scond_free((scond_t*)cond);
}

int MDFND_SignalCond(MDFN_Cond* cond)
{
    scond_signal((scond_t*)cond);
    return 0;
}

int MDFND_WaitCond(MDFN_Cond* cond, MDFN_Mutex* mutex)
{
    scond_wait((scond_t*)cond, (slock_t*)mutex);
    return 0;
}

MDFN_Sem* MDFND_CreateSem(void)
{
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    return (MDFN_Sem*)sem;
}

void MDFND_DestroySem(MDFN_Sem* sem)
{
    dispatch_release((dispatch_object_t)sem);
}

int MDFND_PostSem(MDFN_Sem* sem)
{
    dispatch_semaphore_signal((dispatch_semaphore_t)sem);
    return 0;
}

int MDFND_WaitSem(MDFN_Sem* sem)
{
    dispatch_semaphore_wait((dispatch_semaphore_t)sem, DISPATCH_TIME_FOREVER);
    return 0;
}

int MDFND_WaitSemTimeout(MDFN_Sem* sem, unsigned ms)
{
    dispatch_time_t waitTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(ms * NSEC_PER_MSEC));
    dispatch_semaphore_wait((dispatch_semaphore_t)sem, waitTime);
    return 0;
}

void MDFND_SendData(const void*, uint32) {}
void MDFND_RecvData(void *, uint32) {}
void MDFND_NetplayText(const uint8*, bool) {}
void MDFND_NetworkClose() {}
void MDFND_NetplaySetHints(bool active, bool behind, uint32 local_players_mask)
{}
void MDFND_OutputInfo(const char *s) noexcept
{}
int MDFND_NetworkConnect() { return 0; }
bool MDFND_CheckNeedExit(void) { return false; }

uint32 MDFND_GetTime()
{
    static bool first = true;
    static uint32_t start_ms;

    struct timeval val;
    gettimeofday(&val, NULL);
    uint32_t ms = val.tv_sec * 1000 + val.tv_usec / 1000;

    if(first)
    {
        start_ms = ms;
        first = false;
    }
    
    return ms - start_ms;
}
