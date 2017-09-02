#include "SENTAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


SENTAnalyzerSettings::SENTAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	tick_time_half_us( 3.0 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard SENT (SAE J2716)" );
	mInputChannelInterface->SetChannel( mInputChannel );

	TickTimeInterface.reset( new AnalyzerSettingInterfaceInteger() );
	TickTimeInterface->SetTitleAndTooltip( "tick time (half us)",  "Specify the SENT tick time in half microseconds" );
	TickTimeInterface->SetMax( 100 );
	TickTimeInterface->SetMin( 1);
	TickTimeInterface->SetInteger( tick_time_half_us );

	PausePulseInterface.reset( new AnalyzerSettingInterfaceBool() );
	PausePulseInterface->SetTitleAndTooltip( "Pause pulse",  "Specify whether pause pulse is enabled or not" );
	PausePulseInterface->SetValue(false);

	AddInterface( mInputChannelInterface.get() );
	AddInterface( TickTimeInterface.get() );
	AddInterface( PausePulseInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

SENTAnalyzerSettings::~SENTAnalyzerSettings()
{
}

bool SENTAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	tick_time_half_us = TickTimeInterface->GetInteger();
	pausePulseEnabled = PausePulseInterface->GetValue();

	ClearChannels();
	AddChannel( mInputChannel, "SENT (SAE J2716)", true );

	return true;
}

void SENTAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel(mInputChannel);
	TickTimeInterface->SetInteger(tick_time_half_us);
	PausePulseInterface->SetValue(pausePulseEnabled);
}

void SENTAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> tick_time_half_us;
	text_archive >> pausePulseEnabled;


	ClearChannels();
	AddChannel( mInputChannel, "SENT (SAE J2716)", true );

	UpdateInterfacesFromSettings();
}

const char* SENTAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << tick_time_half_us;
	text_archive << pausePulseEnabled;

	return SetReturnString( text_archive.GetString() );
}
