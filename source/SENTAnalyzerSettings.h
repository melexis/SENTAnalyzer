#ifndef SENT_ANALYZER_SETTINGS
#define SENT_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>
#include <AnalyzerHelpers.h>

using namespace std;

class LOGICAPI AnalyzerSettingInterfaceDouble : public AnalyzerSettingInterfaceText
{
public:
	AnalyzerSettingInterfaceDouble(void) {
		min_value = DBL_MIN;
		max_value = DBL_MAX;
	}
	virtual ~AnalyzerSettingInterfaceDouble() {}

	double GetDouble(bool* error, const char ** error_message)
	{
		*error = true;
		*error_message = "Some error message";
		double retval = 0.0;
		const char* parent_text = AnalyzerSettingInterfaceText::GetText();
		try {
			retval = stod(parent_text);
		}
		catch (...) {
			*error_message = "Not a valid decimal (double) type argument.";
			*error = false;
			return 0.0;
		}

		if (retval < min_value) {
			*error_message = "Input value too small";
			*error = false;
		} else if (retval > max_value) {
			*error_message = "Input value too big";
			*error = false;
		}
		return retval;
	}

	void SetDouble(double data)
	{
		AnalyzerSettingInterfaceText::SetText(to_string(data).c_str());
	}
	void setMax(double max)
	{
		max_value = max;
	}
	void setMin(double min)
	{
		min_value = min;
	}
private:
	double max_value;
	double min_value;
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
