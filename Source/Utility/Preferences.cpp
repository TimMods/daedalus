/*
Copyright (C) 2001 CyRUS64 (http://www.boob.co.uk)
Copyright (C) 2006,2007 StrmnNrmn

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

#include "stdafx.h"
#include "Preferences.h"
#include "IniFile.h"
#include "FramerateLimiter.h"

#include "Core/ROM.h"
#include "Interface/RomDB.h"

#include "Utility/IO.h"

#include "ConfigOptions.h"

#include "Input/InputManager.h"

#include <string>
#include <set>
#include <map>

extern EFrameskipValue gFrameskipValue;
extern f32 gZoomX;

//*****************************************************************************
//
//*****************************************************************************
SGlobalPreferences	gGlobalPreferences;

//*****************************************************************************
//
//*****************************************************************************
class IPreferences : public CPreferences
{
	public:
		IPreferences();
		virtual ~IPreferences();

		//
		// CPreferences implementation
		//
		bool						OpenPreferencesFile( const char * filename );
		void						Commit();

		bool						GetRomPreferences( const RomID & id, SRomPreferences * preferences ) const;
		void						SetRomPreferences( const RomID & id, const SRomPreferences & preferences );

	private:
		void						OutputSectionDetails( const RomID & id, const SRomPreferences & preferences, FILE * fh );

	private:
		typedef std::map<RomID, SRomPreferences>	PreferencesMap;

		PreferencesMap			mPreferences;

		bool					mDirty;				// (STRMNNRMN - Changed since read from disk?)
		std::string				mFilename;
};



//*****************************************************************************
// Singleton creator
//*****************************************************************************
template<> bool	CSingleton< CPreferences >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IPreferences();

	return true;
}


//*****************************************************************************
// Constructor
//*****************************************************************************
IPreferences::IPreferences()
:	mDirty( false )
{
	char ini_filename[MAX_PATH + 1];
	IO::Path::Combine( ini_filename, gDaedalusExePath, "preferences.ini" );
	OpenPreferencesFile( ini_filename );
}

//*****************************************************************************
//
//*****************************************************************************
IPreferences::~IPreferences()
{
	if ( mDirty )
	{
		Commit();
	}
}

//*****************************************************************************
//
//*****************************************************************************
static RomID	RomIDFromString( const char * str )
{
	u32 crc1, crc2, country;
	sscanf( str, "%08x%08x-%02x", &crc1, &crc2, &country );
	return RomID( crc1, crc2, (u8)country );
}

//*****************************************************************************
//
//*****************************************************************************
bool IPreferences::OpenPreferencesFile( const char * filename )
{
	//
	// Remember the filename
	//
	mFilename = filename;

	CIniFile * p_ini_file( CIniFile::Create( filename ) );
	if( p_ini_file == NULL )
	{
		return false;
	}

	const CIniFileSection *	section( p_ini_file->GetDefaultSection() );
	if( section != NULL )
	{
		const CIniFileProperty * property;

#define BOOL_SETTING( b, nm, def )	if( section->FindProperty( #nm, &property ) ) { b.nm = property->GetBooleanValue( def.nm ); }
#define INT_SETTING( b, nm, def )	if( section->FindProperty( #nm, &property ) ) {	b.nm = property->GetIntValue( def.nm ); }
#define FLOAT_SETTING( b, nm, def ) if( section->FindProperty( #nm, &property ) ) {	b.nm = property->GetFloatValue( def.nm ); }

		const SGlobalPreferences	defaults;

		INT_SETTING( gGlobalPreferences, DisplayFramerate, defaults );
		BOOL_SETTING( gGlobalPreferences, SoftwareClipping, defaults );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		BOOL_SETTING( gGlobalPreferences, HighlightInexactBlendModes, defaults );
#endif
		BOOL_SETTING( gGlobalPreferences, BatteryWarning, defaults );
		BOOL_SETTING( gGlobalPreferences, LargeROMBuffer, defaults );
		FLOAT_SETTING( gGlobalPreferences, StickMinDeadzone, defaults );
		FLOAT_SETTING( gGlobalPreferences, StickMaxDeadzone, defaults );

#ifndef DAEDALUS_PUBLIC_RELEASE
		BOOL_SETTING( gGlobalPreferences, CustomBlendModes, defaults );
		BOOL_SETTING( gGlobalPreferences, SkipSplash, defaults );
		BOOL_SETTING( gGlobalPreferences, LogMicrocodes, defaults );
#endif
		
		if( section->FindProperty( "GuiType", &property ) )
		{
			u32 value( property->GetIntValue(defaults.GuiType) );
			if( value < NUM_GUI_TYPES ) //value >= 0 && not needed as it's always True
			{
				gGlobalPreferences.GuiType = EGuiType( value );
			}
		}

		if( section->FindProperty( "GuiColor", &property ) )
		{
			u32 value( property->GetIntValue(defaults.GuiColor) );
			if( value < NUM_COLOR_TYPES ) //value >= 0 && not needed as it's always True
			{
				gGlobalPreferences.GuiColor = EGuiColor( value );
			}
		}

		if( section->FindProperty( "ForceTextureFilter", &property ) )
		{
			u32 value( property->GetIntValue(defaults.ForceTextureFilter) );
			if( value < NUM_FILTER_TYPES ) //value >= 0 && not needed as it's always True
			{
				gGlobalPreferences.ForceTextureFilter = EForceTextureFilter( value );
			}
		}
		if( section->FindProperty( "ViewportType", &property ) )
		{
			u32	value( property->GetIntValue( defaults.ViewportType ) );
			if(value < NUM_VIEWPORT_TYPES ) //value >= 0 && Not need as it's always True
			{
				gGlobalPreferences.ViewportType = EViewportType( value );
			}
		}

		BOOL_SETTING( gGlobalPreferences, TVEnable, defaults );
		BOOL_SETTING( gGlobalPreferences, TVLaced, defaults );
		if( section->FindProperty( "TVType", &property ) )
		{
			u32	value( property->GetIntValue( defaults.TVType ) );
			if( value < 2 ) //value >= 0 && not needed as it's always True
			{
				gGlobalPreferences.TVType = ETVType( value );
			}
		}
	}

	for( u32 section_idx = 0; section_idx < p_ini_file->GetNumSections(); ++section_idx )
	{
		const CIniFileSection * section( p_ini_file->GetSection( section_idx ) );

		RomID			id( RomIDFromString( section->GetName() ) );
		SRomPreferences	preferences;

		const CIniFileProperty * property;
		if( section->FindProperty( "PatchesEnabled", &property ) )
		{
			preferences.PatchesEnabled = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "SpeedSyncEnabled", &property ) )
		{
			preferences.SpeedSyncEnabled = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "DynarecEnabled", &property ) )
		{
			preferences.DynarecEnabled = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "DynarecStackOptimisation", &property ) )
		{
			preferences.DynarecStackOptimisation = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "DynarecLoopOptimisation", &property ) )
		{
			preferences.DynarecLoopOptimisation = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "DoubleDisplayEnabled", &property ) )
		{
			preferences.DoubleDisplayEnabled = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "SimulateDoubleDisabled", &property ) )
		{
			preferences.SimulateDoubleDisabled = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "CleanSceneEnabled", &property ) )
		{
			preferences.CleanSceneEnabled = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "CheckN64FPUsageDisable", &property ) )
		{
			preferences.CheckN64FPUsageDisable = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "AudioRateMatch", &property ) ) 	 
		{ 	 
            preferences.AudioRateMatch = property->GetBooleanValue( false ); 	 
		}
		if( section->FindProperty( "FogEnabled", &property ) ) 	 
		{ 	 
            preferences.FogEnabled = property->GetBooleanValue( false ); 	 
		}
		if( section->FindProperty( "CheckTextureHashFrequency", &property ) )
		{
			preferences.CheckTextureHashFrequency = ROM_GetTextureHashFrequencyFromFrames( atoi( property->GetValue() ) );
		}
		if( section->FindProperty( "Frameskip", &property ) )
		{
			preferences.Frameskip = ROM_GetFrameskipValueFromInt( atoi( property->GetValue() ) );
		}
		if( section->FindProperty( "AudioEnabled", &property ) )
		{
			int audio_enabled = atoi( property->GetValue() );

			if( audio_enabled >= APM_DISABLED && audio_enabled <= APM_ENABLED_SYNC )
				preferences.AudioEnabled = static_cast<EAudioPluginMode>( audio_enabled );
			else
				preferences.AudioEnabled = APM_DISABLED;
		}
//		if( section->FindProperty( "AudioAdaptFrequency", &property ) )
//		{
//			preferences.AudioAdaptFrequency = property->GetBooleanValue( false );
//		}
		if( section->FindProperty( "ZoomX", &property ) )
		{
			preferences.ZoomX = (float) atof( property->GetValue() );
		}
		if( section->FindProperty( "Controller", &property ) )
		{
			preferences.ControllerIndex = CInputManager::Get()->GetConfigurationFromName( property->GetValue() );
		}

		mPreferences[ id ] = preferences;
	}

	mDirty = false;

	delete p_ini_file;
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void IPreferences::OutputSectionDetails( const RomID & id, const SRomPreferences & preferences, FILE * fh )
{
	// Generate the CRC-ID for this rom:
	RomSettings		settings;
	CRomSettingsDB::Get()->GetSettings( id, &settings );

	fprintf(fh, "{%08x%08x-%02x}\t// %s\n", id.CRC[0], id.CRC[1], id.CountryID, settings.GameName.c_str() );
	fprintf(fh, "PatchesEnabled=%d\n",preferences.PatchesEnabled);
	fprintf(fh, "SpeedSyncEnabled=%d\n",preferences.SpeedSyncEnabled);
	fprintf(fh, "DynarecEnabled=%d\n",preferences.DynarecEnabled);
	fprintf(fh, "DynarecStackOptimisation=%d\n",preferences.DynarecStackOptimisation);
	fprintf(fh, "DynarecLoopOptimisation=%d\n",preferences.DynarecLoopOptimisation);
	fprintf(fh, "DoubleDisplayEnabled=%d\n",preferences.DoubleDisplayEnabled);
	fprintf(fh, "SimulateDoubleDisabled=%d\n",preferences.SimulateDoubleDisabled);
	fprintf(fh, "CleanSceneEnabled=%d\n",preferences.CleanSceneEnabled);
	fprintf(fh, "CheckN64FPUsageDisable=%d\n",preferences.CheckN64FPUsageDisable);
	fprintf(fh, "AudioRateMatch=%d\n",preferences.AudioRateMatch);
	fprintf(fh, "FogEnabled=%d\n",preferences.FogEnabled);
	fprintf(fh, "CheckTextureHashFrequency=%d\n", ROM_GetTexureHashFrequencyAsFrames( preferences.CheckTextureHashFrequency ) );
	fprintf(fh, "Frameskip=%d\n", ROM_GetFrameskipValueAsInt( preferences.Frameskip ) );
	fprintf(fh, "AudioEnabled=%d\n", preferences.AudioEnabled);
	fprintf(fh, "ZoomX=%f\n", preferences.ZoomX );
	fprintf(fh, "Controller=%s\n", CInputManager::Get()->GetConfigurationName( preferences.ControllerIndex ));
	fprintf(fh, "\n");			// Spacer
}

//*****************************************************************************
//	Write out the .ini file, keeping the original comments intact
//*****************************************************************************
void IPreferences::Commit()
{
	FILE * fh( fopen(mFilename.c_str(), "w") );
	if (fh != NULL)
	{
		const SGlobalPreferences	defaults;

#define OUTPUT_BOOL( b, nm, def )		fprintf( fh, "%s=%s\n", #nm, b.nm ? "yes" : "no" );
#define OUTPUT_FLOAT( b, nm, def )		fprintf( fh, "%s=%f\n", #nm, b.nm );
#define OUTPUT_INT( b, nm, def )		fprintf( fh, "%s=%d\n", #nm, b.nm );

		OUTPUT_INT( gGlobalPreferences, DisplayFramerate, defaults );
		OUTPUT_BOOL( gGlobalPreferences, SoftwareClipping, defaults );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		OUTPUT_BOOL( gGlobalPreferences, HighlightInexactBlendModes, defaults );
#endif
		OUTPUT_BOOL( gGlobalPreferences, BatteryWarning, defaults );
		OUTPUT_BOOL( gGlobalPreferences, LargeROMBuffer, defaults );
		OUTPUT_INT( gGlobalPreferences, GuiType, defaults );
		OUTPUT_INT( gGlobalPreferences, GuiColor, defaults )
		OUTPUT_INT( gGlobalPreferences, ForceTextureFilter, defaults );
		OUTPUT_FLOAT( gGlobalPreferences, StickMinDeadzone, defaults );
		OUTPUT_FLOAT( gGlobalPreferences, StickMaxDeadzone, defaults );
		OUTPUT_INT( gGlobalPreferences, ViewportType, defaults );
		OUTPUT_BOOL( gGlobalPreferences, TVEnable, defaults );
		OUTPUT_BOOL( gGlobalPreferences, TVLaced, defaults );
		OUTPUT_INT( gGlobalPreferences, TVType, defaults );

#ifndef DAEDALUS_PUBLIC_RELEASE
		OUTPUT_BOOL( gGlobalPreferences, CustomBlendModes, defaults );
		OUTPUT_BOOL( gGlobalPreferences, SkipSplash, defaults );
		OUTPUT_BOOL( gGlobalPreferences, LogMicrocodes, defaults );
#endif

		fprintf( fh, "\n\n" ); //Spacer to go before Rom Settings

		for ( PreferencesMap::const_iterator it = mPreferences.begin(); it != mPreferences.end(); ++it )
		{
			OutputSectionDetails( it->first, it->second, fh );
		}

		fclose( fh );
		mDirty = false;
	}
}

//*****************************************************************************
// Retreive the preferences for the specified rom. Returns false if the rom is
// not in the database
//*****************************************************************************
bool	IPreferences::GetRomPreferences( const RomID & id, SRomPreferences * preferences ) const
{
	PreferencesMap::const_iterator	it( mPreferences.find( id ) );
	if ( it != mPreferences.end() )
	{
		*preferences = it->second;
		return true;
	}
	else
	{
		return false;
	}
}

//*****************************************************************************
// Update the preferences for the specified rom - creates a new entry if necessary
//*****************************************************************************
void	IPreferences::SetRomPreferences( const RomID & id, const SRomPreferences & preferences )
{
	PreferencesMap::iterator	it( mPreferences.find( id ) );
	if ( it != mPreferences.end() )
	{
		it->second = preferences;
	}
	else
	{
		mPreferences[ id ] = preferences;
	}

	mDirty = true;
}

//*****************************************************************************
//
//*****************************************************************************
SGlobalPreferences::SGlobalPreferences()
:	DisplayFramerate( 0 )
,	SoftwareClipping( true )
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
,	HighlightInexactBlendModes( false )
#endif
,	BatteryWarning( false )
,	LargeROMBuffer( true )
,	ForceTextureFilter( FORCE_DEFAULT_FILTER )
,	GuiType( COVERFLOW )
,	GuiColor( BLACK )
,	StickMinDeadzone( 0.28f )
,	StickMaxDeadzone( 1.0f )
,	ViewportType( VT_FULLSCREEN )
,	TVEnable( false )
,	TVLaced( false )
,	TVType( TT_4_3 )
#ifndef DAEDALUS_PUBLIC_RELEASE
,	CustomBlendModes( true )
,	SkipSplash( false )
,	LogMicrocodes( false )
#endif
{
}

//*****************************************************************************
//
//*****************************************************************************
void SGlobalPreferences::Apply() const
{

}

//*****************************************************************************
//
//*****************************************************************************
SRomPreferences::SRomPreferences()
	:	PatchesEnabled( true )
	,	SpeedSyncEnabled( false )
	,	DynarecEnabled( true )
	,	DynarecStackOptimisation( true )
	,	DynarecLoopOptimisation( true )
	,	DoubleDisplayEnabled( true )
	,	SimulateDoubleDisabled( false )
	,	CleanSceneEnabled( false )
	,	CheckN64FPUsageDisable( false )
	,	AudioRateMatch( false )
	,	FogEnabled( false )
	,	CheckTextureHashFrequency( THF_DISABLED )
	,	Frameskip( FV_DISABLED )
	,	AudioEnabled( APM_DISABLED )
//	,	AudioAdaptFrequency( false )
	,	ZoomX( 1.0f )
	,	ControllerIndex( 0 )
{
}

//*****************************************************************************
//
//*****************************************************************************
void SRomPreferences::Reset()
{
	PatchesEnabled = true;
	SpeedSyncEnabled = false;
	DynarecEnabled = true;
	DynarecStackOptimisation = true;
	DynarecLoopOptimisation = true;
	DoubleDisplayEnabled = true;
	SimulateDoubleDisabled = false;
	CleanSceneEnabled = false;
	CheckN64FPUsageDisable = false;
	AudioRateMatch = false;
	FogEnabled = false;
	CheckTextureHashFrequency = THF_DISABLED;
	Frameskip = FV_DISABLED;
	AudioEnabled = APM_DISABLED;
//	AudioAdaptFrequency = false;
	ZoomX = 1.0f;
	ControllerIndex = 0;
}

//*****************************************************************************
//
//*****************************************************************************
void	SRomPreferences::Apply() const
{
	gOSHooksEnabled		= PatchesEnabled;
	gSpeedSyncEnabled	= SpeedSyncEnabled;
	gDynarecEnabled		= g_ROM.settings.DynarecSupported && DynarecEnabled;
	gDynarecStackOptimisation	= g_ROM.settings.DynarecStackOptimisation && DynarecStackOptimisation;
	gDynarecLoopOptimisation	= g_ROM.settings.DynarecLoopOptimisation && DynarecLoopOptimisation;
	gDoubleDisplayEnabled = g_ROM.settings.DoubleDisplayEnabled || DoubleDisplayEnabled;
	gSimulateDoubleDisabled = g_ROM.settings.SimulateDoubleDisabled || SimulateDoubleDisabled;
	gCleanSceneEnabled = g_ROM.settings.CleanSceneEnabled || CleanSceneEnabled;
	gCheckN64FPUsageDisable = g_ROM.settings.CheckN64FPUsageDisable || CheckN64FPUsageDisable;
	gAudioRateMatch = g_ROM.settings.AudioRateMatch || AudioRateMatch;
	gFogEnabled = g_ROM.settings.FogEnabled || FogEnabled;
	gCheckTextureHashFrequency = ROM_GetTexureHashFrequencyAsFrames( CheckTextureHashFrequency );
	gFrameskipValue = Frameskip;
	gZoomX = ZoomX;

	gAudioPluginEnabled = AudioEnabled;
//	gAdaptFrequency = AudioAdaptFrequency;

	gControllerIndex = ControllerIndex;							//Used during ROM initialization 
	CInputManager::Get()->SetConfiguration( ControllerIndex );  //Used after initialization
}


//*****************************************************************************
//
//*****************************************************************************
static const u32 gTextureHashFreqeuncies[] =
{
	0,	//THF_DISABLED = 0,
	1,	//THF_EVERY_FRAME,
	2,	//THF_EVERY_2,
	3,	//THF_EVERY_3,
	4,	//THF_EVERY_4,
	5,	//THF_EVERY_5,
	10,	//THF_EVERY_10,
	15,	//THF_EVERY_15,
	20,	//THF_EVERY_20,
	30,	//THF_EVERY_30,
};

static const char * const gTextureHashFreqeuncyDescriptions[] =
{
	"Disabled",			//THF_DISABLED = 0,
	"Every Frame",		//THF_EVERY_FRAME,
	"Every 2 Frames",	//THF_EVERY_2,
	"Every 3 Frames",	//THF_EVERY_3,
	"Every 4 Frames",	//THF_EVERY_4,
	"Every 5 Frames",	//THF_EVERY_5,
	"Every 10 Frames",	//THF_EVERY_10,
	"Every 15 Frames",	//THF_EVERY_15,
	"Every 20 Frames",	//THF_EVERY_20,
	"Every 30 Frames",	//THF_EVERY_30,
};

//*****************************************************************************
//
//*****************************************************************************
u32	ROM_GetTexureHashFrequencyAsFrames( ETextureHashFrequency thf )
{
	if(thf >= 0 && thf < NUM_THF)
	{
		return gTextureHashFreqeuncies[ thf ];
	}

	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
ETextureHashFrequency	ROM_GetTextureHashFrequencyFromFrames( u32 frames )
{
	for( u32 i = 0; i < NUM_THF; ++i )
	{
		if( frames <= gTextureHashFreqeuncies[ i ] )
		{
			return ETextureHashFrequency( i );
		}
	}

	return THF_EVERY_30;	// Return the maximum
}


//*****************************************************************************
//
//*****************************************************************************
const char * ROM_GetTextureHashFrequencyDescription( ETextureHashFrequency thf )
{
	if(thf >= 0 && thf < NUM_THF)
	{
		return gTextureHashFreqeuncyDescriptions[ thf ];
	}

	return "?";
}

//*****************************************************************************
//
//*****************************************************************************
u32	ROM_GetFrameskipValueAsInt( EFrameskipValue value )
{
	return value;
}

//*****************************************************************************
//
//*****************************************************************************
EFrameskipValue	ROM_GetFrameskipValueFromInt( u32 value )
{
//  This is always False
//	if( value < FV_DISABLED )
//		return FV_DISABLED;

//	if( value > FV_10 )
//		return FV_10;

	return EFrameskipValue( value );
}

//*****************************************************************************
//
//*****************************************************************************
const char *			ROM_GetFrameskipDescription( EFrameskipValue value )
{
	switch( value )
	{
	case FV_DISABLED:		return "Disabled";
	case FV_AUTO:			return "Auto";
	case FV_1:				return "1";
	case FV_2:				return "2";
	case FV_3:				return "3";
	case FV_4:				return "4";
	case FV_5:				return "5";
	case FV_6:				return "6";
	case FV_7:				return "7";
	case FV_8:				return "8";
	case FV_9:				return "9";
#ifndef DAEDALUS_PUBLIC_RELEASE
	case FV_99:				return "99";
#endif
	case NUM_FRAMESKIP_VALUES:
		break;
	}
	DAEDALUS_ERROR( "Unhandled frameskip value" );
	return "?";
}

