/*
 Copyright (c) 2013, OpenEmu Team


 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of the OpenEmu Team nor the
       names of its contributors may be used to endorse or promote products
       derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "MednafenGameCore.h"
#import <OpenEmuBase/OERingBuffer.h>
#import "OELynxSystemResponderClient.h"
#import "OEPCESystemResponderClient.h"
#import "OEPCFXSystemResponderClient.h"
#import "OEPSXSystemResponderClient.h"
#import "OEVBSystemResponderClient.h"
#import "OEWSSystemResponderClient.h"

#include "mednafen/mednafen-types.h"
#include "mednafen/mednafen.h"
#include "mednafen/git.h"
#include "mednafen/general.h"

static MDFNGI *game;
static MDFN_Surface *surf;

enum systemTypes{ lynx, pce, pcfx, psx, vb, wswan };

@interface MednafenGameCore () <OELynxSystemResponderClient, OEPCESystemResponderClient, OEPCFXSystemResponderClient, OEPSXSystemResponderClient, OEVBSystemResponderClient, OEWSSystemResponderClient>
{
    int systemType;
    uint32_t *videoBuffer;
    int videoWidth, videoHeight;
    int16_t pad[2][16];
    NSString *romName;
    double sampleRate;
    
    NSString *mednafenCoreModule;
    NSTimeInterval mednafenCoreTiming;
    int mednafenCoreFBWidth;
    int mednafenCoreFBHeight;
    OEIntSize mednafenCoreAspect;
}

@end

MednafenGameCore *current;
@implementation MednafenGameCore

NSUInteger PSXEmulatorValues[] = { OEPSXButtonUp, OEPSXButtonDown, OEPSXButtonLeft, OEPSXButtonRight, OEPSXButtonTriangle, OEPSXButtonCircle, OEPSXButtonCross, OEPSXButtonSquare, OEPSXButtonL1, OEPSXButtonL2, OEPSXButtonL3, OEPSXButtonR1, OEPSXButtonR2, OEPSXButtonR3, OEPSXButtonStart, OEPSXButtonSelect };

- (oneway void)didPushPSXButton:(OEPSXButton)button forPlayer:(NSUInteger)player;
{
    pad[player-1][PSXEmulatorValues[button]] = 1;
}

- (oneway void)didReleasePSXButton:(OEPSXButton)button forPlayer:(NSUInteger)player;
{
    pad[player-1][PSXEmulatorValues[button]] = 0;
}

static int16_t input_state_callback(unsigned port, unsigned device, unsigned index, unsigned id)
{
    //NSLog(@"polled input: port: %d device: %d id: %d", port, device, id);
    
    if (port == 0 & device == 1) {
        return current->pad[0][id];
    }
    else if(port == 1 & device == 1) {
        return current->pad[1][id];
    }
    
    return 0;
}

static void update_input()
{
    union
    {
        uint32_t u32[2][1 + 8];
        uint8_t u8[2][2 * sizeof(uint16_t) + 8 * sizeof(uint32_t)];
    } static buf;
    
    uint16_t input_buf[2] = {0};
    static unsigned map[] = {
        OEPSXButtonSelect,
        OEPSXButtonL3,
        OEPSXButtonR3,
        OEPSXButtonStart,
        OEPSXButtonUp,
        OEPSXButtonRight,
        OEPSXButtonDown,
        OEPSXButtonLeft,
        OEPSXButtonL2,
        OEPSXButtonR2,
        OEPSXButtonL1,
        OEPSXButtonR1,
        OEPSXButtonTriangle,
        OEPSXButtonCircle,
        OEPSXButtonCross,
        OEPSXButtonSquare,
    };
    
    for (unsigned j = 0; j < 2; j++)
    {
        for (unsigned i = 0; i < 16; i++)
            input_buf[j] |= input_state_callback(j, 1, 0, map[i]) ? (1 << i) : 0;
    }
    
    // Buttons.
    buf.u8[0][0] = (input_buf[0] >> 0) & 0xff;
    buf.u8[0][1] = (input_buf[0] >> 8) & 0xff;
    buf.u8[1][0] = (input_buf[1] >> 0) & 0xff;
    buf.u8[1][1] = (input_buf[1] >> 8) & 0xff;
    
    // Analogs
    for (unsigned j = 0; j < 2; j++)
    {
        int analog_left_x = input_state_callback(j, 5, 0,
                                           0);
        
        int analog_left_y = input_state_callback(j, 5, 0,
                                           1);
        
        int analog_right_x = input_state_callback(j, 5, 1,
                                            0);
        
        int analog_right_y = input_state_callback(j, 5, 1,
                                            1);
        
        uint32_t r_right = analog_right_x > 0 ?  analog_right_x : 0;
        uint32_t r_left  = analog_right_x < 0 ? -analog_right_x : 0;
        uint32_t r_down  = analog_right_y > 0 ?  analog_right_y : 0;
        uint32_t r_up    = analog_right_y < 0 ? -analog_right_y : 0;
        
        uint32_t l_right = analog_left_x > 0 ?  analog_left_x : 0;
        uint32_t l_left  = analog_left_x < 0 ? -analog_left_x : 0;
        uint32_t l_down  = analog_left_y > 0 ?  analog_left_y : 0;
        uint32_t l_up    = analog_left_y < 0 ? -analog_left_y : 0;
        
        buf.u32[j][1] = r_right;
        buf.u32[j][2] = r_left;
        buf.u32[j][3] = r_down;
        buf.u32[j][4] = r_up;
        
        buf.u32[j][5] = l_right;
        buf.u32[j][6] = l_left;
        buf.u32[j][7] = l_down;
        buf.u32[j][8] = l_up;
    }
    
    game->SetInput(0, "gamepad", &buf.u8[0]);
    game->SetInput(1, "gamepad", &buf.u8[1]);
}

static void update_video(const void *data, unsigned width, unsigned height, size_t pitch)
{
    current->videoWidth  = width;
    current->videoHeight = height;
    //NSLog(@"width: %u height: %u pitch: %zu", width, height, pitch);
    
    dispatch_queue_t the_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    dispatch_apply(height, the_queue, ^(size_t y){
        const uint32_t *src = (uint32_t*)data + y * (pitch >> 2); //pitch is in bytes not pixels
        uint32_t *dst = current->videoBuffer + y * current->mednafenCoreFBWidth;
        
        memcpy(dst, src, sizeof(uint32_t)*width);
    });
}

static size_t update_audio_batch(const int16_t *data, size_t frames){
    [[current ringBufferAtIndex:0] write:data maxLength:frames << 2];
    return frames;
}

static void mednafen_init()
{
    MDFN_PixelFormat pix_fmt(MDFN_COLORSPACE_RGB, 16, 8, 0, 24);
    //memset(&last_pixel_format, 0, sizeof(MDFN_PixelFormat));
    surf = new MDFN_Surface(current->videoBuffer, current->mednafenCoreFBWidth, current->mednafenCoreFBHeight, current->mednafenCoreFBWidth, pix_fmt);
    
    std::vector<MDFNGI*> ext;
    MDFNI_InitializeModules(ext);
    
    std::vector<MDFNSetting> settings;
    
    NSString *batterySavesDirectory = current.batterySavesDirectoryPath;
    NSString *biosPath = [NSString pathWithComponents:@[
                          [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject],
                          @"OpenEmu", @"BIOS"]];
    
    MDFNSetting pce_setting = { "pce.cdbios", MDFNSF_EMU_STATE, "PCE CD BIOS", NULL, MDFNST_STRING, [[[biosPath stringByAppendingPathComponent:@"syscard3"] stringByAppendingPathExtension:@"pce"] UTF8String] };
    
    MDFNSetting pcfx_setting = { "pcfx.bios", MDFNSF_EMU_STATE, "PCFX BIOS", NULL, MDFNST_STRING, [[[biosPath stringByAppendingPathComponent:@"pcfx"] stringByAppendingPathExtension:@"rom"] UTF8String] };
    
    MDFNSetting jp_setting = { "psx.bios_jp", MDFNSF_EMU_STATE, "SCPH-5500 BIOS", NULL, MDFNST_STRING, [[[biosPath stringByAppendingPathComponent:@"scph5500"] stringByAppendingPathExtension:@"bin"] UTF8String] };
    MDFNSetting na_setting = { "psx.bios_na", MDFNSF_EMU_STATE, "SCPH-5501 BIOS", NULL, MDFNST_STRING, [[[biosPath stringByAppendingPathComponent:@"scph5501"] stringByAppendingPathExtension:@"bin"] UTF8String] };
    MDFNSetting eu_setting = { "psx.bios_eu", MDFNSF_EMU_STATE, "SCPH-5502 BIOS", NULL, MDFNST_STRING, [[[biosPath stringByAppendingPathComponent:@"scph5502"] stringByAppendingPathExtension:@"bin"] UTF8String] };
    MDFNSetting filesys = { "filesys.path_sav", MDFNSF_NOFLAGS, "Memcards", NULL, MDFNST_STRING, [batterySavesDirectory UTF8String] };
    
    // dox http://mednafen.sourceforge.net/documentation/09x/vb.html
    MDFNSetting vb_parallax = { "vb.disable_parallax", MDFNSF_EMU_STATE, "Disable parallax for BG and OBJ rendering", NULL, MDFNST_BOOL, "1", NULL, NULL, NULL };
    MDFNSetting vb_anaglyph_preset = { "vb.anaglyph.preset", MDFNSF_EMU_STATE, "Disable anaglyph preset", NULL, MDFNST_BOOL, "disabled", NULL, NULL, NULL };
    MDFNSetting vb_anaglyph_lcolor = { "vb.anaglyph.lcolor", MDFNSF_EMU_STATE, "Anaglyph l color", NULL, MDFNST_BOOL, "0xFF0000", NULL, NULL, NULL };
    //MDFNSetting vb_anaglyph_lcolor = { "vb.anaglyph.lcolor", MDFNSF_EMU_STATE, "Anaglyph l color", NULL, MDFNST_BOOL, "0xFFFFFF", NULL, NULL, NULL };
    MDFNSetting vb_anaglyph_rcolor = { "vb.anaglyph.rcolor", MDFNSF_EMU_STATE, "Anaglyph r color", NULL, MDFNST_BOOL, "0x000000", NULL, NULL, NULL };
    //MDFNSetting vb_allow_draw_skip = { "vb.allow_draw_skip", MDFNSF_EMU_STATE, "Allow draw skipping", NULL, MDFNST_BOOL, "1", NULL, NULL, NULL };
    //MDFNSetting vb_instant_display_hack = { "vb.instant_display_hack", MDFNSF_EMU_STATE, "ADisplay latency reduction hack", NULL, MDFNST_BOOL, "1", NULL, NULL, NULL };
    
    settings.push_back(pce_setting);
    
    settings.push_back(pcfx_setting);
    
    settings.push_back(jp_setting);
    settings.push_back(na_setting);
    settings.push_back(eu_setting);
    settings.push_back(filesys);
    
    settings.push_back(vb_parallax);
    settings.push_back(vb_anaglyph_preset);
    settings.push_back(vb_anaglyph_lcolor);
    settings.push_back(vb_anaglyph_rcolor);
    //settings.push_back(vb_allow_draw_skip);
    //settings.push_back(vb_instant_display_hack);
    
    MDFNI_Initialize([biosPath UTF8String], settings);
}

static void emulation_run()
{
    update_input();
    
    static int16_t sound_buf[0x10000];
    MDFN_Rect rects[current->mednafenCoreFBHeight];
    rects[0].w = ~0;
    
    EmulateSpecStruct spec = {0};
    spec.surface = surf;
    spec.SoundRate = 44100;
    spec.SoundBuf = sound_buf;
    spec.LineWidths = rects;
    spec.SoundBufMaxSize = sizeof(sound_buf) / 2;
    spec.SoundVolume = 1.0;
    spec.soundmultiplier = 1.0;
    
    MDFNI_Emulate(&spec);
    
    const uint32_t *pix = surf->pixels;
    
    if(current->systemType == pce)
    {
        unsigned width = rects[0].w;
        unsigned height = spec.DisplayRect.h;
        
        update_video(pix, width, height, current->mednafenCoreFBWidth << 2);
    }
    else if(current->systemType == psx)
    {
        unsigned width = rects[0].w;
        unsigned height = spec.DisplayRect.h;
        
        switch (width)
        {
                // The shifts are not simply (padded_width - real_width) / 2.
            case 350:
                pix += 14;
                width = 320;
                break;
                
            case 700:
                pix += 33;
                width = 640;
                break;
                
            case 400:
                pix += 15;
                width = 364;
                break;
                
            case 280:
                pix += 10;
                width = 256;
                break;
                
            case 560:
                pix += 26;
                width = 512;
                break;
                
            default:
                // This shouldn't happen.
                break;
        }
        
        update_video(pix, width, height, current->mednafenCoreFBWidth << 2);
    }
    else
    {
        unsigned width = spec.DisplayRect.w;
        unsigned height = spec.DisplayRect.h;
        
        update_video(pix, width, height, current->mednafenCoreFBWidth << 2);
    }
    
    update_audio_batch(spec.SoundBuf, spec.SoundBufSize);
}

- (id)init
{
    if((self = [super init]))
    {
        videoBuffer = (uint32_t*)malloc(700 * 576 * 4);
    }
    
    current = self;
    
    return self;
}

- (void)dealloc
{
    free(videoBuffer);
    free(surf);
}

- (void)executeFrame
{
    [self executeFrameSkippingFrame:NO];
}

- (void)executeFrameSkippingFrame: (BOOL) skip
{
    emulation_run();
}

- (BOOL)loadFileAtPath:(NSString *)path
{
    if([[self systemIdentifier] isEqualToString:@"openemu.system.lynx"])
    {
        systemType = lynx;
        
        mednafenCoreModule = @"lynx";
        mednafenCoreTiming = 75;
        mednafenCoreFBWidth = 160;
        mednafenCoreFBHeight = 102;
        mednafenCoreAspect = OEIntSizeMake(8, 5);
    }
    
    if([[self systemIdentifier] isEqualToString:@"openemu.system.pce"])
    {
        systemType = pce;
        
        mednafenCoreModule = @"pce";
        mednafenCoreTiming = 59.82; //7159090.90909090 / 455 / 263 = 59.826
        mednafenCoreFBWidth = 512; //512 ?
        mednafenCoreFBHeight = 264; //224 or 242 ?
        mednafenCoreAspect = OEIntSizeMake(4, 3);
    }
    
    if([[self systemIdentifier] isEqualToString:@"openemu.system.pcfx"])
    {
        systemType = pcfx;
        
        mednafenCoreModule = @"pcfx";
        mednafenCoreTiming = 59.94;
        mednafenCoreFBWidth = 1024; //256 ?
        mednafenCoreFBHeight = 512; //240 or 232 ?
        mednafenCoreAspect = OEIntSizeMake(4, 3);
    }
    
    if([[self systemIdentifier] isEqualToString:@"openemu.system.psx"])
    {
        systemType = psx;
        
        mednafenCoreModule = @"psx";
        mednafenCoreTiming = 59.94;
        mednafenCoreFBWidth = 700;
        mednafenCoreFBHeight = 576;
        mednafenCoreAspect = OEIntSizeMake(4, 3);
    }
    
    if([[self systemIdentifier] isEqualToString:@"openemu.system.vb"])
    {
        systemType = vb;
        
        mednafenCoreModule = @"vb";
        mednafenCoreTiming = 50.27;
        mednafenCoreFBWidth = 384;
        mednafenCoreFBHeight = 224;
        mednafenCoreAspect = OEIntSizeMake(12, 7);
    }
    
    if([[self systemIdentifier] isEqualToString:@"openemu.system.ws"])
    {
        systemType = wswan;
        
        mednafenCoreModule = @"wswan";
        mednafenCoreTiming = 75.47;
        mednafenCoreFBWidth = 224;
        mednafenCoreFBHeight = 144;
        mednafenCoreAspect = OEIntSizeMake(14, 9);
    }
    
    mednafen_init();
    
    memset(pad, 0, sizeof(int16_t) * 10);
    
    game = MDFNI_LoadGame([mednafenCoreModule UTF8String], [path UTF8String]);
    
    frameInterval = mednafenCoreTiming;
    
    emulation_run();
    
    return YES;
}

- (void)setupEmulation
{
    
}

- (void)resetEmulation
{
    MDFNI_Reset();
}

- (void)stopEmulation
{
    MDFNI_CloseGame();
    
    [super stopEmulation];
}

- (OEIntRect)screenRect
{
    return OEIntRectMake(0, 0, current->videoWidth, current->videoHeight);
}

- (OEIntSize)bufferSize
{
    return OEIntSizeMake(current->mednafenCoreFBWidth, current->mednafenCoreFBHeight);
}

- (OEIntSize)aspectSize
{
    return mednafenCoreAspect;
}

- (const void *)videoBuffer
{
    return videoBuffer;
}

- (GLenum)pixelFormat
{
    return GL_BGRA;
}

- (GLenum)pixelType
{
    return GL_UNSIGNED_INT_8_8_8_8_REV;
}

- (GLenum)internalPixelFormat
{
    return GL_RGB8;
}

- (double)audioSampleRate
{
    return sampleRate ? sampleRate : 44100;
    //return game->soundrate;
}

- (NSTimeInterval)frameInterval
{
    return frameInterval ? frameInterval : 59.94;
    //return game->isPalPSX? 50.00 : 59.94;
}

- (NSUInteger)channelCount
{
    return 2;
    //return game->soundchan;
}

- (BOOL)saveStateToFileAtPath:(NSString *)fileName
{
    return NO;
}

- (BOOL)loadStateFromFileAtPath:(NSString *)fileName
{
    return NO;
}

@end
