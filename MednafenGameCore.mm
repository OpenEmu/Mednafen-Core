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
#import <OpenGL/gl.h>
#import "OELynxSystemResponderClient.h"
#import "OEPCESystemResponderClient.h"
#import "OEPCECDSystemResponderClient.h"
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

namespace MDFN_IEN_VB
{
    extern void VIP_SetParallaxDisable(bool disabled);
    extern void VIP_SetAnaglyphColors(uint32 lcolor, uint32 rcolor);
    int mednafenCurrentDisplayMode = 1;
}

enum systemTypes{ lynx, pce, pcfx, psx, vb, wswan };

@interface MednafenGameCore () <OELynxSystemResponderClient, OEPCESystemResponderClient, OEPCECDSystemResponderClient, OEPCFXSystemResponderClient, OEPSXSystemResponderClient, OEVBSystemResponderClient, OEWSSystemResponderClient>
{
    int systemType;
    int videoWidth, videoHeight;
    int videoOffsetX, videoOffsetY;
    uint16_t input_buf[2];
    NSString *romName;
    double sampleRate;
    double masterClock;
    
    NSString *mednafenCoreModule;
    NSTimeInterval mednafenCoreTiming;
    OEIntSize mednafenCoreAspect;
}

@end

MednafenGameCore *current;
@implementation MednafenGameCore

