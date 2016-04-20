//  madronalib
//  tests.h

#include <chrono>

template <class T> class timedResult
{
public:
	timedResult(double time, T _r) : elapsedTime(time), result(_r){}
	double elapsedTime;
	T result;
};


// return 64-bit time stamp counter
inline uint64_t rdtsc()
{
	unsigned int lo,hi;
	unsigned int ax, bx, cx, dx;
	unsigned int func = 0;
	__asm__ __volatile__ ("cpuid":\
						  "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}


template <class T> inline timedResult<T> timeIterations(std::function<T(void)> func, int iters)
{	
	T result;
	uint64_t cycles1, cycles2, cycles3;
	double mean;
	uint64_t totalCyclesUsed = 0;
	const int warmups = 4;
	int c = iters;
	int totalTimed = 0;
	
	while(1)
	{
		for(int i=0; i<warmups; ++i)
		{
			result = func();
			if(!(--c)) goto end;
		}
		cycles1 = rdtsc();

		// do the thing, returning a value so the function call is not optimized away
		result = func();
		totalTimed++;
		if(!(--c)) goto end;
		
		// get overhead
		cycles2 = rdtsc();
		cycles3 = rdtsc();
		
		uint64_t funcTime = (cycles2 - cycles1) - (cycles3 - cycles2);
		if(funcTime > 1)
		{
			totalCyclesUsed += funcTime;
		}
	}
end:
	// calculate time for all iters as a ratio, because we burned some as warmups
	return timedResult<T>(totalCyclesUsed*(iters)/totalTimed, result);
	
}

