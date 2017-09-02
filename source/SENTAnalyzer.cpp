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
	mSimulationInitilized( false ),
	nibble_counter(0)
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

/** This function will create a new Frame with the data, type and timing info provided and commit it to the current packet
 *
 *  @param [in] 	data 	The data to be stored in the frame
 *  @param [in] 	type 	The pulse type (sync, status, fc, ...)
 *  @param [in] 	start 	The sample number of the start of the frame
 *  @param [in] 	end 	The sample number of the end of the frame
 */
void SENTAnalyzer::addSENTFrame(U16 data, enum SENTNibbleType type, U64 start, U64 end)
{
	Frame frame;
	frame.mData1 = data;
	frame.mFlags = 0;
	frame.mType = type;
	frame.mStartingSampleInclusive = start;
	frame.mEndingSampleInclusive = end;
	mResults->AddFrame( frame );
	mResults->CommitResults();
	ReportProgress( frame.mEndingSampleInclusive );
}

/** Function for determining if the detected pulse is a sync pulse or not
 *
 *	First, the function checks if the detected pulse is 56 ticks wide.
 *	This narrows the choice down to 2 options: sync pulse and pause pulse
 *	(the other pulses are between 12 and 27 ticks wide)
 *
 *	Then, in order to determine whether it's a pause pulse or not, we check
 *	whether we were expecting a pause pulse to begin with. If so, we take the
 *	naive approach and assume it's a pause pulse. If not, we say it's a valid
 *	sync pulse.
 *
 *  @retval 	true	The detected pulse is a sync pulse
 *  @retval     false 	The detected pulse is not a sync pulse
 */
bool SENTAnalyzer::isPulseSyncPulse(U16 number_of_ticks)
{
	bool retval = false;
	if(number_of_ticks == 56)
	{
		if ((mSettings->pausePulseEnabled) && (nibble_counter == PAUSE_PULSE_NUMBER))
		{
			retval = false;
		}
		else
		{
			retval = true;
		}
	}
	return retval;
}

/** Main signal processing function
 *
 *  This function will actually attempt to decode the SENT frames.
 *  When successful, a single SENT frame (sync + status + fc data + crc + pause)
 *  will be stored as a single Packet containing multiple frames (1 frame per "nibble")
 *
 *  - As the sync and pause pulse don't actually convey any data, the frames for these pulses
 *    contain the total amount of ticks they consume
 *  - For status, FC and CRC nibbles, the actual encoded value is stored. This means the
 *    total amount of ticks minus 12.\
 *
 *  TODO: Serial messaging
 *  TODO: configurable fast channel nibble amount + optional pause pulse
 */
void SENTAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();

	/* Based on the configured tick time and the sampling rate, determine the amount of samples per tick */
	U32 theoretical_samples_per_ticks = mSampleRateHz * (mSettings->tick_time_half_us / 2.0) / 1000000;

	/* Request the channel we are using for the analysis */
	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	/* Advance the "cursor" to the first falling edge */
	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();
	mSerial->AdvanceToNextEdge();

	U64 starting_sample;

	for( ; ; )
	{
		enum SENTNibbleType nibble_type = Unknown;

		/* We capture the sample number on the falling edge, for reference */
		starting_sample = mSerial->GetSampleNumber();
		/* Then, we advance 2 edges, so we end up on the next falling edge */
		mSerial->AdvanceToNextEdge();
		mSerial->AdvanceToNextEdge();

		/* Now, based on the difference in amount of samples between the current falling edge
		   and the reference one, we can determine the amount of ticks that have passed */
		U16 number_of_ticks = (mSerial->GetSampleNumber() - starting_sample) / theoretical_samples_per_ticks;

		/* Based on the amount of ticks and a nibble counter, we can attempt to determine
   		   what type of pulse was encountered */

		/* First check if the detected pulse is a sync pulse
		   As a sync pulse indicates the start of a new SENT frame, the previous
		   Packet is closed and committed and a new Packet is started.
		   */
		if(isPulseSyncPulse(number_of_ticks))
		{
			mResults->CommitPacketAndStartNewPacket();
			nibble_type = SyncPulse;
			nibble_counter = 0;
		}
		/* Then we check if the nibble counter indicates that we're expecting a pause pulse.
		   The pause pulse can take a larger range of sizes than any of the other pulse types,
		   so no sense in checking for the amount of ticks */
		else if (nibble_counter == PAUSE_PULSE_NUMBER)
		{
			nibble_type = PausePulse;
		}
		/* If not a pause pulse of sync pulse, it must be a data-carrying nibble.
		   The size range for these pulses is limited, so we check that first
		   Then, we check the nibble counter to see which type of nibble is expected */
		else if (number_of_ticks > 11 && number_of_ticks < 28)
		{
			if(nibble_counter == STATUS_NIBBLE_NUMBER)
			{
				nibble_type = StatusNibble;
				/* We extract the actual data by subtracting the number of ticks by 12 */
				number_of_ticks = round(number_of_ticks) - 12;
			}
			else if (nibble_counter > STATUS_NIBBLE_NUMBER && nibble_counter < CRC_NIBBLE_NUMBER)
			{
				nibble_type = FCNibble;
				/* We extract the actual data by subtracting the number of ticks by 12 */
				number_of_ticks = round(number_of_ticks) - 12;
			}
			else if(nibble_counter == CRC_NIBBLE_NUMBER)
			{
				nibble_type = CRCNibble;
				/* We extract the actual data by subtracting the number of ticks by 12 */
				number_of_ticks = round(number_of_ticks) - 12;
			}
		}
		/* If none of the above conditions are met, the frame is marked as "unknown" */
		else {} /* Do nothing. No valid frame was detected */

		nibble_counter++;

		/* Commit the frame to the database and to the current packet */
		addSENTFrame(number_of_ticks, nibble_type, starting_sample + 1,	 mSerial->GetSampleNumber());

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