#include "SENTAnalyzer.h"
#include "SENTAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <math.h>

#define STATUS_NIBBLE_NUMBER 	(1)
#define PAUSE_PULSE_NUMBER 		(crc_nibble_number + 1)

SENTAnalyzer::SENTAnalyzer()
:	Analyzer2(),
	mSettings( new SENTAnalyzerSettings() ),
	mSimulationInitilized( false ),
	nibble_counter(0),
	framelist(),
	corrected_samples_per_tick(0)
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
	crc_nibble_number = STATUS_NIBBLE_NUMBER + mSettings->numberOfDataNibbles + 1;
	if (mSettings->pausePulseEnabled)
	{
		number_of_nibbles = PAUSE_PULSE_NUMBER + 1;
	}
	else
	{
		number_of_nibbles = crc_nibble_number + 1;
	}
}

/** Function for calculation the SENT CRC4
 *
 *  @returns 	U8	the calculated CRC4 on the data of the previous SENT frame.
 */
U8 SENTAnalyzer::CalculateCRC()
{
	U8 crc4_table [16] = {0, 13, 7, 10, 14, 3, 9, 4, 1, 12, 6, 11, 15, 2, 8, 5};
	U8 CheckSum16 = 5;

	/* We increment the start pointer by 2 to skip sync and status nibbles.
	 * In the end condition we decrement the end pointer by 1 to omit the CRC nibble */
	for(std::vector<Frame>::iterator it = framelist.begin() + 2; it != framelist.end() - (number_of_nibbles - crc_nibble_number); it++)
	{
		CheckSum16 = it->mData1 ^ crc4_table[CheckSum16];
	}
	if(!mSettings->legacyCRC) {
		CheckSum16 = 0 ^ crc4_table[CheckSum16];
	}
	return CheckSum16;
}

/** This function will create a new Frame with the data, type and timing info provided and commit it to the current packet
 *
 *  @param [in] 	data 	The data to be stored in the frame
 *  @param [in] 	type 	The pulse type (sync, status, fc, ...)
 *  @param [in] 	start 	The sample number of the start of the frame
 *  @param [in] 	end 	The sample number of the end of the frame
 */
void SENTAnalyzer::addSENTPulse(U16 data, enum SENTNibbleType type, U64 start, U64 end)
{
	Frame frame;
	frame.mData1 = data;
	frame.mFlags = 0;
	frame.mType = type;
	frame.mStartingSampleInclusive = start;
	frame.mEndingSampleInclusive = end;
	framelist.push_back(frame);
}

void SENTAnalyzer::addErrorFrame(U16 data, U64 start, U64 end, SENTErrorType error_type)
{
	Frame frame;
	frame.mData1 = data;
	frame.mFlags = DISPLAY_AS_ERROR_FLAG | (1 << error_type);
	frame.mType = Error;
	frame.mStartingSampleInclusive = start;
	frame.mEndingSampleInclusive = end;

	mResults->AddFrame(frame);
	mResults->CommitResults();
	ReportProgress(frame.mEndingSampleInclusive);
}

/** Callback function for detection of sync pulse
 *
 *  This function will do some sanity checks on the data gathered during the
 *  last SENT frame. It will check
 *
 *  - The amount of nibbles
 *  - The CRC
 *
 *  If any of these checks fail, the SENT frame is dropped.
 */
void SENTAnalyzer::syncPulseDetected()
{
	if(framelist.size() == number_of_nibbles)
	{
		U8 expected_crc = CalculateCRC();
		bool crc_correct = (framelist.at(crc_nibble_number).mData1 == expected_crc);
		for(std::vector<Frame>::iterator it = framelist.begin(); it != framelist.end(); it++) {
			if(!crc_correct && (it->mType == CRCNibble)) {
				addErrorFrame(expected_crc, it->mStartingSampleInclusive, it->mEndingSampleInclusive, CrcError);
			} else {
				mResults->AddFrame( *it );
				mResults->CommitResults();
				ReportProgress( it->mEndingSampleInclusive );
			}
		}
		mResults->CommitPacketAndStartNewPacket();
	}
	else if( framelist.size() > 0 )
	{
		addErrorFrame(framelist.size(), framelist.begin()->mStartingSampleInclusive, framelist.begin()->mEndingSampleInclusive, NibbleNumberError);
		mResults->CommitPacketAndStartNewPacket();
	}
	else /* Framelist is empty. This occurs when the first pulse is already a sync pulse */
	{
		mResults->CancelPacketAndStartNewPacket();
	}
	framelist.clear();
}

