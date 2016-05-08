#include "arm_math.h"

#include "uart_if.h"
#include "common.h"

typedef struct
{
  unsigned char* ucpSignal; //8bit (representing 16bit signal)
  uint32_t signalSize; //length of ucpSignal
    uint32_t Fs; //sampling frequency in Hz of signal
  float32_t* fpSignal; //combined back to 16bit, then converted to float.  half the length of ucpSignal
  float32_t* FFTResults; //half the length of fpSignal
    uint32_t fftSize;
  uint32_t maxEnergyBinIndex; /* Index at which max energy of bin ocuurs */
    float32_t maxEnergyBinValue;
}
tDSPInstance;

typedef unsigned int tboolean;

#ifndef TRUE
#define TRUE                    1
#endif

#ifndef FALSE
#define FALSE                   0
#endif


extern tboolean DSPReadyForSignal();
extern tDSPInstance* DSPCeateInstance(uint32_t signalSize, uint32_t Fs);
extern void DSPDestroyInstance(tDSPInstance* instance);
extern void DSPProcessSignal(tDSPInstance* instance);
extern float32_t DSPGetFundamentalFrequency(tDSPInstance* instance);
extern const char* DSPGetClosestMusicNote(tDSPInstance* instance);

void DSP( void *pvParameters );  //the dsp main thread

void DSPConvertUC2F32(tDSPInstance* instance);
void DSPCalculateFFT(tDSPInstance* instance);



