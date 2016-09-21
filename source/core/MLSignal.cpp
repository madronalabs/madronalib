
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// various operations on signals
// TODO organize

#include "MLSignal.h"

// ----------------------------------------------------------------
#pragma mark MLSignal


MLSignal MLSignal::nullSignal;

// no length argument: make a null object. MLTEST sort out any extant use of nulls and make this return default size with fastest possible ctor.

// TODO fast ctors for default size chunks seems a lot more imporant than a null object pattern. Look at where we are using the latter and do it some other way. 

// DSPutils can operate on the assumption of default size signals: 16 x 1 or whatever.
// these can coexist with slower matrix-like MLSignal methods that actually do range checking. 

MLSignal::MLSignal() : 
mDataAligned(0),
mData(0),
mWidth(0), mHeight(0), mDepth(0)
{
	mRate = kToBeCalculated;
	setDims(0);
}

MLSignal::MLSignal (int width, int height, int depth) : 
mDataAligned(0),
mData(0),
mWidth(0), mHeight(0), mDepth(0)
{
	mRate = kToBeCalculated;
	setDims(width, height, depth);
}

MLSignal::MLSignal(const MLSignal& other) :
mDataAligned(0),
mData(0),
mWidth(0), mHeight(0), mDepth(0)
{
	mSize = other.mSize;
	mData = allocateData(mSize);
	mDataAligned = initializeData(mData, mSize);
	mWidth = other.mWidth;
	mHeight = other.mHeight;
	mDepth = other.mDepth;
	mHeightBits = other.mHeightBits;
	mWidthBits = other.mWidthBits;
	mDepthBits = other.mDepthBits;
	mRate = other.mRate;
	std::copy(other.mDataAligned, other.mDataAligned + mSize, mDataAligned);
}

MLSignal::MLSignal (std::initializer_list<float> values) : 
mDataAligned(0),
mData(0),
mWidth(0), mHeight(0), mDepth(0)
{
	mRate = kToBeCalculated;
	setDims((int)values.size());
	int idx = 0;
	for(float f : values)
	{
		mDataAligned[idx++] = f;
	}
}

// constructor for making loops. only one type for now. we could loop in different directions and dimensions.
MLSignal::MLSignal(MLSignal other, eLoopType loopType, int loopSize) :
mDataAligned(0),
mData(0),
mWidth(0), mHeight(0), mDepth(0)
{
	switch(loopType)
	{
		case kLoopType1DEnd:
		default:
		{
			int w = other.getWidth();
			int loopWidth = ml::clamp(loopSize, 0, w);
			setDims(w + loopWidth, 1, 1);
			mRate = other.mRate;
			std::copy(other.mDataAligned, other.mDataAligned + w, mDataAligned);
			std::copy(other.mDataAligned, other.mDataAligned + loopWidth, mDataAligned + w);
		}
			break;
	}
}

MLSignal& MLSignal::operator= (const MLSignal& other)
{
	if (this != &other) // protect against self-assignment
	{
		if (mSize != other.mSize)
		{			
			// 0: free old data
			freeData();
			
			// 1: allocate new memory and copy the elements
			mSize = other.mSize;
			mData = allocateData(mSize);
			mDataAligned = initializeData(mData, mSize);
			std::copy(other.mDataAligned, other.mDataAligned + mSize, mDataAligned);
			
			// 3: copy other information
			mWidth = other.mWidth;
			mHeight = other.mHeight;
			mDepth = other.mDepth;
			mHeightBits = other.mHeightBits;
			mWidthBits = other.mWidthBits;
			mDepthBits = other.mDepthBits;
			mRate = other.mRate;
		}
		else 
		{
			// keep existing data buffer.
			// copy other elements
			std::copy(other.mDataAligned, other.mDataAligned + this->mSize, mDataAligned);
			
			// copy other info
			mWidth = other.mWidth;
			mHeight = other.mHeight;
			mDepth = other.mDepth;
			mHeightBits = other.mHeightBits;
			mWidthBits = other.mWidthBits;
			mDepthBits = other.mDepthBits;
			mRate = other.mRate;
		}
	}
	return *this;
}

// MLTEST TODO move ctor!
// mark noexcept

