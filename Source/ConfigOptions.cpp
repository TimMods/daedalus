
/*
Copyright (C) 2001 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


// This file contains many variables that can be used to
// change the overall operation of the emu. They are here for
// convienience, so that they can quickly be changed when
// developing. Generally they will be changed by the ini file 
// settings.

#include "stdafx.h"
#include "ConfigOptions.h"

bool	gSpeedSyncEnabled			= false;	// Enable to limit frame rate.
bool	gGraphicsEnabled			= true;		// Show graphics
bool	gDynarecEnabled				= true;		// Use dynamic recompilation
bool	gDynarecStackOptimisation	= true;		// Enable the dynarec stack optmisation
bool	gDynarecLoopOptimisation	= true;		// Enable the dynarec loop optmisation
bool	gOSHooksEnabled				= true;		// Apply os-hooks
u32		gCheckTextureHashFrequency	= 5;		// How often to check textures for updates (every N frames, 0 to disable)
bool	gDoubleDisplayEnabled		= true;		// Workaround for games that have shaking issues
bool	gSimulateDoubleDisabled		= false;	// Hack to disable SimulateDoubles, fixes Mario Party 
bool	gCleanSceneEnabled			= false;	// Clean our Scenes, it gets rid of many glitches
bool	gCheckN64FPUsageDisable		= false;	// This fixes several games, check R4300.cpp > R4300_SetSR : 511
bool	gAudioRateMatch				= false;	// Matches audio rate with framerate, only works if 50-100% sync rate
bool	gFogEnabled					= false;	// Enable fog
u32		gControllerIndex			= 0;		// Which controller config to set

DaedalusConfig g_DaedalusConfig =
{
	false,			// RecurseRomDirectory	// Recursively scan rom directories?
	false,			// WarnMemoryErrors;	// Warn on all memory access errors?
											// If false, the exception handler is
											// taken without any warnings on the debug console
											
	true,			// RunAutomatically;	// Run roms automatically after loading

	true,			// TrapExceptions;		// If set, this causes exceptions in the audio
											// plugin, cpu thread and graphics processing to
											// be handled nicely. I turn this off for 
											// development to see when things mess up
	0,				// nNumRomsDirs
	// szRomsDir
	// szSaveDir

};