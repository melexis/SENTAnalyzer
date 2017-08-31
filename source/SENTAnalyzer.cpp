#include "SENTAnalyzer.h"
#include "SENTAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <math.h>

#define STATUS_NIBBLE_NUMBER (1)
#define CRC_NIBBLE_NUMBER (8)
#define PAUSE_PULSE_NUMBER (9)

SENTAnalyzer::SENTAnalyzer()
:	Analyzer2(),
	mSettings( new SENTAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

SENTAnalyzer::~SENTAnalyzer()
{
	KillThread();
}

void SENTAnalyzer::SetupResults()
{
	mResults.reset( new SENTAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void SENTAnalyzer::addSENTFrame(U16 ticks, enum SENTNibbleType type, U64 start, U64 end)
{
	Frame frame;
	frame.mData1 = ticks;
	frame.mFlags = 0;
	frame.mType = type;
	frame.mStartingSampleInclusive = start;
	frame.mEndingSampleInclusive = end;
	mResults->AddFrame( frame );
	mResults->CommitResults();
	ReportProgress( frame.mEndingSampleInclusive );
}

void SENTAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	U32 theoretical_samples_per_ticks = mSampleRateHz * (mSettings->tick_time_half_us / 2.0) / 1000000;

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();
	mSerial->AdvanceToNextEdge();

	U64 starting_sample;
	U8 nibble_counter = 0;

	for( ; ; )
	{
		enum SENTNibbleType nibble_type = Unknown;
		starting_sample = mSerial->GetSampleNumber();
		mSerial->AdvanceToNextEdge();
		mSerial->AdvanceToNextEdge();
		float number_of_ticks = (mSerial->GetSampleNumber() - starting_sample) / theoretical_samples_per_ticks;

		if(number_of_ticks > 55 && number_of_ticks < 57)
		{
			mResults->CommitPacketAndStartNewPacket();
			nibble_type = SyncPulse;
			nibble_counter = 0;
		}
		else if (nibble_counter == PAUSE_PULSE_NUMBER)
		{
			nibble_type = PausePulse;
		}
		else if (number_of_ticks > 11 && number_of_ticks < 28)
		{
			if(nibble_counter == STATUS_NIBBLE_NUMBER)
			{
				nibble_type = StatusNibble;
				number_of_ticks = round(number_of_ticks) - 12;
			}
			else if (nibble_counter > STATUS_NIBBLE_NUMBER && nibble_counter < CRC_NIBBLE_NUMBER)
			{
				nibble_type = FCNibble;
				number_of_ticks = round(number_of_ticks) - 12;
			}
			else if(nibble_counter == CRC_NIBBLE_NUMBER)
			{
				nibble_type = CRCNibble;
				number_of_ticks = round(number_of_ticks) - 12;
			}
		}
		else {} /* Do nothing. No valid frame was detected */

		nibble_counter++;
		addSENTFrame(number_of_ticks,
					 nibble_type,
				     starting_sample + 1,
					 mSerial->GetSampleNumber());

	}
}

bool SENTAnalyzer::NeedsRerun()
{
	return false;
}

U32 SENTAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SENTAnalyzer::GetMinimumSampleRateHz()
{
	return 2000000 / (mSettings->tick_time_half_us / 2.0);
}

const char* SENTAnalyzer::GetAnalyzerName() const
{
	return "SENT (SAE J2716)";
}

const char* GetAnalyzerName()
{
	return "SENT (SAE J2716)";
}

Analyzer* CreateAnalyzer()
{
	return new SENTAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}