/** Function for correcting the tick time
 *
 *  This function should be called whenever a sync pulse is detected (given the specified clock tolerances)
 *  Then, based on the amount of samples during that period, the amount of samples per tick is recalculated
 *  by dividing this number by the amount of ticks per sync pulse (56).
 *
 *  @param[in] 	number_of_samples	The amount of samples taken during the sync pulse period
 */
void SENTAnalyzer::correctTickTime(U32 number_of_samples)
{
	corrected_samples_per_tick = round(number_of_samples / 56.0);
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
	/* Sync pulse should be 56 ticks. Given a 20% margin, it should fall in range [45:67] */
	if(number_of_ticks >= 45 && number_of_ticks <= 67)
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
 */
void SENTAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();

	/* Based on the configured tick time and the sampling rate, determine the amount of samples per tick */
	U32 theoretical_samples_per_ticks = mSampleRateHz * (mSettings->tick_time_half_us / 2.0) / 1000000;

	/* This is initialized to the theoretical samples per tick, and is adjusted on every
	 * received sync pulse */
	corrected_samples_per_tick = theoretical_samples_per_ticks;

	/* Request the channel we are using for the analysis */
	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	/* Advance the "cursor" to the first falling edge */
	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();
	mSerial->AdvanceToNextEdge();

	U64 starting_sample;

	framelist = std::vector<Frame>();
	nibble_counter = 0;

	for( ; ; )
	{
		enum SENTNibbleType nibble_type = Unknown;

		/* We capture the sample number on the falling edge, for reference */
		starting_sample = mSerial->GetSampleNumber();
		/* Then, we advance 2 edges, so we end up on the next falling edge */
		mSerial->AdvanceToNextEdge();
		mSerial->AdvanceToNextEdge();

		/* Calculate the number of samples in this period */
		U32 number_of_samples = mSerial->GetSampleNumber() - starting_sample;
		/* Now, based on the difference in amount of samples between the current falling edge
		   and the reference one, we can determine the amount of ticks that have passed */
		U16 theoretical_number_of_ticks = round((float)number_of_samples / theoretical_samples_per_ticks);
		U16 corrected_number_of_ticks = round((float)number_of_samples / corrected_samples_per_tick);

		/* Based on the amount of ticks and a nibble counter, we can attempt to determine
   		   what type of pulse was encountered */

		/* First check if the detected pulse is a sync pulse
		   As a sync pulse indicates the start of a new SENT frame, the previous
		   Packet is closed and committed and a new Packet is started.
		   */
		if(isPulseSyncPulse(theoretical_number_of_ticks))
		{
			/* If it's a valid sync pulse, calculate the corrected tick time */
			correctTickTime(number_of_samples);
			/* Then close the previous frame and check if it was valid */
			syncPulseDetected();
			nibble_type = SyncPulse;
			corrected_number_of_ticks = 56;
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
		else if (corrected_number_of_ticks > 11 && corrected_number_of_ticks < 28)
		{
			if(nibble_counter == STATUS_NIBBLE_NUMBER)
			{
				nibble_type = StatusNibble;
				/* We extract the actual data by subtracting the number of ticks by 12 */
				corrected_number_of_ticks -= 12;
			}
			else if (nibble_counter > STATUS_NIBBLE_NUMBER && nibble_counter < crc_nibble_number)
			{
				nibble_type = FCNibble;
				/* We extract the actual data by subtracting the number of ticks by 12 */
				corrected_number_of_ticks -= 12;
			}
			else if(nibble_counter == crc_nibble_number)
			{
				nibble_type = CRCNibble;
				/* We extract the actual data by subtracting the number of ticks by 12 */
				corrected_number_of_ticks -= 12;
			}
		}
		/* If none of the above conditions are met, the frame is marked as "unknown" */
		else {
			nibble_counter = 0;
		} /* Do nothing. No valid frame was detected */

		nibble_counter++;

		/* Commit the frame to the database and to the current packet */
		addSENTPulse(corrected_number_of_ticks, nibble_type, starting_sample + 1, mSerial->GetSampleNumber());
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