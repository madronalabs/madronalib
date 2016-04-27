//  madronalib
//  tests.h

#include <chrono>
#include "../DSP/MLDSPOps.h"

template <class T> class timedResult
{
public:
	timedResult(double time, T _r) : elapsedTime(time), result(_r){}
	double elapsedTime;
	T result;
};

template <class T> inline timedResult<T> timeIterations(std::function<T(void)> func, int iters)
{	
	std::chrono::time_point<std::chrono::system_clock> start, end;
	std::chrono::duration<double> elapsed;
	
	start = std::chrono::system_clock::now();
	
	T result;
	for(int i=0; i<iters; ++i)
	{
		// do the thing, returning a value so the function call is not optimized away
		result = func();
	}
	
	end = std::chrono::system_clock::now();
	elapsed = end-start;
	
	return timedResult<T>(elapsed.count(), result);
}

class timedDSPVectorResult
{
public:
	timedDSPVectorResult(double time, DSPVector _r) : elapsedTime(time), result(_r){}
	double elapsedTime;
	DSPVector result;
};

inline timedDSPVectorResult timeDSPVectorIterations(std::function<DSPVector(void)> func, int iters)
{	
	std::chrono::time_point<std::chrono::system_clock> start, end;
	std::chrono::duration<double> elapsed;
	
	start = std::chrono::system_clock::now();
	
	DSPVector result;
	for(int i=0; i<iters; ++i)
	{
		// do the thing, returning a value so the function call is not optimized away
		result = func();
	}
	
	end = std::chrono::system_clock::now();
	elapsed = end-start;
	
	return timedDSPVectorResult(elapsed.count(), result);
}



