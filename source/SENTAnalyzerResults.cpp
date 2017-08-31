#include "SENTAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SENTAnalyzer.h"
#include "SENTAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>

SENTAnalyzerResults::SENTAnalyzerResults( SENTAnalyzer* analyzer, SENTAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

SENTAnalyzerResults::~SENTAnalyzerResults()
{
}

std::string SENTAnalyzerResults::FrameToString(Frame frame, DisplayBase display_base)
{
	std::string retval = "";
	std::stringstream ss;
	char number_str[128];
	switch( frame.mType )
	{
		case SyncPulse:
			ss << "Sync pulse: ";
			break;
		case StatusNibble:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );
			ss << "Status nibble: " << number_str;
			break;
		case FCNibble:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );
			ss << "Fast channel data: " << number_str;
			break;
		case CRCNibble:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );
			ss << "CRC data: " << number_str;
			break;
		case PausePulse:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );
			ss << "Pause pulse length: " << number_str;
			break;
		case Unknown:
			ss << "Unknown";
			break;
	}
	return ss.str();
}

void SENTAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	//we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by AddChannelBubblesWillAppearOn)
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	AddResultString(FrameToString(frame, display_base).c_str());
}

void SENTAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );

		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << "," << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void SENTAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	ClearTabularText();

	Frame frame = GetFrame( frame_index );

	AddTabularText(FrameToString(frame, display_base).c_str());
}

void SENTAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void SENTAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}