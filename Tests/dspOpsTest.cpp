
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
                << ", precis: " << preciseMaxDiff
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
    int iters = 1000;

    std::cout << "nanoseconds for " << iters << " iterations:\n";
    int i = 0;
    for (auto fnVec : functionVectors)
    {
      timedResult<DSPVector> fnTimeNative =
          timeIterations<DSPVector>(fnVec.second[0], iters);
      timedResult<DSPVector> fnTimePrecise =
          timeIterations<DSPVector>(fnVec.second[1], iters);
      timedResult<DSPVector> fnTimeApprox =
          timeIterations<DSPVector>(fnVec.second[2], iters);
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
    auto a{repeat<rows>(columnIndex())};

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

    DSPVectorArray<2> a{repeat<2>(columnIndex())};
    auto a2{a * 2.f};

    DSPVector b{columnIndex()};
    auto b2 = b * 2.f;

    DSPVectorArray<2> x{3.f};
    DSPVectorArray<1> y{3.f};
    auto xy = x * repeat<2>(y);
    auto yx = repeat<2>(y) * x;

    auto e = a * repeat<2>(b);

    auto aa = repeat<4>(a);

    auto f{repeat<2>(columnIndex())};
    auto g = map([&](DSPVector x, int j) { return x * (j + 1); }, f);

    auto h = stretch<6>(g);

    auto k = zeroPad<6>(columnIndex());

    auto m = rotateRows(k, -1) * 3.f;

    auto n = shiftRows(k, 2);
    // TODO actual tests
  }
}

// TODO tests into units

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
