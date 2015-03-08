#ifndef ACURITE_SIMULATION_DATA_GENERATOR
#define ACURITE_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class AcuriteAnalyzerSettings;

class AcuriteSimulationDataGenerator
{
public:
	AcuriteSimulationDataGenerator();
	~AcuriteSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, AcuriteAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	AcuriteAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateAcuriteBits();
	U32 mBitCounter;

	SimulationChannelDescriptor mSerialSimulationData;

	U8 mPacket[7];
};
#endif //ACURITE_SIMULATION_DATA_GENERATOR