// TODO use row(), column(), plane() submatrix syntax instead. 
// then current getBuffer().row(1) turns into row(1).getBuffer()
//
// private signal constructor: make a reference to a slice of the external signal.
// of course this object will be meaningless when the other Signal is gone, so
// use wih care -- only as a temporary, ideally.  Is there a way to force 
// the object to be a temporary?  In other words not allow a named 
// MLSignal notTemporary(pOther, 4);
//
// NOTE this signal will not pass checkIntegrity()!
//
MLSignal::MLSignal(const MLSignal* other, int slice) : 
mDataAligned(0),
mData(0)
{
	mRate = kToBeCalculated;
	if(other->getDepth() > 1) // make 2d slice
	{
		mDataAligned = other->mDataAligned + other->plane(slice);
		mWidth = other->mWidth;
		mHeight = other->mHeight;
		mDepth = 1;
	}
	else if(other->getHeight() > 1) // make 1d slice
	{
		mDataAligned = other->mDataAligned + other->row(slice);
		mWidth = other->mWidth;
		mHeight = 1;
		mDepth = 1;
	}
	else
	{
		// signal to take slice of must be 2d or 3d!
		assert(false);
	}
	mWidthBits = ml::bitsToContain(mWidth);
	mHeightBits = ml::bitsToContain(mHeight);
	mDepthBits = ml::bitsToContain(mDepth);
	mSize = 1 << mWidthBits << mHeightBits << mDepthBits;
}

MLSignal::~MLSignal() 
{
	freeData();
}

MLSignal MLSignal::getDims()
{
	if(mDepth > 1)
	{
		return MLSignal{static_cast<float>(mWidth), static_cast<float>(mHeight), static_cast<float>(mDepth)};
	}
	else if(mHeight > 1)
	{
		return MLSignal{static_cast<float>(mWidth), static_cast<float>(mHeight)};
	}
	else
	{
		return MLSignal{static_cast<float>(mWidth)};
	}
}

float* MLSignal::setDims (int width, int height, int depth)
{
	if((mWidth == width) && (mHeight == height) && (mDepth == depth))
	{
		return mDataAligned;
	}
	
	freeData();
	
	mWidth = width;
	mHeight = height;
	mDepth = depth;
	mWidthBits = ml::bitsToContain(width);
	mHeightBits = ml::bitsToContain(height);
	mDepthBits = ml::bitsToContain(depth);
	mSize = 1 << mWidthBits << mHeightBits << mDepthBits;
	
	mData = allocateData(mSize);	
	mDataAligned = initializeData(mData, mSize);	
	return mDataAligned;
}

float* MLSignal::setDims(const MLSignal& whd)
{
	switch(whd.getWidth())
	{
		case 1:
		default:
			setDims(static_cast<int>(whd[0]));
			break;
		case 2:
			setDims(static_cast<int>(whd[0]), static_cast<int>(whd[1]));
			break;
		case 3:
			setDims(static_cast<int>(whd[0]), static_cast<int>(whd[1]), static_cast<int>(whd[2]));
			break;
	}
	return mDataAligned;
}


int MLSignal::getFrames() const
{ 		
	if (mRate != kTimeless)
	{
		if (mHeightBits) // if 2D
		{
			return mHeight;
		}
		else // 1D
		{
			return mWidth;
		}
	}
	else
	{
		// not a time series
		return 1;
	}
}


/*
 const float MLSignal::operator()(const float fi, const float fj) const
 {
	float a, b, c, d;
	
	int i = (int)(fi);
	int j = (int)(fj);
	
	// get truncate down for inputs < 0
	// TODO use vectors with _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
	// _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);	
	if (fi < 0) i--;
	if (fj < 0) j--;
	float ri = fi - i;
	float rj = fj - j;
	
	int i1ok = ml::within(i, 0, mWidth);
	int i2ok = ml::within(i + 1, 0, mWidth);
	int j1ok = ml::within(j, 0, mHeight);
	int j2ok = ml::within(j + 1, 0, mHeight);
	
	a = (j1ok && i1ok) ? mDataAligned[row(j) + i] : 0.f;
	b = (j1ok && i2ok) ? mDataAligned[row(j) + i + 1] : 0.f;
	c = (j2ok && i1ok) ? mDataAligned[row(j + 1) + i] : 0.f;
	d = (j2ok && i2ok) ? mDataAligned[row(j + 1) + i + 1] : 0.f;
	
	return lerp(lerp(a, b, ri), lerp(c, d, ri), rj);
 }*/

