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
#include "MLTextFragment.h"


const char a[] = "x";
const char* b = "fff";
const char* c = "fff";
const char* f() { return b; }

int main()
{
	const char * letters("abcd");
	
	uint32_t hashTest = krHash0(letters, strlen(letters));	
	
	std::cout << std::hex << hashTest << std::dec << "\n";
	
	std::cout << "Symbols example:\n";
	
	std::cout << "\n\nTESTING sym param name ";
	
	TestProc p;
	
	p.setParam("9xo", 880.f);
	p.setParam("frequencies", 440.f);
	p.setParam("frequency", 440.f);
	
	char name1[4] = "foo";
	name1[1] = 'x';

	const char name2[4] = "fxo";
	
	// Foo::Foo(const char (&)[N]) [N = 2]
	std::cout << "frequency " << p.getParam("frequency") << "\n";
	
	// Foo::Foo(T, typename IsCharPtr<T>::Type) [T = char *]
	std::cout << "name1 (not const) " << p.getParam(name1) << "\n";
	
	// Foo::Foo(const char (&)[N]) [N = 2]
	std::cout << "name2 (const) " << p.getParam(name2) << "\n";
	
	// Foo::Foo(T, typename IsCharPtr<T>::Type) [T = const char *]
	std::cout << "ff " << p.getParam(b) << "\n";
		
	theSymbolTable().dump();
	
	
	TextFragment t ("hello");
	TextFragment u (",");
	TextFragment v (" ");
	TextFragment w ("world!");
	
	std::cout << t << u << v << w << "\n";
	
	/*
	std::chrono::time_point<std::chrono::system_clock> start, end;	
	std::chrono::duration<double> elapsed;
	start = std::chrono::system_clock::now();	
	end = std::chrono::system_clock::now();
	elapsed = end-start;	
	std::cout << "resource map elapsed time: " << elapsed.count() << "s\n";

	RandomSource rand;
	
	int rs = rand.getIntSample();
	rs = rand.getIntSample();
	
	int q = s1.hash + s2.hash + rs;
	
	float r = q + elapsed.count();
	r *= 0.5f;
	std::cout << r << "\n";
	*/
	

	
	
	
	return 0;
}



