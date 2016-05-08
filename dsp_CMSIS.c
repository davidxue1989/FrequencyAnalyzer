#include "dsp_CMSIS.h"
#include "arm_const_structs.h"
#include "rom_map.h"
#include "utils.h"

#include <stdlib.h>

#include "uart_if.h"
#include "common.h"

#include "FrequencyMusicNotesLookup.h"

volatile tboolean gb_DSPprocessing = FALSE;
static tDSPInstance* gDSP_DSPInstance = NULL;

tboolean DSPReadyForSignal() {
  return gb_DSPprocessing==FALSE;
}

tDSPInstance* DSPCeateInstance(uint32_t signalSize, uint32_t Fs) {
  tDSPInstance* tempInstance;
  tempInstance = (tDSPInstance*)malloc(sizeof(tDSPInstance));
  if (tempInstance == NULL){
    UART_PRINT("DSPCreate malloc error!");
    LOOP_FOREVER(); //malloc error. heap too small?
    return NULL;
  }
  tempInstance->signalSize = signalSize;
  tempInstance->Fs = Fs;
  tempInstance->fftSize = signalSize / 4;
  tempInstance->ucpSignal = (unsigned char*)malloc(sizeof(unsigned char)*signalSize);
  tempInstance->fpSignal = (float32_t*)malloc(sizeof(float32_t)*signalSize/2);
  tempInstance->FFTResults = (float32_t*)malloc(sizeof(float32_t)*signalSize/4);
  if (tempInstance->ucpSignal==NULL || \
      tempInstance->fpSignal==NULL || \
      tempInstance->FFTResults==NULL) {
    UART_PRINT("DSPCreate malloc error!");
    LOOP_FOREVER(); //malloc error. heap too small?
    return NULL;
  }
  tempInstance->maxEnergyBinIndex = 0;
  tempInstance->maxEnergyBinValue = 0;

  return tempInstance;
}

void DSPDestroyInstance(tDSPInstance* instance) {
  if (instance->ucpSignal) {
    free(instance->ucpSignal);
  }
  if (instance->fpSignal) {
    free(instance->fpSignal);
  }
  free(instance);
}

void DSPProcessSignal(tDSPInstance* instance) {
  if (instance==NULL || instance->ucpSignal==NULL || instance->signalSize<=0) {
    return; //instance was not properly created
  }
  if (gb_DSPprocessing) {
    return; //not ready to process more
  }
  gDSP_DSPInstance = instance;
  gb_DSPprocessing = TRUE;
}

float32_t DSPGetFundamentalFrequency(tDSPInstance* instance) {
  return ((float32_t)instance->maxEnergyBinIndex) * instance->Fs / instance->fftSize;
}

uint32_t binarySearchClosest_iterative(const float* arr, uint32_t arrSz, float target) {
  uint32_t l, r, m;
  l = 0;
  r = arrSz-1;
  if (arr[l]>=target)
  {
    return l;
  }
  if (arr[r]<=target) {
    return r;
  }
  m = (r+l)/2;
  while (m>l) {
	if (arr[m] == target) {
	  return m;
    }
    else if (arr[m] > target) {
      r = m;
    }
    else if (arr[m] < target) {
      l = m;
    }
    m = (r+l)/2; //will equal to l when r=l+1;
  }
  uint32_t closest = (target-arr[l])<=(arr[r]-target)?l:r;
  return closest;
}

const char* DSPGetClosestMusicNote(tDSPInstance* instance) {
  uint32_t closest = binarySearchClosest_iterative(frequenciesLookup, sizeof(frequenciesLookup)/sizeof(frequenciesLookup[0]), DSPGetFundamentalFrequency(instance));
  return notesLookup[closest];
}

//the dsp main thread
void DSP( void* pvParameters ) {
  while (1) {
    if (gb_DSPprocessing) {
      DSPConvertUC2F32(gDSP_DSPInstance);
      DSPCalculateFFT(gDSP_DSPInstance);
      gb_DSPprocessing  = FALSE;
    }
    MAP_UtilsDelay(1000);
  }
}


void DSPConvertUC2F32(tDSPInstance* instance) {
  unsigned char* ucpSignal = instance->ucpSignal;
  float32_t* fpSignal = instance->fpSignal;
  unsigned int i;
  for (i=0; i<instance->signalSize; i+=2) {
    //the signal should be 16 kHz 16bit PCM (http://www.ti.com/tool/TIDC-CC3200AUDBOOST)
    //so need to combine two unsigned char together
    fpSignal[i/2] = (float32_t) (ucpSignal[i+1] | (ucpSignal[i]<<8));
  }
}

void DSPCalculateFFT(tDSPInstance* instance) {
	if (instance->signalSize != 1024*2) {
	  while(1) {
	  }//signal size different than our assumption
	}

	uint32_t ifftFlag = 0;
	uint32_t doBitReverse = 1;

//	int i;

//	UART_PRINT("\n\r\n\r");
//	for (i=0; i<instance->signalSize; i++) {
//		UART_PRINT("%d ", instance->ucpSignal[i]);
//	}
//	UART_PRINT("\n\r\n\r");

//	UART_PRINT("\n\r\n\r");
//	for (i=0; i<instance->signalSize/2; i++) {
//		UART_PRINT("%f ", instance->fpSignal[i]);
//	}
//	UART_PRINT("\n\r\n\r");

    /* Process the data through the CFFT/CIFFT module */
    arm_cfft_f32(&arm_cfft_sR_f32_len512, instance->fpSignal, ifftFlag, doBitReverse);
//    UART_PRINT("\n\r\n\r");
//    for (i=0; i<instance->signalSize/2; i++) {
//    	UART_PRINT("%f ", instance->fpSignal[i]);
//    }
//    UART_PRINT("\n\r\n\r");

    /* Process the data through the Complex Magnitude Module for
    calculating the magnitude at each bin */
    arm_cmplx_mag_f32(instance->fpSignal, instance->FFTResults, instance->fftSize);
    //ignore dc bias
    instance->FFTResults[0] = 0;
//    UART_PRINT("\n\r\n\r");
//    for (i=0; i<instance->signalSize/4; i++) {
//    	UART_PRINT("%f ", instance->FFTResults[i]);
//    }
//    UART_PRINT("\n\r\n\r");

    /* Calculates maxValue and returns corresponding BIN value */
    arm_max_f32(instance->FFTResults, instance->fftSize/2, \
        &instance->maxEnergyBinValue, &instance->maxEnergyBinIndex);
//    UART_PRINT("max energy at (%d:%f)\n\r", instance->maxEnergyBinIndex, instance->maxEnergyBinValue);

//    UART_PRINT("\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
}