/*
 // TODO SSE
 const float MLSignal::operator()(const Vec2& pos) const
 {
	return operator()(pos.x(), pos.y());
 }
 */
/*
 // TODO unimplemented
 const float MLSignal::operator() (const float , const float , const float ) const
 {
	return 0.;
 }
 
 const float MLSignal::operator() (const Vec3 ) const
 {
	return 0.;
 }
 */

// return const 2D signal made from a slice of the 3D data in place. 
const MLSignal MLSignal::getFrame(int i) const
{
	// only valid for 3D signals
	assert(mDepthBits > 1);
	
	// use slice constructor as return value.
	return MLSignal(this, i);
}

// setFrame() - set the 2D frame i to the incoming signal.
void MLSignal::setFrame(int i, const MLSignal& src)
{
	// only valid for 3D signals
	assert(is3D());
	
	// source must be 2D
	assert(src.is2D());
	
	// src signal should match our dimensions
	if((src.getWidth() != mWidth) || (src.getHeight() != mHeight))
	{
		return;
	}
	
	float* pDestFrame = mDataAligned + plane(i);
	const float* pSrc = src.getConstBuffer();
	std::copy(pSrc, pSrc + src.getSize(), pDestFrame);
}

//
#pragma mark I/O
// 

// read n samples from an external sample pointer plus a sample offset into start of signal.
//
void MLSignal::read(const float *input, const int offset, const int n)
{
	std::copy(input + offset, input + offset + n, mDataAligned);
}

// write n samples from start of signal to an external sample pointer plus a sample offset.
//
void MLSignal::write(float *output, const int offset, const int n)
{
	std::copy(mDataAligned, mDataAligned + n, output + offset);
}

// TODO SSE
void MLSignal::sigClamp(const MLSignal& a, const MLSignal& b)
{
	int n = ml::min(mSize, a.getSize());
	n = ml::min(n, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = ml::clamp(f, a.mDataAligned[i], b.mDataAligned[i]);
	}
}

void MLSignal::sigMin(const MLSignal& b)
{
	int n = ml::min(mSize, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = ml::min(f, b.mDataAligned[i]);
	}
}

void MLSignal::sigMax(const MLSignal& b)
{
	int n = ml::min(mSize, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = ml::max(f, b.mDataAligned[i]);
	}
}

// TODO SSE
void MLSignal::sigLerp(const MLSignal& b, const float mix)
{
	int n = ml::min(mSize, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] = ml::lerp(mDataAligned[i], b.mDataAligned[i], mix);
	}
}

// TODO SSE
void MLSignal::sigLerp(const MLSignal& b, const MLSignal& mix)
{
	int n = ml::min(mSize, b.getSize());
	n = ml::min(n, mix.getSize());
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] = ml::lerp(mDataAligned[i], b.mDataAligned[i], mix.mDataAligned[i]);
	}
}

//
#pragma mark binary ops
// 

// TODO SSE
bool MLSignal::operator==(const MLSignal& b) const
{
	if(mWidth != b.mWidth) return false;
	if(mHeight != b.mHeight) return false;
	if(mDepth != b.mDepth) return false;
	
	for(int i=0; i<mSize; ++i)
	{
		if(mDataAligned[i] != b.mDataAligned[i]) return false;
	}
	return true;
}

void MLSignal::copy(const MLSignal& b)
{
	const int n = ml::min(mSize, b.getSize());
	std::copy(b.mDataAligned, b.mDataAligned + n, mDataAligned);
}

void MLSignal::copyFast(const MLSignal& b)
{
	std::copy(b.mDataAligned, b.mDataAligned + mSize, mDataAligned);
}

