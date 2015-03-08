// Decoders.h, copied from the Jeelib RF12 example 'ookRelay2'
// Generalized decoder framework for 868 MHz and 433 MHz OOK signals.
// 2010-04-11 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php
// Modified 2015-03-04 <jorj@jorj.org>, adding AcuRite temp sensor demod
//   ... made a little dumber for Saleae Logic embedded use

#include <stdio.h>
#include <stdlib.h>
#include "crc16.h"
#include "glue.h"

class DecodeOOK {
 public:
  //protected:
    byte bits, flip, state, pos, data[25];
    // the following fields are used to deal with duplicate packets
    word lastCrc, lastTime;
    byte repeats, minGap, minCount;

    // gets called once per incoming pulse with the width in us
    // return values: 0 = keep going, 1 = done, -1 = no match
    virtual char decode (word width) =0;
    
    // add one bit to the packet data buffer
    void gotBit (char value) {
        byte *ptr = data + pos;
        *ptr = (*ptr >> 1) | (value << 7);

        if (++bits >= 8) {
            bits = 0;
            if (++pos >= sizeof data) {
                resetDecoder();
                return;
            }
        }
        
        state = OK;
    }
    
    // store a bit using Manchester encoding
    void manchester (char value) {
        flip ^= value; // manchester code, long pulse flips the bit
        gotBit(flip);
    }
    
    // move bits to the front so that all the bits are aligned to the end
    void alignTail (byte max =0) {
        // align bits
        if (bits != 0) {
            data[pos] >>= 8 - bits;
            for (byte i = 0; i < pos; ++i)
                data[i] = (data[i] >> bits) | (data[i+1] << (8 - bits));
            bits = 0;
        }
        // optionally shift bytes down if there are too many of 'em
        if (max > 0 && pos > max) {
            byte n = pos - max;
            pos = max;
            for (byte i = 0; i < pos; ++i)
                data[i] = data[i+n];
        }
    }
    
    void reverseBits () {
        for (byte i = 0; i < pos; ++i) {
            byte b = data[i];
            for (byte j = 0; j < 8; ++j) {
                data[i] = (data[i] << 1) | (b & 1);
                b >>= 1;
            }
        }
    }
    
    void reverseNibbles () {
        for (byte i = 0; i < pos; ++i)
            data[i] = (data[i] << 4) | (data[i] >> 4);
    }
    
public:
    enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };

    DecodeOOK (byte gap =5, byte count =0) 
        : lastCrc (0), lastTime (0), repeats (0), minGap (gap), minCount (count)
        { resetDecoder(); }
        
    bool nextPulse (word width) {
        if (state != DONE)
            switch (decode(width)) {
                case -1: // decoding failed
                    resetDecoder();
                    break;
                case 1: // decoding finished
                    while (bits)
                        gotBit(0); // padding
		    state = DONE;
                    break;
            }
        return state == DONE;
    }
    
    const byte* getData (byte& count) const {
        count = pos;
        return data; 
    }
    
    void resetDecoder () {
        bits = pos = flip = 0;
        state = UNKNOWN;
    }
};

// 433 MHz decoders

#define SYNC_WIDTH 500 // min duration of the positive half of a sync bit (uS)
#define MAX_SYNC_WIDTH 670
#define ONE_WIDTH  380
#define ZERO_WIDTH 190
#define BIT_WIDTH 1200 // total duration of a one-bit pulse
#define NUM_SYNCS 4

class AcuRiteDecoder : public DecodeOOK {
 public:
  AcuRiteDecoder () {}

  virtual char decode (word width) {
    char ret = 0;

    if (!width)
      return -1;

    switch (state) {
    case UNKNOWN:
      // 0-to-positive initial transition
      state = OK;
      break;
    case OK:
      // OK invoked at pos-to-0 transition. Look at width, figure out what 
      // kind of pulse this is (SYNC/0/1)

      if (width > BIT_WIDTH) {
	// Dunno what data we have; it's bad.
	state = UNKNOWN;
	ret = -1;
      } else if (width >= SYNC_WIDTH) {
	// sync bit
	state = T0; // T0: expecting zero-pulse for SYNC of 584-600mS
      } else if (width >= ONE_WIDTH) {
	if (flip != NUM_SYNCS) {
	  state = T3;
	  return 0;
	}
	state = T1; // T1: expecting zero-pulse for 1-bit of 180-200mS
      } else if (width >= ZERO_WIDTH) {
	if (flip != NUM_SYNCS) {
	  state = T3;
	  return 0;
	}
	state = T2; // T2: expecting zero-pulse for 0-bit of 584-800mS
      }
      break;

      // States T0 - T2: receiving the zero-pulse half. Set to state = OK
      // if we receive it as expected, and reset if not?
    case T0: // Sync bits.
      if (width > MAX_SYNC_WIDTH) {
	// reject: the trailing edge is too long, so it's not a sync.
	state = UNKNOWN;
	return -1;
      }
      if (width < SYNC_WIDTH) {
	// We might have missed a transition, and this is the start of a 1/0 -
	// change state, increment sync counter, and re-process it
	flip++;
	state = OK;
	return decode(width);
      }

      flip++; // use flip to count sync bits
      state = OK;
      break;
    case T1:
	gotBit(1);
	if (pos >= 7) {
	  // Data ready to receive - packets are 7 bytes long
	  reverseBits();
	  return 1;
	}

	break;
    case T2:
      gotBit(0);
      
      if (pos >= 7) {
	// Data ready to receive - packets are 7 bytes long
	reverseBits();
	return 1;
      }
      break;
    case T3:
      // error condition; consume the 0-bit and then reset
      return -1;
      break;
    }

    return ret;
  }

};


// Dumb Arduino IDE pre-processing bug - can't put this in the main source file!
typedef struct {
    char typecode;
    const char* name;
    DecodeOOK* decoder;
} DecoderInfo;
