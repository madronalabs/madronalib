
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// various operations on signals
// TODO organize

#include "MLSignal.h"

#ifdef DEBUG
const MLSample kMLSignalEndSamples[4] = 
{
	(MLSample)0x01234567, (MLSample)0x89abcdef, (MLSample)0xfedcba98, (MLSample)0x76543210 
};
#endif

// ----------------------------------------------------------------
#pragma mark MLSignal

// no length argument: make a null object.
MLSignal::MLSignal() : 
	mData(0),
	mDataAligned(0),
	mCopy(0),
	mCopyAligned(0)
{
	mRate = kMLToBeCalculated;
	setDims(0);
}

MLSignal::MLSignal (int width, int height, int depth) : 
	mData(0),
	mDataAligned(0),
	mCopy(0),
	mCopyAligned(0)
{
	mRate = kMLToBeCalculated;
	setDims(width, height, depth);
}

MLSignal::MLSignal(const MLSignal& other) :
	mData(0),
	mDataAligned(0),
	mCopy(0),
	mCopyAligned(0)
{
	mSize = other.mSize;
	mData = allocateData(mSize);
	mDataAligned = initializeData(mData, mSize);
	mConstantMask = other.mConstantMask;
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
mData(0),
mDataAligned(0),
mCopy(0),
mCopyAligned(0)
{
	mRate = kMLToBeCalculated;
	setDims((int)values.size());
	int idx = 0;
	for(float f : values)
	{
		mDataAligned[idx++] = f;
	}
}

// constructor for making loops. only one type for now. we could loop in different directions and dimensions.
MLSignal::MLSignal(MLSignal other, eLoopType loopType, int loopSize) :
mData(0),
mDataAligned(0),
mCopy(0),
mCopyAligned(0)
{
	switch(loopType)
	{
		case kLoopType1DEnd:
		default:
		{
			int w = other.getWidth();
			int loopWidth = clamp(loopSize, 0, w);
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
			// 1: allocate new memory and copy the elements
			mSize = other.mSize;
			MLSample * newData = allocateData(mSize);
			MLSample * newDataAligned = initializeData(newData, mSize);
			std::copy(other.mDataAligned, other.mDataAligned + mSize, newDataAligned);

			// 2: deallocate old memory
			delete[] mData;
			
			// 3: assign the new memory to the object
			mData = newData;
			mDataAligned = newDataAligned;
			mConstantMask = other.mConstantMask;
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
			mConstantMask = other.mConstantMask;
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


// private signal constructor: make a reference to a slice of the external signal.
// of course this object will be meaningless when the other Signal is gone, so
// use wih care -- only as a temporary, ideally.  Is there a way to force 
// the object to be a temporary?  In other words not allow a named 
// MLSignal notTemporary(pOther, 4);
//
// NOTE this signal will not pass checkIntegrity()!
//
MLSignal::MLSignal(const MLSignal* other, int slice) : 
	mData(0),
	mDataAligned(0),
	mCopy(0),
	mCopyAligned(0)
{
	mRate = kMLToBeCalculated;
	setConstant(false);
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
	mWidthBits = bitsToContain(mWidth);
	mHeightBits = bitsToContain(mHeight);
	mDepthBits = bitsToContain(mDepth);
	mSize = 1 << mWidthBits << mHeightBits << mDepthBits;
	mConstantMask = mSize - 1;
}

MLSignal::~MLSignal() 
{
	delete[] mData;
	delete[] mCopy;
}

MLSample* MLSignal::setDims (int width, int height, int depth)
{
	mDataAligned = 0;	
	// delete old
	if (mData)
	{
		delete[] mData;
	}

	mWidth = width;
	mHeight = height;
	mDepth = depth;
	mWidthBits = bitsToContain(width);
	mHeightBits = bitsToContain(height);
	mDepthBits = bitsToContain(depth);
	mSize = 1 << mWidthBits << mHeightBits << mDepthBits;
	mData = allocateData(mSize);	
	mDataAligned = initializeData(mData, mSize);	
	mConstantMask = mSize - 1;
	return mDataAligned;
}

// make the copy buffer if needed. 
// then copy the current data to the copy buffer and return the start of the copy.
//
MLSample* MLSignal::getCopy()
{
	if (!mCopy)
	{
		mCopy = allocateData(mSize);
		if (mCopy)
		{
			mCopyAligned = initializeData(mCopy, mSize);
		}
		else
		{
			std::cerr << "MLSignal::getCopy: out of memory!\n";
		}
	}
	std::copy(mDataAligned, mDataAligned + mSize, mCopyAligned);
	return mCopyAligned;
}

// allocate unaligned data
// TODO test cache-friendly distributions
//
MLSample* MLSignal::allocateData(int size)
{
	MLSample* newData = 0;
	newData = new MLSample[padSize(size)];
	return newData;
}

MLSample* MLSignal::initializeData(MLSample* pData, int size)
{
	MLSample* newDataAligned = 0;
	if(pData)
	{
		newDataAligned = alignToCacheLine(pData); 
		memset((void *)(newDataAligned), 0, (size_t)(size*sizeof(MLSample)));
#ifdef DEBUG
		std::copy(kMLSignalEndSamples, kMLSignalEndSamples + kMLSignalEndSize, newDataAligned + size);
#endif
	}
	return newDataAligned;
}

int MLSignal::getFrames() const
{ 		
	if (mRate != kMLTimeless)
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
const MLSample MLSignal::operator()(const float fi, const float fj) const
{
	MLSample a, b, c, d;
	
	int i = (int)(fi);
	int j = (int)(fj);
	
	// get truncate down for inputs < 0
	// TODO use vectors with _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
	// _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);	
	if (fi < 0) i--;
	if (fj < 0) j--;
	float ri = fi - i;
	float rj = fj - j;
	
	int i1ok = within(i, 0, mWidth);
	int i2ok = within(i + 1, 0, mWidth);
	int j1ok = within(j, 0, mHeight);
	int j2ok = within(j + 1, 0, mHeight);
	
	a = (j1ok && i1ok) ? mDataAligned[row(j) + i] : 0.f;
	b = (j1ok && i2ok) ? mDataAligned[row(j) + i + 1] : 0.f;
	c = (j2ok && i1ok) ? mDataAligned[row(j + 1) + i] : 0.f;
	d = (j2ok && i2ok) ? mDataAligned[row(j + 1) + i + 1] : 0.f;
	
	return lerp(lerp(a, b, ri), lerp(c, d, ri), rj);
}*/

/*
// TODO SSE
const MLSample MLSignal::operator()(const Vec2& pos) const
{
	return operator()(pos.x(), pos.y());
}
*/
/*
// TODO unimplemented
const MLSample MLSignal::operator() (const float , const float , const float ) const
{
	return 0.;
}

const MLSample MLSignal::operator() (const Vec3 ) const
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
	
	MLSample* pDestFrame = mDataAligned + plane(i);
	const MLSample* pSrc = src.getConstBuffer();
	std::copy(pSrc, pSrc + src.getSize(), pDestFrame);
}

//
#pragma mark I/O
// 

// read n samples from an external sample pointer plus a sample offset into start of signal.
//
void MLSignal::read(const MLSample *input, const int offset, const int n)
{
	setConstant(false);
	std::copy(input + offset, input + offset + n, mDataAligned);
}

// write n samples from start of signal to an external sample pointer plus a sample offset.
//
void MLSignal::write(MLSample *output, const int offset, const int n)
{
	std::copy(mDataAligned, mDataAligned + n, output + offset);
}

// TODO SSE
void MLSignal::sigClamp(const MLSignal& a, const MLSignal& b)
{
	int n = min(mSize, a.getSize());
	n = min(n, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		MLSample f = mDataAligned[i];
		mDataAligned[i] = clamp(f, a.mDataAligned[i], b.mDataAligned[i]);
	}
	setConstant(false);
}

void MLSignal::sigMin(const MLSignal& b)
{
	int n = min(mSize, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		MLSample f = mDataAligned[i];
		mDataAligned[i] = min(f, b.mDataAligned[i]);
	}
	setConstant(false);
}

void MLSignal::sigMax(const MLSignal& b)
{
	int n = min(mSize, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		MLSample f = mDataAligned[i];
		mDataAligned[i] = max(f, b.mDataAligned[i]);
	}
	setConstant(false);
}

// TODO SSE
void MLSignal::sigLerp(const MLSignal& b, const MLSample mix)
{
	int n = min(mSize, b.getSize());
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] = lerp(mDataAligned[i], b.mDataAligned[i], mix);
	}
	setConstant(false);
}

// TODO SSE
void MLSignal::sigLerp(const MLSignal& b, const MLSignal& mix)
{
	int n = min(mSize, b.getSize());
	n = min(n, mix.getSize());
	for(int i = 0; i < n; ++i)
	{
		mDataAligned[i] = lerp(mDataAligned[i], b.mDataAligned[i], mix.mDataAligned[i]);
	}
	setConstant(false);
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
	const bool kb = b.isConstant();
	if (kb)
	{
		setToConstant(b.mDataAligned[0]);
	}
	else 
	{
		const int n = min(mSize, b.getSize());
		std::copy(b.mDataAligned, b.mDataAligned + n, mDataAligned);
		setConstant(false);
	}
}

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

	setConstant(false);
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

	setConstant(false);
}

/*
// TEMP
const MLSample MLSignal::operator() (const float i, const float j) const
{
    return getInterpolatedLinear(i, j);

}*/


// TODO SSE
void MLSignal::add(const MLSignal& b)
{
	const bool ka = isConstant();
	const bool kb = b.isConstant();
	if (ka && kb)
	{
		setToConstant(mDataAligned[0] + b.mDataAligned[0]);
	}
	else 
	{
		const int n = min(mSize, b.getSize());
		if (ka && !kb)
		{
			MLSample fa = mDataAligned[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] = fa + b[i];
			}
		}
		else if (!ka && kb)
		{
			MLSample fb = b[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] += fb;
			}
		}
		else
		{
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] += b.mDataAligned[i];
			}
		}
		setConstant(false);
	}
}

// TODO SSE
void MLSignal::subtract(const MLSignal& b)
{
	const bool ka = isConstant();
	const bool kb = b.isConstant();
	if (ka && kb)
	{
		setToConstant(mDataAligned[0] + b.mDataAligned[0]);
	}
	else 
	{
		const int n = min(mSize, b.getSize());
		if (ka && !kb)
		{
			MLSample fa = mDataAligned[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] = fa - b[i];
			}
		}
		else if (!ka && kb)
		{
			MLSample fb = b[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] -= fb;
			}
		}
		else
		{
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] -= b.mDataAligned[i];
			}
		}
		setConstant(false);
	}
}


