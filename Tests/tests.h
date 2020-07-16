//  madronalib
//  tests.h

#include <chrono>
#include "mldsp.h"

using namespace ml;
using namespace std::chrono;

// TODO this could be its own module with tests
template <class T> class Stats
{
public:
	
	void accumulate(T x)
	{
		mCount++;
		
		// compute a running mean.
		// See Knuth TAOCP vol 2, 3rd edition, page 232
		if (mCount == 1)
		{
			m_oldM = m_newM = x;
			m_oldS = T();
			m_min = m_max = x;
		}
		else
		{
			m_newM = (m_oldM + ((x - m_oldM)/mCount));
			m_newS = (m_oldS + ((x - m_oldM)*(x - m_newM)));
			
			// set up for next iteration
			m_oldM = m_newM;
			m_oldS = m_newS;
			
			m_max = std::max(x, m_max);
			m_min = std::min(x, m_min);
		}
	}
	
	int getCount()
	{
		return mCount;
	}
	
	T mean() const
	{
		return (mCount > 0) ? m_newM : T();
	}
	
	T variance() const
	{
		return ( (mCount > 1) ? (m_newS/(mCount - 1)) : T() );
	}
	
	T standardDeviation() const
	{
		return sqrt( variance() );
	}
	
	T min() const { return m_min; }
	T max() const { return m_max; }
	
private:
	int mCount{0};
	T m_oldM, m_newM, m_oldS, m_newS;
	T m_min{0};
	T m_max{0};
};

template <class T> class timedResult
{
public:
	timedResult(double time, const T& _r) : ns(time), result(_r){}
	double ns;
	T result;
};


// Repeat running a function and timing it, throwing out outliers. This does a reasonable job for benchmarking.
// Better would be a two-pass procedure: first roughly measure the execution time and then time the number of
// iterations that will equal the time period that gives the most accurate results - .5ms or so, anecdotally.

template <class T> inline timedResult<T> timeIterations(std::function<T(void)> func, int iters)
{
	std::chrono::time_point<std::chrono::high_resolution_clock> startOne, endOne;
	
	T result{};
	std::vector<double> times;
	Stats<double> durationStats;
	
	// run once in order to prime cache
	result += func();
	
	for(int i=0; i<iters; ++i)
	{
		T fnResult{};
		
		// get the time we use
		startOne = high_resolution_clock::now();
		fnResult = func();
		endOne = high_resolution_clock::now();

		// add result to the return value so the call is not optimized away
		result += fnResult;
		
		auto dur = duration_cast<nanoseconds>(endOne - startOne).count();
		if(dur > 0)
		{
			times.push_back(dur);
			durationStats.accumulate(dur);
		}
	}
	
	double mean = durationStats.mean();
	double stdDev = durationStats.standardDeviation();
	double loBounds = mean - stdDev*0.5;
	double hiBounds = mean + stdDev*0.5;
	
	// make vector of good times without outliers
	std::vector<double> goodTimes;
	std::copy_if(times.cbegin(), times.cend(), std::back_inserter(goodTimes), [=](double x){ return (x >= loBounds)&&(x <= hiBounds); });
	
	// get median of good times
	double medianTime = 0.;
	if(goodTimes.size() > 1)
	{
		std::sort(goodTimes.begin(), goodTimes.end());
		medianTime = goodTimes[goodTimes.size()/2];
	}
	
	return timedResult<T>(medianTime, result);
}

