// example of portaudio wrapping low-level madronalib DSP code.

#include "../source/DSP/MLDSP.h"
#include "../../portaudio/include/portaudio.h"

using namespace ml;

const int kTestSeconds = 5;
const int kSampleRate = 44100;
const int kFramesPerBuffer = kFloatsPerDSPVector;

// portaudio callback function.
// userData is unused.
static int patestCallback( const void *inputBuffer, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo* timeInfo,
						  PaStreamCallbackFlags statusFlags,
						  void *userData )
{
	// make a tick every kSampleRate samples(1 second)
	static TickSource ticks(kSampleRate);
	
	// make an FDN with 4 delay lines (times in samples)
	static FDN fdn({33, 149, 1377, 1969});
	MLSignal freqs({12000, 13000, 14000, 6000});
	freqs.scale(kTwoPi/kSampleRate);
	fdn.setFilterCutoffs(freqs);
	fdn.setFeedbackGains({0.99, 0.99, 0.99, 0.99});
	
	// process audio
	DSPVectorArray<2> verb = fdn(ticks());

	// store audio
	// in non-interleaved mode, portaudio passes an array of float pointers
	store(verb.getRowVector<0>(), ((float**)outputBuffer)[0]);
	store(verb.getRowVector<1>(), ((float**)outputBuffer)[1]);
	
	return paContinue;
}

int main()
{
	PaStreamParameters outputParameters;
	PaStream *stream;
	PaError err;
	void* pData = nullptr;
	
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
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	err = Pa_OpenStream(
						&stream,
						nullptr, // no input 
						&outputParameters,
						kSampleRate,
						kFramesPerBuffer,
						paClipOff, 
						patestCallback,	
						pData );
	if( err != paNoError ) goto error;

	err = Pa_StartStream( stream );
	if( err != paNoError ) goto error;
	
	printf("Playing for %d seconds:\n", kTestSeconds );
	Pa_Sleep( kTestSeconds * 1000 );
	
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

