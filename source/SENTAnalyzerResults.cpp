#include "SENTAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SENTAnalyzer.h"
#include "SENTAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

std::map<enum SENTNibbleType, std::string> TypeMap;

SENTAnalyzerResults::SENTAnalyzerResults( SENTAnalyzer* analyzer, SENTAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
	InitializeTypeMap();
}

SENTAnalyzerResults::~SENTAnalyzerResults()
{
}

void SENTAnalyzerResults::InitializeTypeMap(void)
{
	TypeMap[SyncPulse] = "SYNC_PULSE";
	TypeMap[StatusNibble] = "STATUS_NIBBLE";
	TypeMap[FCNibble] = "FC_NIBBLE";
	TypeMap[CRCNibble] = "CRC_NIBBLE";
	TypeMap[PausePulse] = "PAUSE_PULSE";
	TypeMap[Unknown] = "UNKNOWN";
	TypeMap[Error] = "Error";
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
		case Error:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 128 );
			ss << "Error. Number of nibbles detected: " << number_str;
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
	U32 number_of_packets = GetNumPackets();
	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	std::ofstream file_stream( file, std::ios::out );

	file_stream << "Time [s],Value" << std::endl;

	for( U32 i=0; i < number_of_packets; i++ )
	{
		U64 frameid;
		U64 frameid_end;

		GetFramesContainedInPacket(i, &frameid, &frameid_end);

		while (frameid <= frameid_end)
		{
			Frame frame = GetFrame(frameid);
			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

			file_stream << time_str << ", " << number_str << ", " << TypeMap[(enum SENTNibbleType)frame.mType] << std::endl;

			frameid++;
		}

		if( UpdateExportProgressAndCheckForCancel( i, number_of_packets ) == true )
		{
			file_stream.close();
			return;
		}
		file_stream << "------, ------, -----" << std::endl;
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