static void mednafen_init()
{
    std::vector<MDFNGI*> ext;
    MDFNI_InitializeModules(ext);
    
    std::vector<MDFNSetting> settings;
    
    NSString *batterySavesDirectory = current.batterySavesDirectoryPath;
    NSString *biosPath = current.biosDirectoryPath;
    
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

- (id)init
{
    if((self = [super init]))
    {
        current = self;
    }
    
    return self;
}

- (void)dealloc
{
    delete surf;
}

# pragma mark - Execution

static void emulation_run()
{
    static int16_t sound_buf[0x10000];
    MDFN_Rect rects[game->fb_height];
    rects[0].w = ~0;

    EmulateSpecStruct spec = {0};
    spec.surface = surf;
    spec.SoundRate = current->sampleRate;
    spec.SoundBuf = sound_buf;
    spec.LineWidths = rects;
    spec.SoundBufMaxSize = sizeof(sound_buf) / 2;
    spec.SoundVolume = 1.0;
    spec.soundmultiplier = 1.0;

    MDFNI_Emulate(&spec);

    current->mednafenCoreTiming = current->masterClock / spec.MasterCycles;

    if(current->systemType == psx)
    {
        current->videoWidth = rects[0].w;

        // Crop overscan for NTSC. Might remove as this kinda sucks
        if (!game->isPalPSX)
            switch (current->videoWidth)
        {
                // The shifts are not simply (padded_width - real_width) / 2.
            case 350:
                current->videoOffsetX = 14;
                current->videoWidth = 320;
                break;

            case 700:
                current->videoOffsetX = 33;
                current->videoWidth = 640;
                break;

            case 400:
                current->videoOffsetX = 15;
                current->videoWidth = 364;
                break;

            case 280:
                current->videoOffsetX = 10;
                current->videoWidth = 256;
                break;

            case 560:
                current->videoOffsetX = 26;
                current->videoWidth = 512;
                break;

            default:
                // This shouldn't happen.
                break;
        }
    }
    else if(game->multires)
    {
        current->videoWidth = rects[0].w;
        current->videoOffsetX = rects[0].x;
    }
    else
    {
        current->videoWidth = spec.DisplayRect.w;
        current->videoOffsetX = spec.DisplayRect.x;
    }

    current->videoHeight = spec.DisplayRect.h;
    current->videoOffsetY = spec.DisplayRect.y;

    update_audio_batch(spec.SoundBuf, spec.SoundBufSize);
}

- (BOOL)loadFileAtPath:(NSString *)path
{
    [[NSFileManager defaultManager] createDirectoryAtPath:[self batterySavesDirectoryPath] withIntermediateDirectories:YES attributes:nil error:NULL];
    
    if([[self systemIdentifier] isEqualToString:@"openemu.system.lynx"])
    {
        systemType = lynx;

        mednafenCoreModule = @"lynx";
        mednafenCoreAspect = OEIntSizeMake(8, 5);
        sampleRate         = 48000;
    }

    if([[self systemIdentifier] isEqualToString:@"openemu.system.pce"] || [[self systemIdentifier] isEqualToString:@"openemu.system.pcecd"])
    {
        systemType = pce;

        mednafenCoreModule = @"pce";
        mednafenCoreAspect = OEIntSizeMake(4, 3);
        sampleRate         = 48000;
    }

    if([[self systemIdentifier] isEqualToString:@"openemu.system.pcfx"])
    {
        systemType = pcfx;

        mednafenCoreModule = @"pcfx";
        mednafenCoreAspect = OEIntSizeMake(4, 3);
        sampleRate         = 48000;
    }

    if([[self systemIdentifier] isEqualToString:@"openemu.system.psx"])
    {
        systemType = psx;

        mednafenCoreModule = @"psx";
        mednafenCoreAspect = OEIntSizeMake(4, 3);
        sampleRate         = 44100;
    }

    if([[self systemIdentifier] isEqualToString:@"openemu.system.vb"])
    {
        systemType = vb;

        mednafenCoreModule = @"vb";
        mednafenCoreAspect = OEIntSizeMake(12, 7);
        sampleRate         = 48000;
    }

    if([[self systemIdentifier] isEqualToString:@"openemu.system.ws"])
    {
        systemType = wswan;

        mednafenCoreModule = @"wswan";
        mednafenCoreAspect = OEIntSizeMake(14, 9);
        sampleRate         = 48000;
    }

    mednafen_init();

    game = MDFNI_LoadGame([mednafenCoreModule UTF8String], [path UTF8String]);

    if(!game)
        return NO;

    // BGRA pixel format
    MDFN_PixelFormat pix_fmt(MDFN_COLORSPACE_RGB, 16, 8, 0, 24);
    surf = new MDFN_Surface(NULL, game->fb_width, game->fb_height, game->fb_width, pix_fmt);

    masterClock = game->MasterClock >> 32;
    
    game->SetInput(0, "gamepad", &input_buf[0]);
    // Set P2 gamepad for available systems
    if (systemType == pce || systemType == pcfx || systemType == psx)
        game->SetInput(1, "gamepad", &input_buf[1]);
    
    emulation_run();

    return YES;
}

- (void)executeFrame
{
    [self executeFrameSkippingFrame:NO];
}

- (void)executeFrameSkippingFrame: (BOOL) skip
{
    emulation_run();
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

- (NSTimeInterval)frameInterval
{
    return mednafenCoreTiming ?: 60;
}

# pragma mark - Video

- (OEIntRect)screenRect
{
    return OEIntRectMake(videoOffsetX, videoOffsetY, videoWidth, videoHeight);
}

- (OEIntSize)bufferSize
{
    return OEIntSizeMake(game->fb_width, game->fb_height);
}

- (OEIntSize)aspectSize
{
    return mednafenCoreAspect;
}

- (const void *)videoBuffer
{
    return surf->pixels;
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

# pragma mark - Audio

static size_t update_audio_batch(const int16_t *data, size_t frames){
    [[current ringBufferAtIndex:0] write:data maxLength:frames * [current channelCount] * 2];
    return frames;
}

- (double)audioSampleRate
{
    return sampleRate ? sampleRate : 48000;
}

- (NSUInteger)channelCount
{
    return game->soundchan;
}

# pragma mark - Save States

- (BOOL)saveStateToFileAtPath:(NSString *)fileName
{
    return MDFNSS_Save([fileName UTF8String], "");
}

- (BOOL)loadStateFromFileAtPath:(NSString *)fileName
{
    return MDFNSS_Load([fileName UTF8String], "");
}

# pragma mark - Input

// Map OE button order to Mednafen button order
const int LynxMap[] = { 6, 7, 4, 5, 0, 1, 3, 2 };
const int PCEMap[]  = { 4, 6, 7, 5, 0, 1, 8, 9, 10, 11, 3, 2, 12 };
const int PCFXMap[] = { 8, 10, 11, 9, 0, 1, 2, 3, 4, 5, 7, 6 };
const int PSXMap[]  = { 4, 6, 7, 5, 12, 13, 14, 15, 10, 8, 1, 11, 9, 2, 3, 0 };
const int VBMap[]   = { 9, 8, 7, 6, 4, 13, 12, 5, 3, 2, 0, 1, 10, 11 };
const int WSMap[]   = { 0, 2, 3, 1, 4, 6, 7, 5, 9, 10, 8, 11 };

- (oneway void)didPushLynxButton:(OELynxButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << LynxMap[button];
}

- (oneway void)didReleaseLynxButton:(OELynxButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << LynxMap[button]);
}

- (oneway void)didPushPCEButton:(OEPCEButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << PCEMap[button];
}

- (oneway void)didReleasePCEButton:(OEPCEButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << PCEMap[button]);
}

- (oneway void)didPushPCECDButton:(OEPCECDButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << PCEMap[button];
}

- (oneway void)didReleasePCECDButton:(OEPCECDButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << PCEMap[button]);
}

