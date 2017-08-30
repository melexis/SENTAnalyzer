#include "SENTSimulationDataGenerator.h"
#include "SENTAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

SENTSimulationDataGenerator::SENTSimulationDataGenerator()
{
}

SENTSimulationDataGenerator::~SENTSimulationDataGenerator()
{
}

void SENTSimulationDataGenerator::Initialize( U32 simulation_sample_rate, SENTAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mSerialSimulationData.SetChannel( settings->mInputChannel );
	mSerialSimulationData.SetSampleRate( simulation_sample_rate );
	mSerialSimulationData.SetInitialBitState( BIT_HIGH );
}

U32 SENTSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mSerialSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateSerialByte();
	}

	*simulation_channel = &mSerialSimulationData;
	return 1;
}

void SENTSimulationDataGenerator::AddNibble(U16 number_of_ticks, U16 samples_per_tick)
{
	U16 number_of_high_ticks = number_of_ticks - 5;
	mSerialSimulationData.Transition();
	mSerialSimulationData.Advance( samples_per_tick * 5);
	mSerialSimulationData.Transition();
	mSerialSimulationData.Advance( samples_per_tick * number_of_high_ticks);
}

void SENTSimulationDataGenerator::CreateSerialByte()
{
	U32 samples_per_tick = mSimulationSampleRateHz * mSettings->tick_time_us / 1000000;

	mSerialSimulationData.Advance( samples_per_tick * 10 );

	/* Calibration pulse */
	AddNibble(56, samples_per_tick);
	/* Status nibble */
	AddNibble(12, samples_per_tick);
	/* Fast channel 1 */
	AddNibble(27, samples_per_tick);
	AddNibble(17, samples_per_tick);
	AddNibble(22, samples_per_tick);
	/* Fast channel 2 */
	AddNibble(14, samples_per_tick);
	AddNibble(20, samples_per_tick);
	AddNibble(12, samples_per_tick);
	/* CRC */
	AddNibble(21, samples_per_tick);
	/* Pause pulse */
	AddNibble(100, samples_per_tick);
}
