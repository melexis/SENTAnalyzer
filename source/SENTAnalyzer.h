#ifndef SENT_ANALYZER_H
#define SENT_ANALYZER_H

#include <Analyzer.h>
#include "SENTAnalyzerResults.h"
#include "SENTSimulationDataGenerator.h"

class SENTAnalyzerSettings;
class ANALYZER_EXPORT SENTAnalyzer : public Analyzer2
{
public:
	SENTAnalyzer();
	virtual ~SENTAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< SENTAnalyzerSettings > mSettings;
	std::auto_ptr< SENTAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	SENTSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
	U8 nibble_counter;
	U16 crc_nibble_number;
	U16 number_of_nibbles;
	std::vector<Frame> framelist;
	U32 corrected_samples_per_tick;

	void syncPulseDetected();
	void correctTickTime(U32 number_of_samples);
	U8 CalculateCRC();
	bool isPulseSyncPulse(U16 number_of_ticks);
private:
	void addSENTPulse(U16 ticks, enum SENTNibbleType type, U64 start, U64 end);
	void addErrorFrame(U16 data, U64 start, U64 end, SENTNibbleType error_type);
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SENT_ANALYZER_H
