#include "SENTAnalyzer.h"
#include "SENTAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

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

void SENTAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	U32 theoretical_samples_per_ticks = mSampleRateHz * mSettings->tick_time_us / 1000000;

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();
	mSerial->AdvanceToNextEdge();

	U64 starting_sample;

	for( ; ; )
	{
		starting_sample = mSerial->GetSampleNumber();
		mSerial->AdvanceToNextEdge();
		mSerial->AdvanceToNextEdge();

		float number_of_ticks = (mSerial->GetSampleNumber() - starting_sample) / theoretical_samples_per_ticks;

		if(number_of_ticks > 55 && number_of_ticks < 57)
		{
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
			//we have a byte to save.
			Frame frame;
			frame.mData1 = 0;
			frame.mFlags = 0;
			frame.mStartingSampleInclusive = starting_sample;
			frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

			mResults->AddFrame( frame );
			mResults->CommitResults();
			ReportProgress( frame.mEndingSampleInclusive );
		}
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
	return 2000000 / mSettings->tick_time_us ;
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