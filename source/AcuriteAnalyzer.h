#ifndef ACURITE_ANALYZER_H
#define ACURITE_ANALYZER_H

#include <Analyzer.h>
#include "AcuriteAnalyzerResults.h"
#include "AcuriteSimulationDataGenerator.h"

class AcuriteAnalyzerSettings;
class ANALYZER_EXPORT AcuriteAnalyzer : public Analyzer
{
public:
	AcuriteAnalyzer();
	virtual ~AcuriteAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< AcuriteAnalyzerSettings > mSettings;
	std::auto_ptr< AcuriteAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	AcuriteSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Acurite analysis vars:
	U32 mLastPulse;
	U32 mSampleRateHz;
	U32 mSampleRateMs;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //ACURITE_ANALYZER_H
