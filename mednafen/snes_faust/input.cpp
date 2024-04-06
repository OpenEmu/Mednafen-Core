/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* input.cpp:
**  Copyright (C) 2015-2022 Mednafen Team
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

#include "snes.h"
#include "input.h"
#include "input_device.h"
#include "input/multitap.h"
#include "input/gamepad.h"
#include "input/mouse.h"

namespace MDFN_IEN_SNES_FAUST
{

InputDevice::InputDevice()
{

}

InputDevice::~InputDevice()
{

}

void InputDevice::Power(void)
{

}

void InputDevice::UpdatePhysicalState(const uint8* data)
{

}

uint8 InputDevice::Read(bool IOB)
{
 return 0;
}

void InputDevice::SetLatch(bool state)
{

}

void InputDevice::StateAction(StateMem* sm, const unsigned load, const bool data_only, const char* sname_prefix)
{

}

//
//
//
//
//
static struct
{
 InputDevice_Gamepad gamepad;
 InputDevice_Mouse mouse;
} PossibleDevices[8];

static InputDevice NoneDevice;

static InputDevice_MTap PossibleMTaps[2];
static bool MTapEnabled[2];

// Mednafen virtual
static InputDevice* Devices[8];
static uint8* DeviceData[8];

// SNES physical
static InputDevice* Ports[2];

static uint8 WRIO;

static bool JoyLS;
static uint8 JoyARData[8];

static DEFREAD(Read_JoyARData)
{
 if(MDFN_UNLIKELY(DBG_InHLRead))
 {
  return JoyARData[A & 0x7];
 }

 CPUM.timestamp += MEMCYC_FAST;

 //printf("Read: %08x\n", A);

 return JoyARData[A & 0x7];
}

static DEFREAD(Read_4016)
{
 if(MDFN_UNLIKELY(DBG_InHLRead))
 {
  return CPUM.mdr & 0xFC; // | TODO!
 }

 CPUM.timestamp += MEMCYC_XSLOW;

 uint8 ret = CPUM.mdr & 0xFC;

 ret |= Ports[0]->Read(WRIO & (0x40 << 0));

 //printf("Read 4016: %02x\n", ret);
 return ret;
}

static DEFWRITE(Write_4016)
{
 CPUM.timestamp += MEMCYC_XSLOW;

 JoyLS = V & 1;
 for(unsigned sport = 0; sport < 2; sport++)
  Ports[sport]->SetLatch(JoyLS);

 //printf("Write 4016: %02x\n", V);
}

static DEFREAD(Read_4017)
{
 if(MDFN_UNLIKELY(DBG_InHLRead))
 {
  return (CPUM.mdr & 0xE0) | 0x1C; // | TODO!
 }

 CPUM.timestamp += MEMCYC_XSLOW;
 uint8 ret = (CPUM.mdr & 0xE0) | 0x1C;

 ret |= Ports[1]->Read(WRIO & (0x40 << 1));

 //printf("Read 4017: %02x\n", ret);
 return ret;
}

static DEFWRITE(Write_WRIO)
{
 CPUM.timestamp += MEMCYC_FAST;

 WRIO = V;
}

static DEFREAD(Read_4213)
{
 if(MDFN_UNLIKELY(DBG_InHLRead))
 {
  return WRIO;
 }

 CPUM.timestamp += MEMCYC_FAST;

 return WRIO;
}


void INPUT_AutoRead(void)
{
 for(unsigned sport = 0; sport < 2; sport++)
 {
  Ports[sport]->SetLatch(true);
  Ports[sport]->SetLatch(false);

  unsigned ard[2] = { 0 };

  for(unsigned b = 0; b < 16; b++)
  {
   uint8 rv = Ports[sport]->Read(WRIO & (0x40 << sport));

   ard[0] = (ard[0] << 1) | ((rv >> 0) & 1);
   ard[1] = (ard[1] << 1) | ((rv >> 1) & 1);
  }

  for(unsigned ai = 0; ai < 2; ai++)
   MDFN_en16lsb(&JoyARData[sport * 2 + ai * 4], ard[ai]);
 }
 JoyLS = false;
}

static MDFN_COLD void MapDevices(void)
{
 for(unsigned sport = 0, vport = 0; sport < 2; sport++)
 {
  if(MTapEnabled[sport])
  {
   Ports[sport] = &PossibleMTaps[sport];

   for(unsigned mport = 0; mport < 4; mport++)
    PossibleMTaps[sport].SetSubDevice(mport, Devices[vport++]);
  }
  else
   Ports[sport] = Devices[vport++];
 }
}

void INPUT_Init(void)
{
 for(unsigned bank = 0x00; bank < 0x100; bank++)
 {
  if(bank <= 0x3F || (bank >= 0x80 && bank <= 0xBF))
  {
   Set_A_Handlers((bank << 16) | 0x4016, Read_4016, Write_4016);
   Set_A_Handlers((bank << 16) | 0x4017, Read_4017, OBWrite_XSLOW);

   Set_A_Handlers((bank << 16) | 0x4201, OBRead_FAST, Write_WRIO);

   Set_A_Handlers((bank << 16) | 0x4213, Read_4213, OBWrite_FAST);

   Set_A_Handlers((bank << 16) | 0x4218, (bank << 16) | 0x421F, Read_JoyARData, OBWrite_FAST);
  }
 }

 for(unsigned vport = 0; vport < 8; vport++)
 {
  DeviceData[vport] = nullptr;
  Devices[vport] = &NoneDevice;
 }

 for(unsigned sport = 0; sport < 2; sport++)
  for(unsigned mport = 0; mport < 4; mport++)
   PossibleMTaps[sport].SetSubDevice(mport, &NoneDevice);

 MTapEnabled[0] = MTapEnabled[1] = false;
 MapDevices();
}

void INPUT_SetMultitap(const bool (&enabled)[2])
{
 for(unsigned sport = 0; sport < 2; sport++)
 {
  if(enabled[sport] != MTapEnabled[sport])
  {
   PossibleMTaps[sport].SetLatch(JoyLS);
   PossibleMTaps[sport].Power();
   MTapEnabled[sport] = enabled[sport];
  }
 }

 MapDevices();
}

void INPUT_Kill(void)
{


}

void INPUT_Reset(bool powering_up)
{
 JoyLS = false;
 for(unsigned sport = 0; sport < 2; sport++)
  Ports[sport]->SetLatch(JoyLS);

 memset(JoyARData, 0x00, sizeof(JoyARData));

 if(powering_up)
 {
  WRIO = 0xFF;

  for(unsigned sport = 0; sport < 2; sport++)
   Ports[sport]->Power();
 }
}

void INPUT_Set(unsigned vport, const char* type, uint8* ptr)
{
 InputDevice* nd = &NoneDevice;

 DeviceData[vport] = ptr;

 if(!strcmp(type, "gamepad"))
  nd = &PossibleDevices[vport].gamepad;
 else if(!strcmp(type, "mouse"))
  nd = &PossibleDevices[vport].mouse;
 else if(strcmp(type, "none"))
  abort();

 if(Devices[vport] != nd)
 {
  Devices[vport] = nd;
  Devices[vport]->SetLatch(JoyLS);
  Devices[vport]->Power();
 }

 MapDevices();
}

void INPUT_StateAction(StateMem* sm, const unsigned load, const bool data_only)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(JoyARData),
  SFVAR(JoyLS),

