//
//  MLFFT.cpp
//  Soundplane
//
//  Created by Randy Jones on 3/14/15.
//
//

#include "MLFFT.h"

// just placeholder code,
// originally from Paul Bourke. No optimizations so far, even the no-brainers.

/*-------------------------------------------------------------------------
 Calculate the closest but lower power of two of a number
 twopm = 2**m <= n
 Return TRUE if 2**m == n
 */

int Powerof2(int n, int *m, int *twopm)
{
	if (n <= 1) {
		*m = 0;
		*twopm = 1;
		return(false);
	}
	
	*m = 1;
	*twopm = 2;
	do {
		(*m)++;
		(*twopm) *= 2;
	} while (2*(*twopm) <= n);
	
	if (*twopm != n)
		return(false);
	else
		return(true);
}


/*-------------------------------------------------------------------------
 This computes an in-place complex-to-complex FFT
 x and y are the real and imaginary arrays of 2^m points.
 dir =  1 gives forward transform
 dir = -1 gives reverse transform
 
 Formula: forward
 N-1
 ---
 1   \          - j k 2 pi n / N
 X(n) = ---   >   x(k) e                    = forward transform
 N   /                                n=0..N-1
 ---
 k=0
 
 Formula: reverse
 N-1
 ---
 \          j k 2 pi n / N
 X(n) =       >   x(k) e                    = forward transform
 /                                n=0..N-1
 ---
 k=0
 */