// TODO SSE
void MLSignal::multiply(const MLSignal& b)
{
	const bool ka = isConstant();
	const bool kb = b.isConstant();
	if (ka && kb)
	{
		setToConstant(mDataAligned[0] + b.mDataAligned[0]);
	}
	else 
	{
		const int n = min(mSize, b.getSize());
		if (ka && !kb)
		{
			MLSample fa = mDataAligned[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] = fa * b[i];
			}
		}
		else if (!ka && kb)
		{
			MLSample fb = b[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] *= fb;
			}
		}
		else
		{
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] *= b.mDataAligned[i];
			}
		}
		setConstant(false);
	}
}

// TODO SSE
void MLSignal::divide(const MLSignal& b)
{
	const bool ka = isConstant();
	const bool kb = b.isConstant();
	if (ka && kb)
	{
		setToConstant(mDataAligned[0] + b.mDataAligned[0]);
	}
	else 
	{
		const int n = min(mSize, b.getSize());
		if (ka && !kb)
		{
			MLSample fa = mDataAligned[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] = fa / b[i];
			}
		}
		else if (!ka && kb)
		{
			MLSample fb = b[0];
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] /= fb;
			}
		}
		else
		{
			for(int i = 0; i < n; ++i)
			{
				mDataAligned[i] /= b.mDataAligned[i];
			}
		}
		setConstant(false);
	}
}