  SFVAR(WRIO),

  SFEND
 };

 MDFNSS_StateAction(sm, load, data_only, StateRegs, "INPUT");

 for(unsigned sport = 0; sport < 2; sport++)
 {
  char sprefix[32] = "PORTn";

  sprefix[4] = '0' + sport;

  Ports[sport]->StateAction(sm, load, data_only, sprefix);
 }
}

void INPUT_UpdatePhysicalState(void)
{
 for(unsigned vport = 0; vport < 8; vport++)
  Devices[vport]->UpdatePhysicalState(DeviceData[vport]);
}

static const std::vector<InputDeviceInfoStruct> InputDeviceInfo =
{
 // None
 { 
  "none",
  "none",
  NULL,
  IDII_Empty
 },

 // Gamepad
 {
  "gamepad",
  "Gamepad",
  NULL,
  GamepadIDII
 },

 // Mouse
 {
  "mouse",
  "Mouse",
  NULL,
  MouseIDII
 },
};

const std::vector<InputPortInfoStruct> INPUT_PortInfo =
{
 { "port1", "Virtual Port 1", InputDeviceInfo, "gamepad" },
 { "port2", "Virtual Port 2", InputDeviceInfo, "gamepad" },
 { "port3", "Virtual Port 3", InputDeviceInfo, "gamepad" },
 { "port4", "Virtual Port 4", InputDeviceInfo, "gamepad" },
 { "port5", "Virtual Port 5", InputDeviceInfo, "gamepad" },
 { "port6", "Virtual Port 6", InputDeviceInfo, "gamepad" },
 { "port7", "Virtual Port 7", InputDeviceInfo, "gamepad" },
 { "port8", "Virtual Port 8", InputDeviceInfo, "gamepad" }
};

}
