#ifndef SENT_ANALYZER_SETTINGS
#define SENT_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>
#include <AnalyzerHelpers.h>

using namespace std;

class LOGICAPI AnalyzerSettingInterfaceDouble : public AnalyzerSettingInterfaceText
{
public:
	AnalyzerSettingInterfaceDouble(void) {}
	virtual ~AnalyzerSettingInterfaceDouble() {}

	double GetDouble() 
	{
		double retval;
		const char* parent_text = AnalyzerSettingInterfaceText::GetText(); 
		retval = stod(parent_text);
		return retval;
	}
	void SetDouble(double data)
	{
		AnalyzerSettingInterfaceText::SetText(to_string(data).c_str());
	}
};

class SENTAnalyzerSettings : public AnalyzerSettings
{
public:
	SENTAnalyzerSettings();
	virtual ~SENTAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();


	Channel mInputChannel;
	double tick_time;
	bool pausePulseEnabled;
	U32 numberOfDataNibbles;
	bool legacyCRC;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceDouble >	    tickTimeInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		pausePulseInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	dataNibblesInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		legacyCRCInterface;
};

#endif //SENT_ANALYZER_SETTINGS