//
#pragma mark unary ops
// 

void MLSignal::clear()
{
//	std::fill(mDataAligned, mDataAligned+mSize, 0);
//	setToConstant(0); // TODO 
	memset((void *)(mDataAligned), 0, (size_t)(mSize*sizeof(MLSample)));
}


void MLSignal::fill(const MLSample f)
{
	std::fill(mDataAligned, mDataAligned+mSize, f);
}

void MLSignal::scale(const MLSample k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] *= k;
	}
}

void MLSignal::add(const MLSample k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] += k;
	}
}

void MLSignal::subtract(const MLSample k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] -= k;
	}
}

void MLSignal::subtractFrom(const MLSample k)
{
	for(int i=0; i<mSize; ++i)
	{
		mDataAligned[i] = k - mDataAligned[i];
	}
}

// name collision with clamp template made this sigClamp
void MLSignal::sigClamp(const MLSample min, const MLSample max)	
{
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = clamp(f, (float)min, (float)max);
	}
}

// TODO SSE
void MLSignal::sigMin(const MLSample m)
{
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = min(f, (float)m);
	}
}

// TODO SSE
void MLSignal::sigMax(const MLSample m)	
{
	for(int i=0; i<mSize; ++i)
	{
		float f = mDataAligned[i];
		mDataAligned[i] = max(f, (float)m);
	}
}

