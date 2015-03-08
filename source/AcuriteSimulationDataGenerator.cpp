#include <stdlib.h>
#include "AcuriteSimulationDataGenerator.h"
#include "AcuriteAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

// Simulation data: 4 sync pulses @ index 0 [662ms on, 564ms off]
// 56 bits of data: logic 1 [436, 180] or logic 0 [245, 366]
// one 100ms on-pulse at the end
#define SYNCHIGH 662
#define SYNCLOW 564
#define ONEHIGH 436
#define ONELOW 180
#define ZEROHIGH 245
#define ZEROLOW 366
#define STOPHIGH 100

AcuriteSimulationDataGenerator::AcuriteSimulationDataGenerator()
:	mBitCounter( 0 )
{
}

AcuriteSimulationDataGenerator::~AcuriteSimulationDataGenerator()
{
}

unsigned long RandomLessThan(unsigned long max)
{
  unsigned long ret = (unsigned long) (((float)rand() / (float)RAND_MAX) * max);
  return ret;
}

void log2(const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  char buf[128];
  vsprintf(buf, fmt, args);
  va_end(args);
  
  FILE *f = fopen("/tmp/debug2.log", "a");
  fprintf(f, "%s\n", buf);
  fclose(f);
}

void SetParity(U8 *b)
{
  U8 result = 0;
  for (char i=0; i<=6; i++) {
    result ^= ((*b) & (1<<i)) ? 1 : 0;
  }

  *b |= result ? 0x80 : 0x00;
}


void AcuriteSimulationDataGenerator::Initialize( U32 simulation_sample_rate, AcuriteAnalyzerSettings* settings )
{
  mSimulationSampleRateHz = simulation_sample_rate;
  mSettings = settings;

  srand(time(NULL));
  
  mSerialSimulationData.SetChannel( mSettings->mInputChannel );
  mSerialSimulationData.SetSampleRate( simulation_sample_rate );
  mSerialSimulationData.SetInitialBitState( BIT_LOW );

  // Generate the sample packet we're going to emulate

  // Source: A, B, C.
  if (rand() < RAND_MAX / 3) {
    mPacket[0] = 0xC0;
  } else if (rand() < RAND_MAX / 2) {
    mPacket[0] = 0x80;
  } else {
    mPacket[0] = 0x00;
  }

  // Source ID, broken up between two bytes...
  mPacket[0] |= RandomLessThan(63);
  mPacket[1] = RandomLessThan(127);

  // Magic number signature byte
  mPacket[2] = 0x44;

  // Humidity
  mPacket[3] = RandomLessThan(100);

  // Temperature, from -40 to +140 C, in mPacket[4] and mPacket[5]
  mPacket[4] = RandomLessThan(15);
  mPacket[5] = RandomLessThan(127);

  // Calculate parity bits as appropriate
  for (int i=1; i<=5; i++) {
    SetParity(&mPacket[i]);
  }

  // Checksum
  unsigned char cksum = 0;
  for (int i=0; i<=5; i++) {
    cksum += mPacket[i];
  }
  mPacket[6] = cksum;

  for (int i=0; i<7; i++) {
    log2("mPacket[%d] = 0x%.2X", i, mPacket[i]);
  }
}


U32 AcuriteSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
  U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );
  
  while( mSerialSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
    {
      CreateAcuriteBits();
    }
  
  *simulation_channel = &mSerialSimulationData;
  return 1;
}

void AcuriteSimulationDataGenerator::CreateAcuriteBits()
{
  U32 samples_per_ms = mSimulationSampleRateHz / 1000000;

  if (mBitCounter == 0) {
    // Lead-in time...
    mSerialSimulationData.Advance( samples_per_ms * 100000);
  }
  
  if (mBitCounter < 4) {
    // sync pulse, please
    mSerialSimulationData.Transition(); // go high
    mSerialSimulationData.Advance( samples_per_ms * SYNCHIGH );
    mSerialSimulationData.Transition(); // back low
    mSerialSimulationData.Advance( samples_per_ms * SYNCLOW );
  } else if (mBitCounter < 56 + 4) {

    // mid-bitstream; figure out our byte index and then the bit in it
    int byteCount = (mBitCounter - 4) / 8;
    int bitCount = (mBitCounter - 4) - 8 * byteCount;
    int bitOn = mPacket[byteCount] & (1 << ( 7 - bitCount ) );

    if (bitOn) {
      // 1-bit
      mSerialSimulationData.Transition(); // go high
      mSerialSimulationData.Advance( samples_per_ms * ONEHIGH );
      mSerialSimulationData.Transition(); // back low
      mSerialSimulationData.Advance( samples_per_ms * ONELOW );
    } else {
      // 0-bit
      mSerialSimulationData.Transition(); // go high
      mSerialSimulationData.Advance( samples_per_ms * ZEROHIGH );
      mSerialSimulationData.Transition(); // back low
      mSerialSimulationData.Advance( samples_per_ms * ZEROLOW );
    }
  } else {
    // final half-bit
    mSerialSimulationData.Transition(); // go high
    mSerialSimulationData.Advance( samples_per_ms * STOPHIGH );
    mSerialSimulationData.Transition(); // back low
    mSerialSimulationData.Advance( samples_per_ms * 30000000 );
    mBitCounter = -1;
  }

  mBitCounter++;
}