/*
 // add the entire signal b to this signal, at the integer destination offset. 
 // 
 void MLSignal::add2D(const MLSignal& b, int destX, int destY)
 {
	MLSignal& a = *this;
	MLRect srcRect(0, 0, b.getWidth(), b.getHeight());
	MLRect destRect = srcRect.translated(Vec2(destX, destY)).intersect(getBoundsRect());
	
	for(int j=destRect.top(); j<destRect.bottom(); ++j)
	{
 for(int i=destRect.left(); i<destRect.right(); ++i)
 {
 a(i, j) += b(i - destX, j - destY);
 }
	}
 }
 
 // add the entire signal b to this signal, at the subpixel destination offset. 
 // 
 void MLSignal::add2D(const MLSignal& b, const Vec2& destOffset)
 {
	MLSignal& a = *this;
 
	Vec2 iDestOffset, fDestOffset;
	destOffset.getIntAndFracParts(iDestOffset, fDestOffset);
	
	int destX = iDestOffset[0];
	int destY = iDestOffset[1];	
	float srcPosFX = fDestOffset[0];
	float srcPosFY = fDestOffset[1];
	
	MLRect srcRect(0, 0, b.getWidth() + 1, b.getHeight() + 1); // add (1, 1) for interpolation
	MLRect destRect = srcRect.translated(iDestOffset).intersect(getBoundsRect());
	
	for(int j=destRect.top(); j<destRect.bottom(); ++j)
	{
 for(int i=destRect.left(); i<destRect.right(); ++i)
 {
 a(i, j) += b.getInterpolatedLinear(i - destX - srcPosFX, j - destY - srcPosFY);
 }
	}
 }
 */


/*
 // TEMP
 const float MLSignal::operator() (const float i, const float j) const
 {
 return getInterpolatedLinear(i, j);
 
 }*/

// TODO SSE
void MLSignal::add(const MLSignal& b)
{
	int n = ml::min(mSize, b.mSize);
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] += b.mDataAligned[i];
	}
}

// TODO SSE
void MLSignal::subtract(const MLSignal& b)
{
	int n = ml::min(mSize, b.mSize);
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] -= b.mDataAligned[i];
	}
}

// TODO SSE
void MLSignal::multiply(const MLSignal& b)
{
	int n = ml::min(mSize, b.mSize);
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] *= b.mDataAligned[i];
	}
}

// TODO SSE
void MLSignal::divide(const MLSignal& b)
{
	int n = ml::min(mSize, b.mSize);
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] /= b.mDataAligned[i];
	}
}


//
#pragma mark unary ops
// 

void MLSignal::fill(const float f)
{
	std::fill(mDataAligned, mDataAligned+mSize, f);
}

void MLSignal::scale(const float k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] *= k;
	}
}

void MLSignal::add(const float k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] += k;
	}
}

void MLSignal::subtract(const float k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] -= k;
	}
}

void MLSignal::subtractFrom(const float k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] = k - mDataAligned[i];
	}
}

// name collision with clamp template made this sigClamp
void MLSignal::sigClamp(const float min, const float max)	
{
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = ml::clamp(f, (float)min, (float)max);
	}
}

// TODO SSE
void MLSignal::sigMin(const float m)
{
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = ml::min(f, (float)m);
	}
}

// TODO SSE
void MLSignal::sigMax(const float m)	
{
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = ml::max(f, (float)m);
	}
}

// convolve a 1D signal with a 3-point impulse response.
void MLSignal::convolve3x1(const float km, const float k, const float kp)
{
	MLSignal copy(*this);
	float* pIn = copy.getBuffer();
	
	// left
	mDataAligned[0] = k*pIn[0] + kp*pIn[1];
	
	// center
	for(int i=1; i<mWidth - 1; ++i)
	{
		mDataAligned[i] = km*pIn[i - 1] + k*pIn[i] + kp*pIn[i + 1];
	}
	
	// right
	mDataAligned[mWidth - 1] = km*pIn[mWidth - 2] + k*pIn[mWidth - 1];
}

void MLSignal::convolve5x1(const float kmm, const float km, const float k, const float kp, const float kpp)
{
	MLSignal copy(*this);
	float* pIn = copy.getBuffer();
	
	// left
	mDataAligned[0] = k*pIn[0] + kp*pIn[1] + kpp*pIn[2];
	mDataAligned[1] = km*pIn[0] + k*pIn[1] + kp*pIn[2] + kpp*pIn[3];
	
	// center
	for(int i=2; i<mWidth - 2; ++i)
	{
		mDataAligned[i] = kmm*pIn[i - 2] + km*pIn[i - 1] + k*pIn[i] + kp*pIn[i + 1] + kpp*pIn[i + 2];
	}
	
	// right
	mDataAligned[mWidth - 2] = kmm*pIn[mWidth - 4] + km*pIn[mWidth - 3] + k*pIn[mWidth - 2] + kp*pIn[mWidth - 1];
	mDataAligned[mWidth - 1] = kmm*pIn[mWidth - 4] + km*pIn[mWidth - 3] + k*pIn[mWidth - 2];
}