// convolve a 1D signal with a 3-point impulse response.
void MLSignal::convolve3x1(const MLSample km, const MLSample k, const MLSample kp)
{
    // TODO SSE
	int width = mWidth;
	MLSample* pIn = getCopy();
    
    // left
    mDataAligned[0] = k*pIn[0] + kp*pIn[1];
    
    // center
    for(int i=1; i<width - 1; ++i)
    {
        mDataAligned[i] = km*pIn[i - 1] + k*pIn[i] + kp*pIn[i + 1];
    }
    
    // right
    mDataAligned[width - 1] = km*pIn[width - 2] + k*pIn[width - 1];
}

void MLSignal::convolve5x1(const MLSample kmm, const MLSample km, const MLSample k, const MLSample kp, const MLSample kpp)
{
    // TODO SSE
	int width = mWidth;
	MLSample* pIn = getCopy();
    
    // left
    mDataAligned[0] = k*pIn[0] + kp*pIn[1] + kpp*pIn[2];
    mDataAligned[1] = km*pIn[0] + k*pIn[1] + kp*pIn[2] + kpp*pIn[3];
    
    // center
    for(int i=2; i<width - 2; ++i)
    {
        mDataAligned[i] = kmm*pIn[i - 2] + km*pIn[i - 1] + k*pIn[i] + kp*pIn[i + 1] + kpp*pIn[i + 2];
    }
    
    // right
    mDataAligned[width - 2] = kmm*pIn[width - 4] + km*pIn[width - 3] + k*pIn[width - 2] + kp*pIn[width - 1];
    mDataAligned[width - 1] = kmm*pIn[width - 4] + km*pIn[width - 3] + k*pIn[width - 2];
}