- (oneway void)didPushPCFXButton:(OEPCFXButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << PCFXMap[button];
}

- (oneway void)didReleasePCFXButton:(OEPCFXButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << PCFXMap[button]);
}

- (oneway void)didPushPSXButton:(OEPSXButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << PSXMap[button];
}

- (oneway void)didReleasePSXButton:(OEPSXButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << PSXMap[button]);
}

- (oneway void)didPushVBButton:(OEVBButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << VBMap[button];
}

- (oneway void)didReleaseVBButton:(OEVBButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << VBMap[button]);
}

- (oneway void)didPushWSButton:(OEWSButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] |= 1 << WSMap[button];
}

- (oneway void)didReleaseWSButton:(OEWSButton)button forPlayer:(NSUInteger)player;
{
    input_buf[player-1] &= ~(1 << WSMap[button]);
}

- (void)changeDisplayMode
{
    if (systemType == vb)
    {
        switch (MDFN_IEN_VB::mednafenCurrentDisplayMode)
        {
            case 0: // (2D) red/black
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFF0000, 0x000000);
                MDFN_IEN_VB::VIP_SetParallaxDisable(true);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 1: // (2D) white/black
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFFFFFF, 0x000000);
                MDFN_IEN_VB::VIP_SetParallaxDisable(true);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 2: // (2D) purple/black
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFF00FF, 0x000000);
                MDFN_IEN_VB::VIP_SetParallaxDisable(true);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 3: // (3D) red/blue
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFF0000, 0x0000FF);
                MDFN_IEN_VB::VIP_SetParallaxDisable(false);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 4: // (3D) red/cyan
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFF0000, 0x00B7EB);
                MDFN_IEN_VB::VIP_SetParallaxDisable(false);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 5: // (3D) red/electric cyan
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFF0000, 0x00FFFF);
                MDFN_IEN_VB::VIP_SetParallaxDisable(false);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 6: // (3D) red/green
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFF0000, 0x00FF00);
                MDFN_IEN_VB::VIP_SetParallaxDisable(false);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 7: // (3D) green/red
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0x00FF00, 0xFF0000);
                MDFN_IEN_VB::VIP_SetParallaxDisable(false);
                MDFN_IEN_VB::mednafenCurrentDisplayMode++;
                break;
                
            case 8: // (3D) yellow/blue
                MDFN_IEN_VB::VIP_SetAnaglyphColors(0xFFFF00, 0x0000FF);
                MDFN_IEN_VB::VIP_SetParallaxDisable(false);
                MDFN_IEN_VB::mednafenCurrentDisplayMode = 0;
                break;
                
            default:
                return;
                break;
        }
    }
}

@end
