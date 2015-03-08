#include "AcuriteAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


AcuriteAnalyzerSettings::AcuriteAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard Acurite" );
	mInputChannelInterface->SetChannel( mInputChannel );

	AddInterface( mInputChannelInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

AcuriteAnalyzerSettings::~AcuriteAnalyzerSettings()
{
}

bool AcuriteAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();

	ClearChannels();
	AddChannel( mInputChannel, "Acurite", true );

	return true;
}

void AcuriteAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
}

void AcuriteAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;

	ClearChannels();
	AddChannel( mInputChannel, "Acurite", true );

	UpdateInterfacesFromSettings();
}

const char* AcuriteAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;

	return SetReturnString( text_archive.GetString() );
}