// an operator for 2D signals only
void MLSignal::convolve3x3r(const float kc, const float ke, const float kk)
{
	int i, j;
	float f;
	float * pr1, * pr2, * pr3; // input row ptrs
	float * prOut; 	
	
	MLSignal copy(*this);
	float* pIn = copy.getBuffer();	
	float* pOut = mDataAligned;
	int width = mWidth;
	int height = mHeight;
	
	j = 0;	// top row
	{
		// row ptrs
		pr2 = (pIn + row(j));
		pr3 = (pIn + row(j + 1));
		prOut = (pOut + row(j));
		
		i = 0; // top left corner
		{
			f = ke * (pr2[i+1] + pr3[i]);
			f += kk * (pr3[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
		
		for(i = 1; i < width - 1; i++) // top side
		{
			f = ke * (pr2[i-1] + pr2[i+1] + pr3[i]);
			f += kk * (pr3[i-1] + pr3[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;
		}
		
		i = width - 1; // top right corner
		{
			f = ke * (pr2[i-1] + pr3[i]);
			f += kk * (pr3[i-1]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
	}
	for(j = 1; j < height - 1; j++) // center rows
	{
		// row ptrs
		pr1 = (pIn + row(j - 1));
		pr2 = (pIn + row(j));
		pr3 = (pIn + row(j + 1));
		prOut = (pOut + row(j));
		
		i = 0; // left side
		{
			f = ke * (pr1[i] + pr2[i+1] + pr3[i]);
			f += kk * (pr1[i+1] + pr3[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
		
		for(i = 1; i < width - 1; i++) // center
		{
			f = ke * (pr2[i-1] + pr1[i] + pr2[i+1] + pr3[i]);
			f += kk * (pr1[i-1] + pr1[i+1] + pr3[i-1] + pr3[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;
		}
		
		i = width - 1; // right side
		{
			f = ke * (pr2[i-1] + pr1[i] + pr3[i]);
			f += kk * (pr1[i-1] + pr3[i-1]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
	}
	j = height - 1;	// bottom row
	{
		// row ptrs
		pr1 = (pIn + row(j - 1));
		pr2 = (pIn + row(j));
		prOut = (pOut + row(j));
		
		i = 0; // bottom left corner
		{
			f = ke * (pr1[i] + pr2[i+1]);
			f += kk * (pr1[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
		
		for(i = 1; i < width - 1; i++) // bottom side
		{
			f = ke * (pr2[i-1] + pr1[i] + pr2[i+1]);
			f += kk * (pr1[i-1] + pr1[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;
		}
		
		i = width - 1; // bottom right corner
		{
			f = ke * (pr2[i-1] + pr1[i]);
			f += kk * (pr1[i-1]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
	}
}


// an operator for 2D signals only
// convolve signal with coefficients, duplicating samples at border. 
void MLSignal::convolve3x3rb(const float kc, const float ke, const float kk)
{
	int i, j;
	float f;
	float * pr1, * pr2, * pr3; // input row ptrs
	float * prOut; 	
	
	MLSignal copy(*this);
	float* pIn = copy.getBuffer();
	
	float* pOut = mDataAligned;
	int width = mWidth;
	int height = mHeight;
	
	j = 0;	// top row
	{
		// row ptrs
		pr2 = (pIn + row(j));
		pr3 = (pIn + row(j + 1));
		prOut = (pOut + row(j));
		
		i = 0; // top left corner
		{
			f = ke * (pr2[i+1] + pr3[i] + pr2[i] + pr2[i]);
			f += kk * (pr3[i+1] + pr2[i+1] + pr3[i] + pr2[i]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
		
		for(i = 1; i < width - 1; i++) // top side
		{
			f = ke * (pr2[i-1] + pr2[i+1] + pr3[i] + pr2[i]);
			f += kk * (pr3[i-1] + pr3[i+1] + pr2[i-1] + pr2[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;
		}
		
		i = width - 1; // top right corner
		{
			f = ke * (pr2[i-1] + pr3[i] + pr2[i] + pr2[i]);
			f += kk * (pr3[i-1] + pr2[i-1] + pr3[i] + pr2[i]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
	}
	for(j = 1; j < height - 1; j++) // center rows
	{
		// row ptrs
		pr1 = (pIn + row(j - 1));
		pr2 = (pIn + row(j));
		pr3 = (pIn + row(j + 1));
		prOut = (pOut + row(j));
		
		i = 0; // left side
		{
			f = ke * (pr1[i] + pr2[i+1] + pr3[i] + pr2[i]);
			f += kk * (pr1[i+1] + pr3[i+1] + pr1[i] + pr3[i]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
		
		for(i = 1; i < width - 1; i++) // center
		{
			f = ke * (pr2[i-1] + pr1[i] + pr2[i+1] + pr3[i]);
			f += kk * (pr1[i-1] + pr1[i+1] + pr3[i-1] + pr3[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;
		}
		
		i = width - 1; // right side
		{
			f = ke * (pr2[i-1] + pr1[i] + pr3[i] + pr2[i]);
			f += kk * (pr1[i-1] + pr3[i-1] + pr1[i] + pr3[i]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
	}
	j = height - 1;	// bottom row
	{
		// row ptrs
		pr1 = (pIn + row(j - 1));
		pr2 = (pIn + row(j));
		prOut = (pOut + row(j));
		
		i = 0; // bottom left corner
		{
			f = ke * (pr1[i] + pr2[i+1] + pr2[i] + pr2[i]);
			f += kk * (pr1[i+1] + pr1[i] + pr2[i+1] + pr2[i]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
		
		for(i = 1; i < width - 1; i++) // bottom side
		{
			f = ke * (pr2[i-1] + pr1[i] + pr2[i+1] + pr2[i]);
			f += kk * (pr1[i-1] + pr1[i+1] + pr2[i-1] + pr2[i+1]);
			f += kc * pr2[i];
			prOut[i] = f;
		}
		
		i = width - 1; // bottom right corner
		{
			f = ke * (pr2[i-1] + pr1[i] + pr2[i] + pr2[i]);
			f += kk * (pr1[i-1] + pr1[i] + pr2[i-1] + pr2[i]);
			f += kc * pr2[i];
			prOut[i] = f;		
		}
	}
}

// TODO SIMD
float MLSignal::getRMS()
{
	float d = 0.f;
	for(int i=0; i<mSize; ++i)
	{
		const float v = (mDataAligned[i]);
		d += v*v;
	}
	return sqrtf(d/mSize);
}

float MLSignal::rmsDiff(const MLSignal& b)
{
	float d = 0.f;
	if(mWidth != b.mWidth) return -1.f;
	if(mHeight != b.mHeight) return -1.f;
	if(mDepth != b.mDepth) return -1.f;
	
	for(int i=0; i<mSize; ++i)
	{
		float v = (mDataAligned[i] - b.mDataAligned[i]);
		d += v*v;
	}
	return sqrtf(d/mSize);
}

void MLSignal::flipVertical()
{
	float* p0 = mDataAligned;
	for(int j=0; j<(mHeight>>1) - 1; ++j)
	{
		float temp;
		float* p1 = p0 + row(j);
		float* p2 = p0 + row(mHeight - 1 - j);
		for(int i=0; i<mWidth; ++i)
		{
			temp = p1[i];
			p1[i] = p2[i];
			p2[i] = temp;
		}
	}
}

void MLSignal::square()
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] *= mDataAligned[i];
	}
}

void MLSignal::sqrt()
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] = sqrtf(mDataAligned[i]);
	}
}

void MLSignal::abs()
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] = fabs(mDataAligned[i]);
	}
}

void MLSignal::inv()
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] = 1.0f / mDataAligned[i];
	}
}

void MLSignal::ssign()
{
	// TODO SSE
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = f < 0.f ? -1.f : 1.f;
	}
}

/*
 void MLSignal::log2Approx()
 {
	float* px1 = getBuffer();
 
	int c = getSize() >> kfloatsPerSIMDVectorBits;
	__m128 vx1, vy1;
	
	for (int n = 0; n < c; ++n)
	{
 vx1 = _mm_load_ps(px1);
 vy1 = log2Approx4(vx1); 		
 _mm_store_ps(px1, vy1);
 px1 += kFloatsPerSIMDVector;
	}
 }*/

void MLSignal::setIdentity()
{
	MLSignal& a = *this;
	clear();
	int n = ml::min(mWidth, mHeight);
	for(int i = 0; i < n; ++i)
	{
		a(i, i) = 1;
	}
}

// make a boundary useful for DSP and other operations by writing the 
// edge values with duplicates of the neighboring values.
void MLSignal::makeDuplicateBoundary2D()
{
	MLSignal& a = *this;
	
	// top and bottom
	for(int i=1; i<mWidth - 1; ++i)
	{
		a(i, 0) = a(i, 1);
		a(i, mHeight - 1) = a(i, mHeight - 2);
	}
	
	// left and right
	for(int j=0; j<mHeight; ++j)
	{
		a(0, j) = a(1, j);
		a(mWidth - 1, j) = a(mWidth - 2, j);
	}
}

/*
 
 // return integer coordinates (with float z) of peak value in a 2D signal.
 //
 Vec3 MLSignal::findPeak() const
 {
	int maxX = -1;
	int maxY = -1;
	float z = -1.f;
	float maxZ = -MAXFLOAT;
	
	for (int j=0; j < mHeight; ++j)
	{
 for (int i=0; i < mWidth; i++)
 {
 z = operator()(i, j);
 if(z > maxZ)
 {
 maxZ = z;
 maxX = i;
 maxY = j;
 }
 }
	}	
	return Vec3(maxX, maxY, maxZ);
 }
 */

int MLSignal::checkForNaN() const
{
	int ret = false;
	const float* p = mDataAligned;
	for(int i=0; i<mSize; ++i)
	{
		const float k = p[i];
		if (k != k)
		{
			ret = true;
			break;
		}
	}
	return ret;
}

float MLSignal::getSum() const
{
	float sum = 0.f;
	for(int i=0; i<mSize; ++i)
	{
		sum += mDataAligned[i];
	}
	return sum;
}

float MLSignal::getMean() const
{
	return getSum() / (float)mSize;
}

float MLSignal::getMin() const
{
	float fMin = FLT_MAX;
	for(int i=0; i<mSize; ++i)
	{
		float x = mDataAligned[i];
		if(x < fMin)
		{
			fMin = x;
		}
	}
	return fMin;
}

// TODO SIMD
float MLSignal::getMax() const
{
	float fMax = -FLT_MAX;
	for(int i=0; i<mSize; ++i)
	{
		float x = mDataAligned[i];
		if(x > fMax)
		{
			fMax = x;
		}
	}
	return fMax;
}

void MLSignal::dump(std::ostream& s, int verbosity) const
{
	s << "signal @ " << std::hex << this << std::dec << 
	" " << mWidth << "x" << mHeight << "x" << mDepth << " [" << mWidth*mHeight*mDepth << " frames] : sum " << getSum() << "\n";
	
	int w = mWidth;
	int h = mHeight;
	const MLSignal& f = *this;
	if(verbosity > 0)
	{
		if(is2D())
		{
			s << std::setprecision(4);
			for (int j=0; j<h; ++j)
			{
				s << j << " | ";
				for(int i=0; i<w; ++i)
				{
					s << f(i, j) << " ";
				}
				s << "\n";
			}
		}
		else
		{
			s << std::setprecision(5);
			for (int i=0; i<w; ++i)
			{
				if(verbosity > 1)
				{
					s << "[" << i << "]";
				}
				s << mDataAligned[i] << " ";
			}
			s << "\n";
		}
	}
}

/*
 void MLSignal::dump(std::ostream& s, const MLRect& b) const
 {
	const MLSignal& f = *this;
	{
 s << std::fixed << std::setprecision(3);
 for (int j=b.top(); j< b.bottom(); ++j)
 {
 s << j << " | ";
 for(int i=b.left(); i< b.right(); ++i)
 {
 s << f(i, j) << " ";
 }
 s << "\n";
 }
	}
 }
 
 */
void MLSignal::dumpASCII(std::ostream& s) const
{
	const char* g = " .:;+=xX$&";
	int w = mWidth;
	int h = mHeight;
	const MLSignal& f = *this;
	const int scale = (int)strlen(g);
	for (int j=0; j<h; ++j)
	{
		s << "|";
		for(int i=0; i<w; ++i)
		{
			int v = (f(i,j)*scale);
			s << g[ml::clamp(v, 0, scale - 1)];
		}
		s << "|\n";
	}
}

/*
 Vec2 MLSignal::correctPeak(const int ix, const int iy, const float maxCorrect) const
 {		
	const MLSignal& in = *this;
	int width = in.getWidth();
	int height = in.getHeight();
	int x = ml::clamp(ix, 1, width - 2);
	int y = ml::clamp(iy, 1, height - 2);
	Vec2 pos(x, y);
	
	// Use centered differences to find derivatives.
	float dx = (in(x + 1, y) - in(x - 1, y)) / 2.f;
	float dy = (in(x, y + 1) - in(x, y - 1)) / 2.f;
	float dxx = (in(x + 1, y) + in(x - 1, y) - 2*in(x, y));
	float dyy = (in(x, y + 1) + in(x, y - 1) - 2*in(x, y));
	float dxy = (in(x + 1, y + 1) + in(x - 1, y - 1) - in(x + 1, y - 1) - in(x - 1, y + 1)) / 4.f;
	
	if((dxx != 0.f)&&(dxx != 0.f)&&(dxx != 0.f))
	{
 float oneOverDiscriminant = 1.f/(dxx*dyy - dxy*dxy);
 float fx = (dyy*dx - dxy*dy) * oneOverDiscriminant;
 float fy = (dxx*dy - dxy*dx) * oneOverDiscriminant;
 
 // here is the code to return the z approximaton if needed. 
	//	float fz = dx*fx + dy*fy + 0.5f*(dxx*fx*fx + 2.f*dxy*fx*fy + dyy*fy*fy);
	//	float z = in(x, y) + fz;
 
 fx = ml::clamp(fx, -maxCorrect, maxCorrect);
 fy = ml::clamp(fy, -maxCorrect, maxCorrect);
 
 pos -= Vec2(fx, fy);
	}
	return pos;
 }
 
 */

// a simple pixel-by-pixel measure of the distance between two signals.
//
float rmsDifference2D(const MLSignal& a, const MLSignal& b)
{
	int w = ml::min(a.getWidth(), b.getWidth());
	int h = ml::min(a.getHeight(), b.getHeight());
	float sum = 0.;
	float d;
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			d = a(i, j) - b(i, j);
			sum += d*d;
		}
	}
	sum /= w*h;
	sum = sqrtf(sum);
	return sum;
}


// centered partial derivative of 2D signal in x
//
void MLSignal::partialDiffX()
{
	int i, j;
	float * pr; // input row ptr
	float * prOut; 	
	
	MLSignal copy(*this);
	float* pIn = copy.getBuffer();
	
	float* pOut = mDataAligned;
	int width = mWidth;
	int height = mHeight;
	
	for(j = 0; j < height; ++j) 
	{
		// row ptrs
		pr = (pIn + row(j));
		prOut = (pOut + row(j));
		
		i = 0; // left side
		{
			prOut[i] = (pr[i+1]) / 2.f;		
		}
		
		for(i = 1; i < width - 1; ++i) 
		{
			prOut[i] = (pr[i+1] - pr[i-1]) / 2.f;
		}
		
		i = width - 1; // right side
		{
			prOut[i] = (-pr[i-1]) / 2.f;		
		}
	}
}

// centered partial derivative of 2D signal in y
//
void MLSignal::partialDiffY()
{
	int i, j;
	float * pr1, * pr2, * pr3; // input row ptrs
	float * prOut; 	
	
	MLSignal copy(*this);
	float* pIn = copy.getBuffer();
	
	float* pOut = mDataAligned;
	int width = mWidth;
	int height = mHeight;
	
	j = 0; // top row
	{
		pr2 = (pIn + row(j));
		pr3 = (pIn + row(j + 1));
		prOut = (pOut + row(j));
		
		for(i = 0; i < width; ++i) 
		{
			prOut[i] = (pr3[i]) / 2.f;
		}
	}
	
	for(j = 1; j < height - 1; ++j) // center rows
	{
		pr1 = (pIn + row(j - 1));
		pr2 = (pIn + row(j));
		pr3 = (pIn + row(j + 1));
		prOut = (pOut + row(j));
		
		for(i = 0; i < width; ++i) 
		{
			prOut[i] = (pr3[i] - pr1[i]) / 2.f;
		}
	}
	
	j = height - 1;	// bottom row
	{
		pr1 = (pIn + row(j - 1));
		pr2 = (pIn + row(j));
		prOut = (pOut + row(j));
		
		for(i = 0; i < width; ++i) 
		{
			prOut[i] = (-pr1[i]) / 2.f;
		}
	}
}

std::ostream& operator<< (std::ostream& out, const MLSignal & s)
{
	s.dump(out);
	return out;
}

// helper functions
MLSignal MLSignal::copyWithLoopAtEnd(const MLSignal& src, int loopLength)
{
	return MLSignal(src, kLoopType1DEnd, loopLength);
}

