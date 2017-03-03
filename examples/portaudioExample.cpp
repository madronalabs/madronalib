// example of portaudio wrapping low-level madronalib DSP code.

#include "../source/DSP/MLDSP.h"
#include "../../portaudio/include/portaudio.h"
#include "MLProperty.h"
#include "MLPropertySet.h"
#include "MLTextUtils.h"

#include "../source/procs/MLProcFactory.h"

using namespace ml;

const int kTestSeconds = 2;
const int kSampleRate = 44100;
const int kFramesPerBuffer = kFloatsPerDSPVector;

// TEST
#include "../source/procs/MLProcMultiply.h"


uint64_t rdtsc()
{
	unsigned int lo,hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}

int kTotalSamples = 0;

std::unique_ptr<Proc> pm (ProcFactory::theFactory().create("multiply")); 
textUtils::NameMaker namer;

OnePole noiseFreqFilter(onePoleCoeffs::onePole(5000.f*kTwoPi/kSampleRate));

// make a tick every kSampleRate samples(1 second)
TickSource ticks(kSampleRate);

// portaudio callback function.
// userData is unused.
static int patestCallback( const void *inputBuffer, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo* timeInfo,
						  PaStreamCallbackFlags statusFlags,
						  void *userData )
{	
	// make freqs signal
	MLSignal freqs({12000, 13000, 14000, 6000});
	freqs.scale(kTwoPi/kSampleRate);

	// NOTE
	// C++11 adds a runtime check to access static data like this! It is very convenient
	// but do something else to initialize performance-critical code.
	// make an FDN with 4 delay lines (times in samples)
	static FDN fdn
	{ 
		{"delays", {33.f, 149.f, 1390.f, 1400.f} },
		{"cutoffs", freqs } , // TODO more functional rewrite of MLSignal so we can create freqs statically
		{"gains", {0.99, 0.99, 0.99, 0.99} }
	};

	// process audio
	auto verb = fdn(noiseFreqFilter(ticks())); // returns a DSPVectorArray<2>

	// store audio
	// in non-interleaved mode, portaudio passes an array of float pointers
	store(verb.getRowVector<0>(), ((float**)outputBuffer)[0]);
	store(verb.getRowVector<1>(), ((float**)outputBuffer)[1]);
	
	/*
	DSPVector va, vb, vc;
	va = 2;
	vb = 3;
	pm->setInput("foo", va);
	pm->setInput("bar", vb);
	pm->setOutput("baz", vc);	
	pm->setTextParam("mode", TextFragment(namer.nextName()));
	pm->process();
	*/
	
	kTotalSamples += kFloatsPerDSPVector;
	
	return paContinue;
}

int main()
{
	PaStreamParameters outputParameters;
	PaStream *stream;
	PaError err;
	void* pData = nullptr;
	
	std::cout << "portaudio example:\n";
	
	
	// size = 128. why was size of old procs > 6kb?
	std::cout << "size of multiply:" << sizeof(ProcMultiply) << "\n";
	
	
	/*
	// MLTEST Property changes
	MLPropertyChange dx{"toad", {1.f, 2.f, 3.f}};
	MLPropertyChange dy{"toad", 23};
	MLPropertyChange dz{"toad", "wet"};
	std::vector<MLPropertyChange> dv = { {"toad", {1.f, 2.f, 3.f} }, {"todd", 23.f} };
	*/
	
	uint64_t startCycles = rdtsc();
	
	
	// MLTEST constexpr Proc setup
	constStr a("hello");
	constStr b("world");
	constStr c("hello");
	std::cout << a << ", " << b << "!\n";
	std::cout << " a, b: " << ( a == b )  << "\n";
	std::cout << " a, c: " << ( a == c )  << "\n";
	
	std::unique_ptr<Proc> pm (ProcFactory::theFactory().create("multiply")); 
	
	DSPVector va, vb, vc;
	va = 2;
	vb = 3;
	
	pm->setInput("foo", va);
	pm->setInput("bar", vb);
	pm->setOutput("baz", vc);
	
	/*
	 TODO make non-const versions
	 and integrate the parameter list stuff in FDN
	 
	int k = fabs(ml::rand()*15.);
	std::cout << "rand number: " << k  << "\n";
	Symbol randSym = ("rand") + ml::textUtils::naturalNumberToText(k);
	std::cout << "random symbol: " << randSym << "\n";
	pm->setParam(randSym, 23.);
	*/
	
/*	DSPVector x = unityInterpVector()*DSPVector(20.f);
	std::cout << "exp2  : " << exp2(x) << "\n";
	std::cout << "exp2 a: " << exp2Approx(x) << "\n";
*/	
	
	pm->setParam("a", 4.5);
	pm->setTextParam("mode", "cosmic");
	pm->process();
		
	
	const constStrArray& paramNames = pm->getParamNames();
	int v = paramNames.size();
	std::cout << v << " params: ";
	for(int i=0; i<v; ++i)
	{
		std::cout << paramNames[i] << " ";
	}
	std::cout << "\n";
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
	
	// MLTEST who decides buffer size?
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
	
	{
		uint64_t endCycles = rdtsc();
		uint64_t diff = endCycles - startCycles;
		std::cout << "cycles used: " << diff << "\n";
		std::cout << "samples processed: " << kTotalSamples << "\n";
		double samplesPerCycle = (double)kTotalSamples / (double)diff;
		std::cout << samplesPerCycle << " samples / cycle.\n";
	}

	
	return err;
	
error:
	Pa_Terminate();
	fprintf( stderr, "An error occured while using the portaudio stream\n" );
	fprintf( stderr, "Error number: %d\n", err );
	fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	return err;
}

