#ifndef SENT_ANALYZER_RESULTS
#define SENT_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class SENTAnalyzer;
class SENTAnalyzerSettings;

enum SENTNibbleType { SyncPulse, StatusNibble, FCNibble, CRCNibble, PausePulse, Unknown, Error};

class SENTAnalyzerResults : public AnalyzerResults
{
public:
	SENTAnalyzerResults( SENTAnalyzer* analyzer, SENTAnalyzerSettings* settings );
	virtual ~SENTAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	SENTAnalyzerSettings* mSettings;
	SENTAnalyzer* mAnalyzer;
	std::string FrameToString(Frame frame, DisplayBase display_base);
	void InitializeTypeMap(void);
};

#endif //SENT_ANALYZER_RESULTS
