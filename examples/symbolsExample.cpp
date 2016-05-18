//
//  symbolsExample.cpp
//  madronalib
//
//  Created by Randy Jones on 5/17/16.
//
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "catch.hpp"
#include "../include/madronalib.h"
#include "MLStringUtils.h"

#include <iostream>

#include "madronalib.h"



const char a[] = "x";
const char* b = "x";
const char* f() { return b; }




int main()
{
	std::cout << "Symbols example:\n";
	
	std::cout << "\n\nTESTING sym param name ";
	
	TestProc p;
	
	p.setParam("frequency", 440.f);
	p.setParam("fxo", 880.f);
	
	char name1[4] = "foo";
	name1[1] = 'x';

	const char name2[4] = "fxo";
	
	std::cout << "frequency " << p.getParam("frequency") << "\n";
	std::cout << "name1 (not const) " << p.getParam(name1) << "\n";
	std::cout << "name2 (const) " << p.getParam(name2) << "\n";
	std::cout << "nothing " << p.getParam("nothing") << "\n";
		
  char buffer[10] = "lkj";
  char* c = buffer;
  MLSymbol l("x");     // Foo::Foo(const char (&)[N]) [N = 2]
  MLSymbol aa(a);      // Foo::Foo(const char (&)[N]) [N = 2]
  MLSymbol bb(b);      // Foo::Foo(T, typename IsCharPtr<T>::Type) [T = const char *]
  MLSymbol cc(c);      // Foo::Foo(T, typename IsCharPtr<T>::Type) [T = char *]
  MLSymbol ee(buffer); // Foo::Foo(char (&)[N]) [N = 10]
  MLSymbol ff(f());    // Foo::Foo(T, typename IsCharPtr<T>::Type) [T = const char *]
  return 0;
	

	
	
}



