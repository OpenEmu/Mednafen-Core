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
//#import "OELynxSystemResponderClient.h"
//#import "OEPCESystemResponderClient.h"
//#import "OEPCFXSystemResponderClient.h"
//#import "OEPSXSystemResponderClient.h"
//#import "OEVBSystemResponderClient.h"
//#import "OEWSSystemResponderClient.h"

//@interface MednafenGameCore () <OELynxSystemResponderClient, OEPCESystemResponderClient, OEPCFXSystemResponderClient, OEPSXSystemResponderClient, OEVBSystemResponderClient, OEWSSystemResponderClient>
@interface MednafenGameCore ()
{
    uint32_t *videoBuffer;
    int videoWidth, videoHeight;
    int16_t pad[2][16];
    NSString *romName;
    double sampleRate;
}

@end

MednafenGameCore *current;
@implementation MednafenGameCore

//- (oneway void)didPushPSXButton:(OEPSXButton)button forPlayer:(NSUInteger)player;
//{
//    pad[player-1][PSXEmulatorValues[button]] = 1;
//}
//
//- (oneway void)didReleasePSXButton:(OEPSXButton)button forPlayer:(NSUInteger)player;
//{
//    pad[player-1][PSXEmulatorValues[button]] = 0;
//}

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
}

- (void)executeFrame
{
    [self executeFrameSkippingFrame:NO];
}

- (void)executeFrameSkippingFrame: (BOOL) skip
{
    
}

- (void)setupEmulation
{
    
}

- (BOOL)loadFileAtPath:(NSString *)path
{
    memset(pad, 0, sizeof(int16_t) * 10);
    return NO;
}

- (void)resetEmulation
{
    
}

- (void)stopEmulation
{
    [super stopEmulation];
}

- (OEIntRect)screenRect
{
    return OEIntRectMake(0, 0, current->videoWidth, current->videoHeight);
}

- (OEIntSize)bufferSize
{
    return OEIntSizeMake(700, 576);
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
}

- (NSTimeInterval)frameInterval
{
    return frameInterval ? frameInterval : 59.94;
}

- (NSUInteger)channelCount
{
    return 2; //gameInfo->soundchan;
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
