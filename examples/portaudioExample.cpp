// example of portaudio wrapping low-level madronalib DSP code.

#include "../source/DSP/MLDSP.h"
#include "../../portaudio/include/portaudio.h"
#include "MLProperty.h"
#include "MLPropertySet.h"

#include "../source/procs/MLProcMultiply.h"

using namespace ml;

const int kTestSeconds = 4;
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
	// MLTEST
	static ProcMultiply pm;
	
	// make a tick every kSampleRate samples(1 second)
	static TickSource ticks(kSampleRate);
	
	// make freqs signal
	MLSignal freqs({12000, 13000, 14000, 6000});
	freqs.scale(kTwoPi/kSampleRate);

	// make an FDN with 4 delay lines (times in samples)
	static FDN fdn
	{ 
		{"delays", {33.f, 149.f, 1390.f, 1400.f} },
		{"cutoffs", freqs } , // TODO more functional rewrite of MLSignal so we can create freqs inline
		{"gains", {0.99, 0.99, 0.99, 0.99} }
	};

	// process audio
	auto verb = fdn(ticks()); // returns a DSPVectorArray<2>

	// store audio
	// in non-interleaved mode, portaudio passes an array of float pointers
	store(verb.getRowVector<0>(), ((float**)outputBuffer)[0]);
	store(verb.getRowVector<1>(), ((float**)outputBuffer)[1]);
	
	DSPVector va, vb, vc;
	va = 2;
	vb = 3;
	
	pm.setInput("foo", va);
	pm.setInput("bar", vb);
	
	pm.setOutput("baz", vc);
	
	pm.process();
	
	std::cout << vc << "\n";

	
	return paContinue;
}

int main()
{
	PaStreamParameters outputParameters;
	PaStream *stream;
	PaError err;
	void* pData = nullptr;
	
	std::cout << "portaudio example:\n";
	
	// MLTEST Property changes
	MLPropertyChange dx{"toad", {1.f, 2.f, 3.f}};
	MLPropertyChange dy{"toad", 23};
	MLPropertyChange dz{"toad", "wet"};
	std::vector<MLPropertyChange> dv = { {"toad", {1.f, 2.f, 3.f} }, {"todd", 23.f} };
	
	// MLTEST constexpr Proc setup
	str_const a("hello");
	str_const b("world");
	str_const c("hello");
	std::cout << a << ", " << b << "!\n";
	std::cout << " a, b: " << ( a == b )  << "\n";
	std::cout << " a, c: " << ( a == c )  << "\n";
	
	
	// test access from Proc ptr
	std::unique_ptr<Proc> pm (new ProcMultiply());
	// ProcMultiply pm;
	
	
	DSPVector va, vb, vc;
	va = 2;
	vb = 3;
	
	pm->setInput("foo", va);
	pm->setInput("bar", vb);
	pm->setOutput("baz", vc);
	
	pm->process();
	
	std::cout << vc << "\n";
	
//	pm->test();
	
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

