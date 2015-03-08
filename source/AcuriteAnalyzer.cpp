#include "AcuriteAnalyzer.h"
#include "AcuriteAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include "decoders.h"

AcuriteAnalyzer::AcuriteAnalyzer()
:	Analyzer(),  
	mLastPulse ( 0 ),
	mSettings( new AcuriteAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

AcuriteAnalyzer::~AcuriteAnalyzer()
{
	KillThread();
}

void log(const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  char buf[128];
  vsprintf(buf, fmt, args);
  va_end(args);

  FILE *f = fopen("/tmp/debug.log", "a");
  fprintf(f, "%s\n", buf);
  fclose(f);
}

byte calcParity(byte b)
{
  byte result = 0;
  for (char i=0; i<=6; i++) {
    result ^= (b & (1<<i)) ? 1 : 0;
  }
  return result ? 0x80 : 0x00;
}

void interpretData(const byte *data, int size, char *output)
{
  if (size != 7) {
    sprintf(output, "Invalid data (not 7 bytes)");
    return;
  }

  // Check parity bits. (Byte 0 and byte 7 have no parity bits.)              
  for (int i=1; i<=5; i++) {
    if (calcParity(data[i]) != (data[i] & 0x80)) {
      sprintf(output, "Parity failure in byte %d [0x%.2X !~ 0x%.2X]", i, data[i], calcParity(data[i]));
      return;
    }
  }

  // Check checksum.                                                          
  unsigned char cksum = 0;
  for (int i=0; i<=5; i++) {
    cksum += data[i];
  }
  if ((cksum & 0xFF) != data[6]) {
    sprintf(output, "Checksum failure (0x%X vs. 0x%X)", cksum, data[6]);
    return;
  }

  // Data is good! Construct string of data...

  // Source A/B/C and ID                                                      
  output += sprintf(output, "Channel ",  (data[0] & 0xC0) == 0xC0 ? 'A' :
		    (data[0] & 0xC0) == 0x80 ? 'B' :
		    (data[0] & 0xC0) == 0x00 ? 'C' :
		    'x');

  output += sprintf(output, "0x%X ", (data[0] & 0x3F) << 7 | (data[1] & 0x7F));

  output += sprintf(output, "%d%% ", data[3] & 0x7f);

  // Temperature                                                              
  unsigned long t1 = ((data[4] & 0x0F) << 7) | (data[5] & 0x7F);
  float temp = ((float)t1 - (float) 1024) / 10.0;
  output += sprintf(output, "%f C (%f F)", temp, temp * 9.0 / 5.0 + 32.0);
}


void AcuriteAnalyzer::WorkerThread()
{
  AcuRiteDecoder decoder;
  U32 lastPulse = 0;
  Frame frame; // for saving/printing data
  frame.mFlags = 0;
  frame.mStartingSampleInclusive = 0;

  mResults.reset( new AcuriteAnalyzerResults( this, mSettings.get() ) );
  SetAnalyzerResults( mResults.get() );
  mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
  
  mSampleRateHz = GetSampleRate();
  log("mSampleRateHz is %lu", mSampleRateHz);
  mSampleRateMs = mSampleRateHz / 1000000;
  log("mSampleRateMs is %lu", mSampleRateMs);
  
  mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

  log("Looking for start low...");

  // Have to start on a low pulse...
  if( mSerial->GetBitState() == BIT_HIGH )
    mSerial->AdvanceToNextEdge();
  
  while (1) {
    // Find the leading edge
    log("looking for next leading edge");
    mSerial->AdvanceToNextEdge();

    // Determine the time of this pulse
    U64 samplepos = mSerial->GetSampleNumber() / mSampleRateMs;

    if ( (!frame.mStartingSampleInclusive) || 
	 decoder.state == DecodeOOK::UNKNOWN ) {
      frame.mStartingSampleInclusive = mSerial->GetSampleNumber();
      mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
    }

    // Pass the pulse to the decoder
    log("calling decoder.nextPulse(%lu)", (unsigned long)samplepos - lastPulse);
    if (decoder.nextPulse((unsigned long)samplepos - lastPulse)) {
      byte size;

      log("nextPulse is now done");

      const byte *data = decoder.getData(size);
      char output[1024];
      interpretData(data, size, output);
      decoder.resetDecoder();

      // Display the data
      mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
      frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
      frame.mData1 = (U64)output; // fixme - this is fugly.
      mResults->AddFrame( frame );
      mResults->CommitResults();

      ReportProgress( frame.mEndingSampleInclusive );

      // reset frame for next one
      frame.mStartingSampleInclusive = 0;
      
      // make sure we're at a low level when we're done decoding
      if ( mSerial->GetBitState() == BIT_HIGH)
	mSerial->AdvanceToNextEdge();
    }

    log("Decoder state is now %d", decoder.state);
    lastPulse = samplepos;
  }


}

bool AcuriteAnalyzer::NeedsRerun()
{
	return false;
}

U32 AcuriteAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 AcuriteAnalyzer::GetMinimumSampleRateHz()
{
  // fixme
	return 9600 * 4;
}

const char* AcuriteAnalyzer::GetAnalyzerName() const
{
	return "Acurite";
}

const char* GetAnalyzerName()
{
	return "Acurite";
}

Analyzer* CreateAnalyzer()
{
	return new AcuriteAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
