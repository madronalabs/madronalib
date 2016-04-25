// example of portaudio wrapping low-level madronalib DSP code.

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>

#include "../include/madronalib.h"
#include "../source/DSP/MLDSP.h"
#include "../tests/tests.h"
#include "../../portaudio/include/portaudio.h"

using namespace ml;

#define NUM_SECONDS   (5)
#define SAMPLE_RATE   (44100)

const int kFramesPerBuffer = kFloatsPerDSPVector;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int patestCallback( const void *inputBuffer, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo* timeInfo,
						  PaStreamCallbackFlags statusFlags,
						  void *userData )
{
	static DSPVector silence(0);
	static TickSource ticksL(SAMPLE_RATE/3);
	static TickSource ticksR(SAMPLE_RATE/4);
	
	float* outL = ((float**)outputBuffer)[0];
	float* outR = ((float**)outputBuffer)[1];

	ticksL().store(outL);
	ticksR().store(outR);
	
	// Prevent unused variable warnings. 
	(void) timeInfo; 
	(void) statusFlags;
	(void) inputBuffer;
	
	return paContinue;
}

int main()
{
	PaStreamParameters outputParameters;
	PaStream *stream;
	PaError err;
	
	std::cout << "portaudio example:\n";

	err = Pa_Initialize();
	if( err != paNoError ) goto error;
	
	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	if (outputParameters.device == paNoDevice) 
	{
		fprintf(stderr,"Error: No default output device.\n");
		goto error;
	}
	
	outputParameters.channelCount = 2;  
	outputParameters.sampleFormat = paFloat32 | paNonInterleaved; 
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream(
						&stream,
						NULL, /* no input */
						&outputParameters,
						SAMPLE_RATE,
						kFramesPerBuffer,
						paClipOff,      /* we won't output out of range samples so don't bother clipping them */
						patestCallback,	
						nullptr); // &data );
	if( err != paNoError ) goto error;

	err = Pa_StartStream( stream );
	if( err != paNoError ) goto error;
	
	printf("Playing for %d seconds:\n", NUM_SECONDS );
	Pa_Sleep( NUM_SECONDS * 1000 );
	
	err = Pa_StopStream( stream );
	if( err != paNoError ) goto error;
	
	err = Pa_CloseStream( stream );
	if( err != paNoError ) goto error;
	
	Pa_Terminate();
	printf("Test finished.\n");
	
	return err;
error:
	Pa_Terminate();
	fprintf( stderr, "An error occured while using the portaudio stream\n" );
	fprintf( stderr, "Error number: %d\n", err );
	fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	return err;

}