int FFT(int dir, int m, float *x, float *y)
{
	long nn,i,i1,j,k,i2,l,l1,l2;
	double c1,c2,tx,ty,t1,t2,u1,u2,z;
	
	/* Calculate the number of points */
	nn = 1;
	for (i=0;i<m;i++)
		nn *= 2;
	
	/* Do the bit reversal */
	i2 = nn >> 1;
	j = 0;
	for (i=0;i<nn-1;i++) 
	{
		if (i < j) 
		{
			tx = x[i];
			ty = y[i];
			x[i] = x[j];
			y[i] = y[j];
			x[j] = tx;
			y[j] = ty;
		}
		k = i2;
		while (k <= j) 
		{
			j -= k;
			k >>= 1;
		}
		j += k;
	}
	
	/* Compute the FFT */
	c1 = -1.0;
	c2 = 0.0;
	l2 = 1;
	for (l=0;l<m;l++) 
	{
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;
		for (j=0;j<l1;j++) 
		{
			for (i=j;i<nn;i+=l2) 
			{
				i1 = i + l1;
				t1 = u1 * x[i1] - u2 * y[i1];
				t2 = u1 * y[i1] + u2 * x[i1];
				x[i1] = x[i] - t1;
				y[i1] = y[i] - t2;
				x[i] += t1;
				y[i] += t2;
			}
			z =  u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = sqrt((1.0 - c1) / 2.0);
		if (dir == 1)
			c2 = -c2;
		c1 = sqrt((1.0 + c1) / 2.0);
	}
	
	/* Scaling for forward transform */
	if (dir == 1) 
	{
		for (i=0;i<nn;i++) 
		{
			x[i] /= (float)nn;
			y[i] /= (float)nn;
		}
	}
	
	return(true);
}


/*-------------------------------------------------------------------------
 Perform a 2D FFT inplace given a complex 2D array
 The direction dir, 1 for forward, -1 for reverse
 The size of the array (nx,ny)
 Return false if there are memory problems or
 the dimensions are not powers of 2
 */


int FFT2D(MLSignal& cReal, MLSignal& cImag, int nx, int ny, int dir)
{
	int i,j;
	int m,twopm;
	float *real, *imag;
	
	/* Transform the rows */
	real = new float[nx];
	imag = new float[nx];
	
	if (real == NULL || imag == NULL)
		return(false);
	if (!Powerof2(nx,&m,&twopm) || twopm != nx)
		return(false);
	for (j=0;j<ny;j++) {
		for (i=0;i<nx;i++) {
			real[i] = cReal(i, j);
			imag[i] = cImag(i, j);
		}
		FFT(dir,m,real,imag);
		for (i=0;i<nx;i++) {
			cReal(i, j) = real[i];
			cImag(i, j) = imag[i];
		}
	}
	delete[] real;
	delete[] imag;
	
	/* Transform the columns */
	real = new float[ny];
	imag = new float[ny];
	
	if (real == NULL || imag == NULL)
		return(false);
	if (!Powerof2(ny,&m,&twopm) || twopm != ny)
		return(false);
	for (i=0;i<nx;i++) 
	{
		for (j=0;j<ny;j++) 
		{
			real[j] = cReal(i, j);
			imag[j] = cImag(i, j);
		}
		
		FFT(dir,m,real,imag);
		for (j=0;j<ny;j++) 
		{
			cReal(i, j) = real[j];
			cImag(i, j) = imag[j];
		}
	}
	delete[] real;
	delete[] imag;
	
	return(true);
}


// temp 
int FFT1DReal(MLSignal& cReal)
{
	int dir = 1;
	int w = cReal.getWidth();
	
	// allocate temp signal for imaginary values
	MLSignal cImag(w);
	cImag = cReal;
	FFT(dir, bitsToContain(w), cReal.getBuffer(), cImag.getBuffer());
	return 1;
}

// temp 
int FFT1DRealInverse(MLSignal& cReal)
{
	int dir = -1;
	int w = cReal.getWidth();
	
	// allocate temp signal for imaginary values
	MLSignal cImag(w);
	cImag = cReal;
	FFT(dir, bitsToContain(w), cReal.getBuffer(), cImag.getBuffer());
	
	// shuffle and flip 
	cImag = cReal;
	for(int i=0; i<w; ++i)
	{
		int ii = w - i;
		if (ii == w) ii = 0;
		cReal[i] = cImag[ii];
	}
	
	return 1;
}

/*
void FFTEachRowReal(MLSignal& cReal)
{
	int h = cReal.getHeight();
	int w = cReal.getWidth();
	int dir = 1;
	
	// allocate temp signal for imaginary values
	MLSignal cImag(w, h);
	cImag = cReal;
	
	for(int j=0; j<h; ++j)
	{
		float* realRow = cReal.getBuffer() + cReal.row(j);
		float* imagRow = cImag.getBuffer() + cImag.row(j);
		FFT(dir, bitsToContain(w), realRow, imagRow);
	}
}

void FFTEachRowRealInverse(MLSignal& cReal)
{
	int h = cReal.getHeight();
	int w = cReal.getWidth();
	int dir = -1;
	
	// allocate temp signal for imaginary values
	MLSignal cImag(w, h);
	cImag = cReal;
	
	for(int j=0; j<h; ++j)
	{
		float* realRow = cReal.getBuffer() + cReal.row(j);
		float* imagRow = cImag.getBuffer() + cImag.row(j);
		FFT(dir, bitsToContain(w), realRow, imagRow);
		
		// silly copy to temp
		for(int i=0; i<w; ++i)
		{
			imagRow[i] = realRow[i];
		}
		
		// shuffle and flip 
		for(int i=0; i<w; ++i)
		{
			int ii = w - i;
			if (ii == w) ii = 0;
			realRow[i] = imagRow[ii];
		}
	}	
}
*/

void FFTEachRow(MLSignal& aReal, MLSignal& bImag)
{
	int h = aReal.getHeight();
	int w = aReal.getWidth();
	int dir = 1;
	
	for(int j=0; j<h; ++j)
	{
		float* realRow = aReal.getBuffer() + aReal.row(j);
		float* imagRow = bImag.getBuffer() + bImag.row(j);
		FFT(dir, bitsToContain(w), realRow, imagRow);
	}
}

void FFTEachRowInverse(MLSignal& aReal, MLSignal& bImag)
{
	int h = aReal.getHeight();
	int w = aReal.getWidth();
	int dir = -1;
	
	for(int j=0; j<h; ++j)
	{
		float* realRow = aReal.getBuffer() + aReal.row(j);
		float* imagRow = bImag.getBuffer() + bImag.row(j);
		FFT(dir, bitsToContain(w), realRow, imagRow);
	}	
}


// divide (a + bi) by (c + di) and put the result in (a + bi)
// of course all signals must be the same dims. 
// TODO this is stupid and temporary, write as complex signals
void FFTEachRowDivide(MLSignal& aReal, MLSignal& bImag, MLSignal& cReal, MLSignal& dImag)
{
	int w = aReal.getWidth();
	int h = aReal.getHeight();
	
	MLSignal temp1(w, h);
	MLSignal temp2(w, h);
	MLSignal acMinusBd(w, h);
	MLSignal bcMinusAd(w, h);
	MLSignal denom(w, h);
	
	acMinusBd = aReal;
	acMinusBd.multiply(cReal);
	temp1 = bImag;
	temp1.multiply(dImag);
	acMinusBd.subtract(temp1);
	
	bcMinusAd = bImag;
	bcMinusAd.multiply(cReal);
	temp1 = aReal;
	temp1.multiply(dImag);
	bcMinusAd.subtract(temp1);
	
	// denom = c^2 + d^2
	temp1 = cReal;
	temp1.multiply(temp1);
	temp2 = dImag;
	temp2.multiply(temp2);
	denom = temp1;
	denom.add(temp2);
	
	// put results into aReal and bImag
	aReal = acMinusBd;
	aReal.divide(denom);
	bImag = bcMinusAd;
	bImag.divide(denom);	
}



// temp 
int FFT2DReal(MLSignal& cReal)
{
	int dir = 1;
	int w = cReal.getWidth();
	int h = cReal.getHeight();
	
	// allocate temp signal for imaginary values
	MLSignal cImag(w, h);
	cImag = cReal;
	FFT2D(cReal, cImag, w, h, dir);
	return 1;
}

// temp 
int FFT2DRealInverse(MLSignal& cReal)
{
	int dir = -1;
	int w = cReal.getWidth();
	int h = cReal.getHeight();
	
	// allocate temp signal for imaginary values
	MLSignal cImag(w, h);
	cImag = cReal;
	FFT2D(cReal, cImag, w, h, dir);	
	
	// shuffle and flip horizontal
	cImag = cReal;
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			int ii = w - i;
			if (ii == w) ii = 0;
			int jj = h - j;
			if (jj == h) jj = 0;
			cReal(i, j) = cImag(ii, jj);
		}
	}

	return 1;
}


