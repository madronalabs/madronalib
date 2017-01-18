
#pragma once

#include "../source/DSP/MLDSP.h"
#include "MLProperty.h"
#include "MLPropertySet.h"

using namespace ml;


// constexpr string courtesy of Scott Schurr
class str_const 
{ 
	//private:
public:	
	const char* const p_;
	const std::size_t  size_;
	
public:
	template<std::size_t N>
	constexpr str_const(const char(&a)[N]) : 
	p_(a), size_(N-1) {}
	
	constexpr char operator[](std::size_t n) const 
	{
		return n < size_ ? p_[n] :
		throw std::out_of_range("");
	}
	
	constexpr std::size_t size() const 
	{ 
		return size_; 
	}
	
	friend constexpr bool operator==(const str_const& a, const str_const& b );
	
};

constexpr bool charArraysEqual(char const * a, char const * b)
{
	return (*a && *b) ? 
	(*a == *b && charArraysEqual(a + 1, b + 1)) :
	(!*a && !*b);
}

constexpr bool operator==(const str_const& a, const str_const& b ) 
{
	return charArraysEqual(a.p_, b.p_);
}

inline std::ostream& operator<< (std::ostream& out, const str_const & r)
{
	out << r.p_;
	return out;
}	

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N] ) noexcept
{
	return N;
}


constexpr int getIndex (const str_const* begin, int i, int N, str_const const& value)
{
	return (! ((i != N) && !(*begin == value))) ?
	(i) :
	getIndex (begin + 1, i + 1, N, value);		
}

// returns the length N if not found, otherwise the index of the array element equal to str.
template <std::size_t N>
constexpr int find(str_const const(&array)[N], str_const str)
{ 
	return getIndex(&(array[0]), 0, N, str);
}

// MLTEST proc class
// compiler needs to be able to query the functor / proc about its i/o size possibilities
// to turn bytecode into a list of process() calls

class Proc
{
public:
	virtual void process() = 0;
	virtual void setInput(str_const str, DSPVector &v) = 0;
	virtual void setOutput(str_const str, DSPVector &v) = 0;
	
};

/*
 
 class MLProcAdd : public MLProc
 {
 public:
	void process(const int frames) override;		
	MLProcInfoBase& procInfo() override { return mInfo; }
 
 private:
	MLProcInfo<MLProcAdd> mInfo;
 };
 
 // ----------------------------------------------------------------
 // registry section
 
 namespace
 {
	MLProcRegistryEntry<MLProcAdd> classReg("add");
	ML_UNUSED MLProcInput<MLProcAdd> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcAdd> outputs[] = {"out"};
 }	
 
 */
