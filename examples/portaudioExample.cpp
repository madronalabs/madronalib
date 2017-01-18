// example of portaudio wrapping low-level madronalib DSP code.

#include "../source/DSP/MLDSP.h"
#include "../../portaudio/include/portaudio.h"
#include "MLProperty.h"
#include "MLPropertySet.h"

using namespace ml;

const int kTestSeconds = 2;
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
	
	return paContinue;
}

// constexpr string courtesy of Scott Schurr
class str_const 
{ 
//private:
public:	
	const char* const p_;
	const std::size_t  size_;
	
public:
	template<std::size_t N>
	constexpr str_const(const char(&a)[N]) : 
	p_(a), size_(N-1) {}
	
	constexpr char operator[](std::size_t n) const 
	{
		return n < size_ ? p_[n] :
		throw std::out_of_range("");
	}

	constexpr std::size_t size() const 
	{ 
		return size_; 
	}
	
	friend constexpr bool operator==(const str_const& a, const str_const& b );
	
};

constexpr bool charArraysEqual(char const * a, char const * b)
{
	return (*a && *b) ? 
		(*a == *b && charArraysEqual(a + 1, b + 1)) :
		(!*a && !*b);
}
						 
constexpr bool operator==(const str_const& a, const str_const& b ) 
{
	return charArraysEqual(a.p_, b.p_);
}

inline std::ostream& operator<< (std::ostream& out, const str_const & r)
{
	out << r.p_;
	return out;
}	

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N] ) noexcept
{
	return N;
}

// typedef str_const str_const_array[];

/*
constexpr int getIndex (const str_const* begin, const str_const* end, str_const const& value)
{
	return (! (begin != end && !(*begin == value))) ?
	(end - begin) :
	getIndex (begin+1, end, value);
}
*/

constexpr int getIndex (const str_const* begin, int i, int N, str_const const& value)
{
	return (! ((i != N) && !(*begin == value))) ?
		(i) :
		getIndex (begin + 1, i + 1, N, value);		
}

// returns the length N if not found, otherwise the index of the array element equal to str.
template <std::size_t N>
constexpr int find(str_const const(&array)[N], str_const str)
{ 
	return getIndex(&(array[0]), 0, N, str);
}

// MLTEST proc class
// compiler needs to be able to query the functor / proc about its i/o size possibilities
// to turn bytecode into a list of process() calls

class Proc
{
public:
	virtual void process() = 0;
	
	void setInput();	
};

/*
 
 class MLProcAdd : public MLProc
 {
 public:
	void process(const int frames) override;		
	MLProcInfoBase& procInfo() override { return mInfo; }
 
 private:
	MLProcInfo<MLProcAdd> mInfo;
 };
 
 // ----------------------------------------------------------------
 // registry section
 
 namespace
 {
	MLProcRegistryEntry<MLProcAdd> classReg("add");
	ML_UNUSED MLProcInput<MLProcAdd> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcAdd> outputs[] = {"out"};
 }	

*/

class ProcMultiply : public Proc
{
	
public:
	
//	MLProcInfoBase& procInfo() override { return mInfo; }
	
	static const int i = 3;
	
	static constexpr str_const paramNames[]{ "a", "b", "c" };
	static constexpr str_const inputNames[]{ "foo", "bar" };
	static constexpr str_const outputNames[]{ "baz" };
	
	// + 1 leaves room for setting when names are not found
	float params[countof(paramNames) + 1];
	DSPVector* inputs[countof(inputNames) + 1];
	DSPVector* outputs[countof(outputNames) + 1];
	
	inline float& param(str_const str)
	{
		return params[find(paramNames, str)];
	}
	
	inline DSPVector& input(str_const str)
	{
		return *inputs[find(inputNames, str)];
	}
	
	inline void setInput(str_const str, DSPVector &v)
	{
		inputs[find(inputNames, str)] = &v;
	}
	
	inline DSPVector& output(str_const str)
	{
		return *outputs[find(outputNames, str)];
	}
	
	inline void setOutput(str_const str, DSPVector &v)
	{
		outputs[find(outputNames, str)] = &v;
	}
	
	void test ()
	{
		std::cout << "counts: " << countof(ProcMultiply::paramNames) << " " << countof(ProcMultiply::inputNames) << "\n";
		std::cout << "finds: " << find(ProcMultiply::inputNames, "bar") << "\n";
		std::cout << paramNames[1] << paramNames[2] << "\n";
		
		std::cout << "params: " << param("a") << " " << param("b") << " " << param("c") << " " << param("d") << " " << "\n";
		param("a") = 1.29f;
		param("b") = 2.29f;
		param("c") = 3.29f;
		param("d") = 4.29f;
		std::cout << "params: " << param("a") << " " << param("b") << " " << param("c") << " " << param("d") << " " << "\n";
	}

	void process() override
	{
		output("baz") = multiply(input("foo"), input("bar"));
	}

private:
	
//	MLProcInfo<MLProcMultiplyAdd> mInfo;
	
};

// definition of static constexpr data member at namespace scope is still required, until C++17
constexpr str_const ProcMultiply::paramNames[];
constexpr str_const ProcMultiply::inputNames[];
constexpr str_const ProcMultiply::outputNames[];

template<typename T> constexpr int inputIndex()
{
	return 1;
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
	
//	std::unique_ptr<Proc> pm (new ProcMultiply());
	std::unique_ptr<ProcMultiply> pm (new ProcMultiply());
	
	std::cout << a << ", " << b << "!\n";
	std::cout << " a, b: " << ( a == b )  << "\n";
	std::cout << " a, c: " << ( b == c )  << "\n";
	
	
	DSPVector va, vb, vc;
	va = 2;
	vb = 3;
	
	pm->setInput("foo", va);
	pm->setInput("bar", vb);
	pm->setOutput("baz", vc);
	
	pm->process();
	
	std::cout << vc << "\n";
	
	pm->test();
	
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

