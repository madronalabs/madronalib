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
const char* b = "fff";
const char* c = "fff";
const char* f() { return b; }

int main()
{
	/*
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
	
	 */
//	constexpr HashedStringLiteral(const char (&sym)[N]) : hash(KRHashLen(sym, N)), pSym(sym), len(N) { }

//	constexpr HashedStringLiteral al("abc") ;
//	std::cout << al.pSym << " " << al.hash << "\n";
	
	/*
	TestProc p;
	p.setParam("abc", 880.f);
	std::cout << p.getParam(MLSymbol("abc")) << "\n";
	*/


//	HashedStringLiteral s1 ("abcd"); // OK
//	HashedStringLiteral s2 ("b"); // OK
//	std::cout << std::hex << s1.hash << std::dec;
//	std::cout << s2.hash;
	
//	MLSymbol a1("abcd");
//	MLSymbol a2("efgh");
	
//	MLSymbol a2("b");
//	std::cout << a1.getHash() << "\n";
//	std::cout << a2.getHash() << "\n";
	
	//std::cout << std::hex << a1.getHash() << std::dec << "\n";
	//std::cout << std::hex << a1.getID() << std::dec << "\n";
	
	/*
	for(int i=0; i<s1.hash; ++i)
	{
		std::cout << ":";
	}
	for(int i=0; i<s2.hash; ++i)
	{
		std::cout << ".";
	}
	//	MLSymbol aa("abs");
*/
	
	
	/*
	char b[5] = "foo";	
	MLSymbol bb(b);
	
	MLSymbol cc(c);

	char buffer[10] = "lkj";
	char* d = buffer;
	
	MLSymbol dd(d);
	*/
	
	TestProc p;
	p.setParam("abcd", 880.f);
	
	
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
	
//	std::cout << s1.hash << "\n";
//	std::cout << s2.hash << "\n";
	
	
//	std::cout << aa.getID();
//	std::cout << bb.getID();
//	std::cout << cc.getID();
//	std::cout << dd.getID();

	return 0;
}



