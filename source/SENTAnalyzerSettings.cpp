#include "SENTAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


SENTAnalyzerSettings::SENTAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	tick_time_half_us(3),
	pausePulseEnabled(true),
	numberOfDataNibbles(6)
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard SENT (SAE J2716)" );
	mInputChannelInterface->SetChannel( mInputChannel );

	tickTimeInterface.reset( new AnalyzerSettingInterfaceInteger() );
	tickTimeInterface->SetTitleAndTooltip( "tick time (half us)",  "Specify the SENT tick time in half microseconds" );
	tickTimeInterface->SetMax( 100 );
	tickTimeInterface->SetMin( 1);
	tickTimeInterface->SetInteger( tick_time_half_us );

	pausePulseInterface.reset( new AnalyzerSettingInterfaceBool() );
	pausePulseInterface->SetTitleAndTooltip( "Pause pulse",  "Specify whether pause pulse is enabled or not" );
	pausePulseInterface->SetValue(pausePulseEnabled);

	dataNibblesInterface.reset( new AnalyzerSettingInterfaceInteger() );
	dataNibblesInterface->SetTitleAndTooltip( "Number of data nibbles", "Specify the total number of fast channel data nibbles" );
	dataNibblesInterface->SetMax( 6 );
	dataNibblesInterface->SetMin( 0);
	dataNibblesInterface->SetInteger( numberOfDataNibbles );

	AddInterface( mInputChannelInterface.get() );
	AddInterface( tickTimeInterface.get() );
	AddInterface( pausePulseInterface.get() );
	AddInterface( dataNibblesInterface.get() );

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
	tick_time_half_us = tickTimeInterface->GetInteger();
	pausePulseEnabled = pausePulseInterface->GetValue();
	numberOfDataNibbles = dataNibblesInterface->GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "SENT (SAE J2716)", true );

	return true;
}

void SENTAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel(mInputChannel);
	tickTimeInterface->SetInteger(tick_time_half_us);
	pausePulseInterface->SetValue(pausePulseEnabled);
	dataNibblesInterface->SetInteger(numberOfDataNibbles);
}

void SENTAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> tick_time_half_us;
	text_archive >> pausePulseEnabled;
	text_archive >> numberOfDataNibbles;

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
	text_archive << numberOfDataNibbles;

	return SetReturnString( text_archive.GetString() );
}