// an operator for 2D signals only
void MLSignal::convolve3x3r(const MLSample kc, const MLSample ke, const MLSample kk)
{
	int i, j;
    float f;
    float * pr1, * pr2, * pr3; // input row ptrs
    float * prOut; 	
	
	MLSample* pIn = getCopy();
	MLSample* pOut = mDataAligned;
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
void MLSignal::convolve3x3rb(const MLSample kc, const MLSample ke, const MLSample kk)
{
	int i, j;
	float f;
	float * pr1, * pr2, * pr3; // input row ptrs
	float * prOut; 	
	
	MLSample* pIn = getCopy();
	MLSample* pOut = mDataAligned;
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

// an operator for 2D signals only
void MLSignal::variance3x3()
{
	MLSample* pIn = getCopy();
	MLSample* pOut = mDataAligned;
	int i, j;
	float f;
	float * pr1, * pr2, * pr3; // input row ptrs
	float * prOut; 		
	int width = mWidth;
	int height = mHeight;
	float c, x1, x2, x3, x4, x5, x6, x7, x8;
	
	i = 0;	// top row
	{
		// row ptrs
		pr2 = (pIn + row(i));
		pr3 = (pIn + row(i + 1));
		prOut = (pOut + row(i));
		
		j = 0; // top left corner
		{
			c = pr2[j]; x5 = pr2[j+1];
			x7 = pr3[j]; x8 = pr3[j+1];
			
			x5 -= c;
			x7 -= c; x8 -= c;
			
			x5 *= x5;
			x7 *= x7; x8 *= x8;
			
			f = x5 + x7 + x8;
			f /= 3.f;
			prOut[j] = sqrtf(f);
		}
		for(j = 1; j < width - 1; j++) // top side
		{
			x4 = pr2[j-1]; c = pr2[j]; x5 = pr2[j+1];
			x6 = pr3[j-1]; x7 = pr3[j]; x8 = pr3[j+1];
			
			x4 -= c; x5 -= c;
			x6 -= c; x7 -= c; x8 -= c;
			
			x4 *= x4; x5 *= x5;
			x6 *= x6; x7 *= x7; x8 *= x8;
			
			f = x4 + x5 + x6 + x7 + x8;
			f /= 5.f;
			prOut[j] = sqrtf(f);
		}		
		j = width - 1; // top right corner
		{
			x4 = pr2[j-1]; c = pr2[j];
			x6 = pr3[j-1]; x7 = pr3[j];
			
			x4 -= c;
			x6 -= c; x7 -= c; 
			
			x4 *= x4; 
			x6 *= x6; x7 *= x7; 
			
			f = x4 + x6 + x7;
			f /= 3.f;
			prOut[j] = sqrtf(f);
		}
	}
	for(i = 1; i < height - 1; i++) // center rows
	{
		// row ptrs
		pr1 = (pIn + row(i - 1));
		pr2 = (pIn + row(i));
		pr3 = (pIn + row(i + 1));
		prOut = (pOut + row(i));
		
		j = 0; // left side
		{
			x2 = pr1[j]; x3 = pr1[j+1];
			c = pr2[j]; x5 = pr2[j+1];
			x7 = pr3[j]; x8 = pr3[j+1];
			
			x2 -= c; x3 -= c;
			x5 -= c;
			x7 -= c; x8 -= c;
			
			x2 *= x2; x3 *= x3;
			x5 *= x5;
			x7 *= x7; x8 *= x8;
			
			f = x2 + x3 + x5 + x7 + x8;
			f /= 5.f;
			prOut[j] = sqrtf(f);
		}
		for(j = 1; j < width - 1; j++) // center
		{
			x1 = pr1[j-1]; x2 = pr1[j]; x3 = pr1[j+1];
			x4 = pr2[j-1]; c = pr2[j]; x5 = pr2[j+1];
			x6 = pr3[j-1]; x7 = pr3[j]; x8 = pr3[j+1];
			
			x1 -= c; x2 -= c; x3 -= c;
			x4 -= c; x5 -= c;
			x6 -= c; x7 -= c; x8 -= c;
			
			x1 *= x1; x2 *= x2; x3 *= x3;
			x4 *= x4; x5 *= x5;
			x6 *= x6; x7 *= x7; x8 *= x8;
			
			f = x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8;
			f /= 8.f;
			prOut[j] = sqrtf(f);
		}
		
		j = width - 1; // right side
		{
			x1 = pr1[j-1]; x2 = pr1[j]; 
			x4 = pr2[j-1]; c = pr2[j]; 
			x6 = pr3[j-1]; x7 = pr3[j]; 
			
			x1 -= c; x2 -= c;
			x4 -= c; 
			x6 -= c; x7 -= c; 
			
			x1 *= x1; x2 *= x2; 
			x4 *= x4; 
			x6 *= x6; x7 *= x7;
			
			f = x1 + x2 + x4 + x6 + x7;
			f /= 5.f;
			prOut[j] = sqrtf(f);
		}
	}
	i = height - 1;	// bottom row
	{
		// row ptrs
		pr1 = (pIn + row(i - 1));
		pr2 = (pIn + row(i));
		prOut = (pOut + row(i));
		
		j = 0; // bottom left corner
		{
			x2 = pr1[j]; x3 = pr1[j+1];
			c = pr2[j]; x5 = pr2[j+1];
			
			x2 -= c; x3 -= c;
			x5 -= c;
			
			x2 *= x2; x3 *= x3;
			x5 *= x5;
			
			f = x2 + x3 + x5 ;
			f /= 3.f;
			prOut[j] = sqrtf(f);
		}
			
		for(j = 1; j < width - 1; j++) // bottom side
		{
			x1 = pr1[j-1]; x2 = pr1[j]; x3 = pr1[j+1];
			x4 = pr2[j-1]; c = pr2[j]; x5 = pr2[j+1];
			
			x1 -= c; x2 -= c; x3 -= c;
			x4 -= c; x5 -= c;
			
			x1 *= x1; x2 *= x2; x3 *= x3;
			x4 *= x4; x5 *= x5;
			
			f = x1 + x2 + x3 + x4 + x5 ;
			f /= 5.f;
			prOut[j] = sqrtf(f);
		}
		
		j = width - 1; // bottom right corner
		{
			x1 = pr1[j-1]; x2 = pr1[j]; 
			x4 = pr2[j-1]; c = pr2[j]; 
			
			x1 -= c; x2 -= c;
			x4 -= c;
			
			x1 *= x1; x2 *= x2; 
			x4 *= x4; 
						
			f = x1 + x2 + x4 ;
			f /= 3.f;
			prOut[j] = sqrtf(f);
		}
	}
}

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
	MLSample* p0 = mDataAligned;
	for(int j=0; j<(mHeight>>1) - 1; ++j)
	{
		MLSample temp;
		MLSample* p1 = p0 + row(j);
		MLSample* p2 = p0 + row(mHeight - 1 - j);
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
		MLSample f = mDataAligned[i];
		mDataAligned[i] = f < 0.f ? -1.f : 1.f;
	}
}

void MLSignal::log2Approx()
{
	MLSample* px1 = getBuffer();
    
	int c = getSize() >> kMLSamplesPerSSEVectorBits;
	__m128 vx1, vy1;
	
	for (int n = 0; n < c; ++n)
	{
		vx1 = _mm_load_ps(px1);
		vy1 = log2Approx4(vx1); 		
		_mm_store_ps(px1, vy1);
		px1 += kSSEVecSize;
	}
}

void MLSignal::setIdentity()
{
	MLSignal& a = *this;
    clear();
    int n = min(mWidth, mHeight);
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

int MLSignal::checkIntegrity() const
{
	int ret = true;
#ifdef DEBUG
	const MLSample* p = mDataAligned + mSize;
	for(int i=0; i<kMLSignalEndSize; ++i)
	{
		if (p[i] != kMLSignalEndSamples[i])
		{
			ret = false;
			break;
		}
	}
#endif 
	return ret;
}


int MLSignal::checkForNaN() const
{
	int ret = false;
	const MLSample* p = mDataAligned;
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
	MLSample sum = 0.f;
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
	MLSample fMin = kMLMaxSample;
	for(int i=0; i<mSize; ++i)
	{
		MLSample x = mDataAligned[i];
		if(x < fMin)
		{
			fMin = x;
		}
	}
	return fMin;
}

float MLSignal::getMax() const
{
	MLSample fMax = kMLMinSample;
	for(int i=0; i<mSize; ++i)
	{
		MLSample x = mDataAligned[i];
		if(x > fMax)
		{
			fMax = x;
		}
	}
	return fMax;
}

void MLSignal::dump(std::ostream& s, int verbosity) const
{
	s << "signal @ " << std::hex << this << std::dec << " [" << mWidth*mHeight*mDepth << " frames] : sum " << getSum() << "\n";
	
	int w = mWidth;
	int h = mHeight;
	const MLSignal& f = *this;
	if(verbosity > 0)
	{
		if(isConstant())
		{
			s << "constant " << mDataAligned[0] << "\n";
		}
		else if(is2D())
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
			s << g[clamp(v, 0, scale - 1)];
		}
		s << "|\n";
	}
}

Vec2 MLSignal::correctPeak(const int ix, const int iy, const float maxCorrect) const
{		
	const MLSignal& in = *this;
	int width = in.getWidth();
	int height = in.getHeight();
	int x = clamp(ix, 1, width - 2);
	int y = clamp(iy, 1, height - 2);
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
		
		fx = clamp(fx, -maxCorrect, maxCorrect);
		fy = clamp(fy, -maxCorrect, maxCorrect);
			
		pos -= Vec2(fx, fy);
	}
	return pos;
}


// a simple pixel-by-pixel measure of the distance between two signals.
//
float rmsDifference2D(const MLSignal& a, const MLSignal& b)
{
	int w = min(a.getWidth(), b.getWidth());
	int h = min(a.getHeight(), b.getHeight());
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
	
	MLSample* pIn = getCopy();
	MLSample* pOut = mDataAligned;
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
	
	MLSample* pIn = getCopy();
	MLSample* pOut = mDataAligned;
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
