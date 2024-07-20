// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"
#include "testUtils.h"

const int longSleepMs{50};

using namespace ml;

TEST_CASE("madronalib/core/timer/basic", "[timer][basic]")
{
  // call this once in an application.
  bool deferToMainThread = false;
  SharedResourcePointer<ml::Timers> t;
  t->start(deferToMainThread);

  // calling start again should not break anything.
  SharedResourcePointer<ml::Timers> t2;
  t2->start(deferToMainThread);

  int sum = 0;

  // test number of calls correct
  const int testSize = 10;
  std::vector<std::unique_ptr<Timer> > v;
  for (int i = 0; i < testSize; ++i)
  {
    v.emplace_back(std::unique_ptr<Timer>(new Timer));
    v[i]->callNTimes(
        [&sum]() {
          sum += 1;
          // std::cout << ".";
        },
        milliseconds(10 + 20 * i), 2);
  }
  std::this_thread::sleep_for(milliseconds(longSleepMs));
  // std::cout << "timer sum: " << sum << "\n";
  
  // not working in GitHub Actions
  // REQUIRE(sum == 20);

  // test deleting timers while running (no REQUIRE)
  const int test2Size = 10;
  {
    std::vector<std::unique_ptr<Timer> > v2;
    for (int i = 0; i < test2Size; ++i)
    {
      v2.emplace_back(std::unique_ptr<Timer>(new Timer));
      
      // rare crash?
      // terminating with uncaught exception of type std::__1::bad_function_call: std::exception
      // in timer::run()
      // v2[i]->start([=]() { std::cout << i << " "; }, milliseconds(10 * i));
    }
    std::this_thread::sleep_for(milliseconds(longSleepMs));
  }

  // test stopping timers while running
  const int test3Size = 10;
  {
    int sum{0};
    // std::cout << "\n----\n";
    std::vector<std::unique_ptr<Timer> > v3;
    for (int i = 0; i < test3Size; ++i)
    {
      v3.emplace_back(std::unique_ptr<Timer>(new Timer));
      v3[i]->start(
          [&sum, i]() {
            sum++;
            // std::cout << i << " ";
          },
          milliseconds(10));
    }
    for (int i = 0; i < test3Size; ++i)
    {
      std::this_thread::sleep_for(milliseconds(10));
      v3[i]->stop();
    }

    // make sure all timers have stopped
    std::this_thread::sleep_for(milliseconds(100));
    sum = 0;
    std::this_thread::sleep_for(milliseconds(100));
    
    // not working in GitHub Actions
    // REQUIRE(sum == 0);
  }

  // temp
  /*
  typedef SmallStackBuffer<float, 64> vBuf;
  vBuf va(16);
  vBuf vb(128);
  vBuf *pvc = new vBuf(16);
  vBuf *pvd = new vBuf(128);

  std::cout << "\n\n a: " << &va << " b: " << &vb << " c: " << pvc
            << " d: " << pvd << "\n";
  std::cout << "\n\n a: " << va.data() << " b: " << vb.data()
            << " c: " << pvc->data() << " d: " << pvd->data() << "\n";
   */

#ifdef _WINDOWS
  system("pause");
#endif
}
