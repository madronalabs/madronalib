//  madronalib
//  tests.h

#include <chrono>
#include <deque>
#include <thread>
#include <iostream>

#include "MLDSPProjections.h"

using namespace std::chrono;

#if (__APPLE__)
  #include <pthread.h>
#endif

// TODO this could be its own module with tests
template <class T> struct Stats
{
    static constexpr size_t kMaxSampleSize{1000};
    
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
        
        recentSamples.push_back(x);
        if(recentSamples.size() > kMaxSampleSize)
        {
            recentSamples.pop_front();
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
    
    T median()
    {
        std::sort(recentSamples.begin(), recentSamples.end());
        size_t n = recentSamples.size();
        if(n == 0)
        {
            return T();
        }
        else if (n & 1)
        {
            return recentSamples[(n - 1)/2];
        }
        else
        {
            return (recentSamples[n/2] + recentSamples[n/2 - 1])/2;
        }
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
    
    void removeOutliers()
    {
        T medianValue = median();
        T stdDev = standardDeviation();
        T loBounds = medianValue - stdDev;
        T hiBounds = medianValue + stdDev;
        
        recentSamples.erase(std::remove_if(recentSamples.begin(),
                                           recentSamples.end(),
                                           [&](const T x)-> bool
                                            { return (x < loBounds)||(x > hiBounds); }),
                                            recentSamples.end());
    }
    
	int mCount{0};
	T m_oldM, m_newM, m_oldS, m_newS;
    T m_min{0.};
    T m_max{0.};
    std::deque< T > recentSamples;
};

template <class T> struct TimedResult
{
 	double ns;
	T value;
};

template <class T> struct WorkerData
{
    std::function<T(void)>* workFn;
    Stats< double > stats;
    TimedResult< T > result;
};


// Repeat running a function and timing it, throwing out outliers. This does a reasonable
// job for benchmarking using a two-pass procedure: first roughly measure the execution time
// and then time the number of iterations that will take 0.01s or so.

template <class T> inline TimedResult<T> timeIterations(std::function<T(void)> func)
{
	T result{};
	Stats<double> durationStats;
	
	// run once in order to prime cache
	result += func();
    
    // roughly time the function
    constexpr int kRoughTimeIters{1000};
    auto roughStart = high_resolution_clock::now();
    for(int i=0; i<kRoughTimeIters; ++i)
    {
        // add result to the return value so the call is not optimized away
        result += func();
    }
    auto roughEnd = high_resolution_clock::now();
    auto roughNanosTotal = duration_cast<nanoseconds>(roughEnd - roughStart).count();
    double roughNanosPerIteration = roughNanosTotal/(kRoughTimeIters + 0.f);
    
    // use the number of iterations of the function that will take roughly 0.1s
    constexpr int itersChunkSize{100};
    double roughRunTimeInSecs = 1e-1;
    double roughSecsPerIteration = roughNanosPerIteration*1e-9;
    int itersToTime = roughRunTimeInSecs / roughSecsPerIteration;
    itersToTime = std::clamp(itersToTime, itersChunkSize, 1000000);
	for(int i=0; i<itersToTime/itersChunkSize; ++i)
	{
		T fnResult{};
		
		// get the time we use
		auto startOne = high_resolution_clock::now();
        for(int j=0; j < itersChunkSize; ++j)
        {
            fnResult = func();
        }
		auto endOne = high_resolution_clock::now();

		// add result to the return value so the call is not optimized away
		result += fnResult;
		
		auto dur = duration_cast<nanoseconds>(endOne - startOne).count();
        double singleDur = dur/(itersChunkSize + 0.);
		if(singleDur > 0.)
		{
			durationStats.accumulate(singleDur);
		}
	}
	
    durationStats.removeOutliers();
    double medianTime = durationStats.median();
	
    return TimedResult< T >{medianTime, result};
}


#if (__APPLE__)

template <class T> inline void* workerFn(void* dataPtr)
{
    T resultValue{};
    auto pData = static_cast< WorkerData< T >* >(dataPtr);
    
    // run once in order to prime cache
    resultValue += (*pData->workFn)();
    
    // roughly time the function
    constexpr int kRoughTimeIters{1000};
    auto roughStart = high_resolution_clock::now();
    for(int i=0; i<kRoughTimeIters; ++i)
    {
        // add result to the return value so the call is not optimized away
        resultValue += (*pData->workFn)();
    }
    auto roughEnd = high_resolution_clock::now();
    auto roughNanosTotal = duration_cast<nanoseconds>(roughEnd - roughStart).count();
    double roughNanosPerIteration = roughNanosTotal/(kRoughTimeIters + 0.f);
    
    // use the number of iterations of the function that will take roughly 0.1s
    constexpr int itersChunkSize{100};
    double roughRunTimeInSecs = 1e-1;
    double roughSecsPerIteration = roughNanosPerIteration*1e-9;
    double itersToTime = roughRunTimeInSecs / roughSecsPerIteration;
    itersToTime = std::clamp(itersToTime, itersChunkSize + 0., 1e9);
    
    auto startAll = high_resolution_clock::now();
    for(int i=0; i<itersToTime/itersChunkSize; ++i)
    {
        T fnResult{};
        
        // get the time we use
        auto startOne = high_resolution_clock::now();
        for(int j=0; j < itersChunkSize; ++j)
        {
            fnResult = (*pData->workFn)();
        }
        auto endOne = high_resolution_clock::now();

        // add result to the return value so the call is not optimized away
        resultValue += fnResult;
        
        auto dur = duration_cast<nanoseconds>(endOne - startOne).count();
        double singleDur = dur/(itersChunkSize + 0.);
        if(singleDur > 0.)
        {
            pData->stats.accumulate(singleDur);
        }
    }
    auto endAll = high_resolution_clock::now();
    auto durAll = duration_cast<nanoseconds>(endAll - startAll).count();
    //std::cout << "(" << itersToTime << " iters in " << durAll << "ns )\n";
    
    pData->stats.removeOutliers();
    double medianTime = pData->stats.median();
    
    pData->result = TimedResult< T >{medianTime, resultValue};
    
    pthread_exit( NULL );
}

template <class T> inline TimedResult<T> timeIterationsInThread(std::function<T(void)> func)
{
    WorkerData< T > wData;
    wData.workFn = &func;
    
    pthread_t worker;
    pthread_attr_t qosAttribute;
    pthread_attr_init(&qosAttribute);
    pthread_attr_set_qos_class_np(&qosAttribute, QOS_CLASS_USER_INITIATED, 0);

    auto result = pthread_create(&worker, &qosAttribute, &workerFn< T >, &wData);
    if(result)
    {
        std::cout << "timeIterationsInThread: failed to create thread!\n";
    }
    
    void* status;
    auto joinResult = pthread_join(worker, &status);
    if(joinResult)
    {
        std::cout << "timeIterationsInThread: error in join!\n";
    }
    
    return wData.result;
}


#endif
