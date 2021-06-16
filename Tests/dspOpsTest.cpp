
//
// dspOpsTest
// a unit test made using the Catch framework in catch.hpp / tests.cpp.
//

#include <chrono>
#include <iostream>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>

#include "catch.hpp"
#include "madronalib.h"
#include "mldsp.h"
#include "tests.h"

using namespace ml;

TEST_CASE("madronalib/core/dsp_ops", "[dsp_ops]")
{
  DSPVector a(rangeClosed(-kPi, kPi));

  auto sinN = ([&]() {
    DSPVector v;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      v[i] = sinf(a[i]);
    }
    return v;
  });
  auto sinP = ([&]() { return sin(a); });
  auto sinA = ([&]() { return sinApprox(a); });
  std::vector<std::function<DSPVector(void)> > sinFunctions{sinN, sinP, sinA};

  auto cosN = ([&]() {
    DSPVector v;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      v[i] = cosf(a[i]);
    }
    return v;
  });
  auto cosP = ([&]() { return cos(a); });
  auto cosA = ([&]() { return cosApprox(a); });
  std::vector<std::function<DSPVector(void)> > cosFunctions{cosN, cosP, cosA};

  auto logN = ([&]() {
    DSPVector v;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      v[i] = logf(a[i]);
    }
    return v;
  });
  auto logP = ([&]() { return log(a); });
  auto logA = ([&]() { return logApprox(a); });
  std::vector<std::function<DSPVector(void)> > logFunctions{logN, logP, logA};

  auto expN = ([&]() {
    DSPVector v;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      v[i] = expf(a[i]);
    }
    return v;
  });
  auto expP = ([&]() { return exp(a); });
  auto expA = ([&]() { return expApprox(a); });
  std::vector<std::function<DSPVector(void)> > expFunctions{expN, expP, expA};

  std::vector<
      std::pair<std::string, std::vector<std::function<DSPVector(void)> > > >
      functionVectors{{"sin", sinFunctions},
                      {"cos", cosFunctions},
                      {"log", logFunctions},
                      {"exp", expFunctions}};

  SECTION("precision")
  {
    // test precision of sin, cos, log, exp and approximations
    // use native math as reference
    std::cout << "max differences from reference:\n";

    for (auto fnVec : functionVectors)
    {
      DSPVector native = fnVec.second[0]();
      DSPVector precise = fnVec.second[1]();
      DSPVector approx = fnVec.second[2]();

      float nativeMaxDiff = max(abs(native - native));
      float preciseMaxDiff = max(abs(native - precise));
      float approxMaxDiff = max(abs(native - approx));

      std::cout << fnVec.first << " native: " << nativeMaxDiff
                << ", precise: " << preciseMaxDiff
                << ", approx: " << approxMaxDiff << " \n";

      // these differences are to accommodate the exp functions, the other ones
      // are a lot more precise.
      REQUIRE(preciseMaxDiff < 2e-6f);
      REQUIRE(approxMaxDiff < 2e-4f);
    }
  }

  SECTION("time")
  {
    // test speed of precise functions relative to native ones.
    // test speed of approximate functions relative to precise ones.
    // approximate ones should be faster!
    std::cout << "nanoseconds per iteration:\n";

    int i = 0;
    for (auto fnVec : functionVectors)
    {
        
        // temporarily we have a separate timing function for Apple Silicon, which
        // tries to runs the test on a performance core.
        
#if (defined __ARM_NEON) || (defined __ARM_NEON__)
        TimedResult<DSPVector> fnTimeNative =
        timeIterationsInThread<DSPVector>(fnVec.second[0]);
        TimedResult<DSPVector> fnTimePrecise =
        timeIterationsInThread<DSPVector>(fnVec.second[1]);
        TimedResult<DSPVector> fnTimeApprox =
        timeIterationsInThread<DSPVector>(fnVec.second[2]);
#else
        TimedResult<DSPVector> fnTimeNative =
        timeIterations<DSPVector>(fnVec.second[0]);
        TimedResult<DSPVector> fnTimePrecise =
        timeIterations<DSPVector>(fnVec.second[1]);
        TimedResult<DSPVector> fnTimeApprox =
        timeIterations<DSPVector>(fnVec.second[2]);
#endif
        
        std::cout << fnVec.first << " native: " << fnTimeNative.ns
                  << ", precise: " << fnTimePrecise.ns
                  << ", approx: " << fnTimeApprox.ns << " \n";
        i++;
    }
  }

  SECTION("lerp")
  {
    DSPVector a{columnIndex()};
    DSPVector b{0.f};
    auto c = lerp(a, b, 0.5f);
    REQUIRE(c[kFloatsPerDSPVector - 1] == (kFloatsPerDSPVector - 1) * 0.5f);
  }

  SECTION("map")
  {
    constexpr int rows = 2;
    auto a{repeatRows<rows>(columnIndex())};

    // map void -> float
    auto b = map([&]() { return 4; }, a);

    // map float -> float
    auto c = map([&](float x) { return x * 2.f; }, a);

    // map int -> float
    auto d = map([&](int x) { return x * 2; }, a);

    // map DSPVector -> DSPVector
    auto e = map([&](DSPVector x) { return x * 2.f; }, a);

    // map DSPVector, int row -> DSPVector
    auto f = map([&](DSPVector x, int j) { return j * 2; }, a);

    REQUIRE(c == d);
    REQUIRE(d == e);
  }

  SECTION("row operations")
  {
    std::cout << "\nROW OPERATIONS\n";

    DSPVectorArray<2> a{repeatRows<2>(columnIndex())};
    auto a2{a * 2.f};

    DSPVector b{columnIndex()};
    auto b2 = b * 2.f;

    DSPVectorArray<2> x{3.f};
    DSPVectorArray<1> y{3.f};
    auto xy = x * repeatRows<2>(y);
    auto yx = repeatRows<2>(y) * x;

    auto e = a * repeatRows<2>(b);

    auto aa = repeatRows<4>(a);

    auto f{repeatRows<2>(columnIndex())};
    auto g = map([&](DSPVector x, int j) { return x * (j + 1); }, f);

    auto h = stretchRows<6>(g);

    auto k = zeroPadRows<6>(columnIndex());

    auto m = rotateRows(k, -1) * 3.f;

    auto n = shiftRows(k, 2);
    // TODO actual tests
  }
  
  SECTION("combining")
  {
    std::cout << "\nCOMBINING\n";
    
    DSPVectorArray<2> a{repeatRows<2>(columnIndex())};
    DSPVectorArray<2> b{rowIndex<2>() + 1};
    DSPVectorArray<2> c{1};

    auto sum = add(a, b, c);
    
    DSPVectorArray<3> gains = concatRows(DSPVector{0.300f}, DSPVector{0.030f}, DSPVector{0.003f});

    auto mixResult = mix(gains, c, c, c);
    
    DSPVectorArray<6> gg = repeatRows<2>(gains);

    DSPVectorArray<2> h = separateRows<4, 6>(gg);
    
    // TODO tests
  }
  
  SECTION("multiplex")
  {
    std::cout << "\nMULTIPLEX\n";
    
    DSPVectorArray<2> input{columnIndex<2>() + rowIndex<2>()};
    DSPVectorArray<2> a{7};
    DSPVectorArray<2> b{11};
    DSPVectorArray<2> c{13};
    DSPVectorArray<2> d{17};
    DSPVectorArray<2> e{19};

    // rangeOpen(0-1): equal amounts of a, b, c, d, e
    auto dv = multiplex(rangeOpen(0, 1), a, b, c, d, e);
    REQUIRE(dv[kFloatsPerDSPVector - 1] == e[kFloatsPerDSPVector - 1]);
    
    // rangeClosed(0, 4.f/5.f): last element should be e
    auto dw = multiplexLinear(rangeClosed(0, 4.f/5.f), a, b, c, d, e);
    REQUIRE(dv[kFloatsPerDSPVector - 1] == e[kFloatsPerDSPVector - 1]);

    // the sum of the linear demultiplexer's outputs should equal the input
    auto demuxInput2 = repeatRows<2>(DSPVector{1});
    demultiplexLinear(rangeClosed(0, 3.f/4.f), demuxInput2, &a, &b, &c, &d);
    auto sumOfDemuxOutputs2 = add(a, b, c, d);
    REQUIRE(demuxInput2 == sumOfDemuxOutputs2);
    REQUIRE(sumOfDemuxOutputs2[kFloatsPerDSPVector - 1] == 1);
    
    // demultiplex to multiple outputs, then multiplex wtih same selector
    // should equal the input
    auto selectorSignal = rangeOpen(0, 1);
    auto demuxInput3 = columnIndex<2>();
    demultiplex(selectorSignal, demuxInput3, &a, &b, &c, &d);
    auto demuxThenMux = multiplex(selectorSignal, a, b, c, d);
    REQUIRE(demuxInput3 == demuxThenMux);
  }
  
  SECTION("bank")
  {
    std::cout << "\nBANK\n";
    constexpr size_t n = 5;
    
    // process with two arguments
    Bank<PulseGen, n> pulses;
    auto freqs = rowIndex<n>()*0.01 + 0.1;
    auto widths = rowIndex<n>()*0.01 + 0.5;
    auto pulseOuts = pulses(freqs, widths);
    
    // process with one argument
    Bank<SineGen, n> sines;
    auto sineOuts = sines(freqs);
    
    // process with no arguments
    Bank<NoiseGen, n> noises;
    auto noiseOuts = noises();
    
    // access one processor directly
    noises[2].step();
  }
}

TEST_CASE("madronalib/core/projections", "[projections]")
{
  std::cout << "\n\nPROJECTIONS\n";
  {
    auto pa = projections::piecewiseLinear({3, 5, 8});
    int size = 20;
    for (int i = 0; i <= size; ++i)
    {
      float fx = i / (size + 0.f);
      std::cout << fx << " -> " << pa(fx) << "\n";
    }
  }

  {
    auto pa = projections::piecewise(
        {1, 2, 3}, {projections::easeIn, projections::easeOut});
    int size = 20;
    for (int i = 0; i <= size; ++i)
    {
      float fx = i / (size + 0.f);
      std::cout << fx << " -> " << pa(fx) << "\n";
    }
  }
}

