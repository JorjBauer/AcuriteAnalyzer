#ifndef ACURITE_ANALYZER_SETTINGS
#define ACURITE_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class AcuriteAnalyzerSettings : public AnalyzerSettings
{
public:
	AcuriteAnalyzerSettings();
	virtual ~AcuriteAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
};

#endif //ACURITE_ANALYZER_SETTINGS
