

/* cephes functions, copied here to serve as a reference */

/*							sinf.c
 *
 *	Circular sine
 *
 *
 *
 * SYNOPSIS:
 *
 * float x, y, sinf();
 *
 * y = sinf( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Range reduction is into intervals of pi/4.  The reduction
 * error is nearly eliminated by contriving an extended precision
 * modular arithmetic.
 *
 * Two polynomial approximating functions are employed.
 * Between 0 and pi/4 the sine is approximated by
 *      x  +  x**3 P(x**2).
 * Between pi/4 and pi/2 the cosine is represented as
 *      1  -  x**2 Q(x**2).
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain      # trials      peak       rms
 *    IEEE    -4096,+4096   100,000      1.2e-7     3.0e-8
 *    IEEE    -8192,+8192   100,000      3.0e-7     3.0e-8
 * 
 * ERROR MESSAGES:
 *
 *   message           condition        value returned
 * sin total loss      x > 2^24              0.0
 *
 * Partial loss of accuracy begins to occur at x = 2^13
 * = 8192. Results may be meaningless for x >= 2^24
 * The routine as implemented flags a TLOSS error
 * for x >= 2^24 and returns 0.0.
 */

/*							cosf.c
 *
 *	Circular cosine
 *
 *
 *
 * SYNOPSIS:
 *
 * float x, y, cosf();
 *
 * y = cosf( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Range reduction is into intervals of pi/4.  The reduction
 * error is nearly eliminated by contriving an extended precision
 * modular arithmetic.
 *
 * Two polynomial approximating functions are employed.
 * Between 0 and pi/4 the cosine is approximated by
 *      1  -  x**2 Q(x**2).
 * Between pi/4 and pi/2 the sine is represented as
 *      x  +  x**3 P(x**2).
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain      # trials      peak         rms
 *    IEEE    -8192,+8192   100,000      3.0e-7     3.0e-8
 */

/*
 Cephes Math Library Release 2.2:  June, 1992
 Copyright 1985, 1987, 1988, 1992 by Stephen L. Moshier
 Direct inquiries to 30 Frost Street, Cambridge, MA 02140
 */


/* Single precision circular sine
 * test interval: [-pi/4, +pi/4]
 * trials: 10000
 * peak relative error: 6.8e-8
 * rms relative error: 2.6e-8
 */


static float FOPI = 1.27323954473516;
static float PIO4F = 0.7853981633974483096;
/* Note, these constants are for a 32-bit significand: */
/*
 static float DP1 =  0.7853851318359375;
 static float DP2 =  1.30315311253070831298828125e-5;
 static float DP3 =  3.03855025325309630e-11;
 static float lossth = 65536.;
 */

/* These are for a 24-bit significand: */
static float DP1 = 0.78515625;
static float DP2 = 2.4187564849853515625e-4;
static float DP3 = 3.77489497744594108e-8;
static float lossth = 8192.;
static float T24M1 = 16777215.;

static float sincof[] = {
	-1.9515295891E-4,
	8.3321608736E-3,
	-1.6666654611E-1
};
static float coscof[] = {
	2.443315711809948E-005,
	-1.388731625493765E-003,
	4.166664568298827E-002
};

float cephes_sinf( float xx )
{
	float *p;
	float x, y, z;
	register unsigned long j;
	register int sign;
	
	sign = 1;
	x = xx;
	if( xx < 0 )
	{
		sign = -1;
		x = -xx;
	}
	if( x > T24M1 )
	{
		//mtherr( "sinf", TLOSS );
		return(0.0);
	}
	j = FOPI * x; /* integer part of x/(PI/4) */
	y = j;
	/* map zeros to origin */
	if( j & 1 )
	{
		j += 1;
		y += 1.0;
	}
	j &= 7; /* octant modulo 360 degrees */
	/* reflect in x axis */
	if( j > 3)
	{
		sign = -sign;
		j -= 4;
	}
	if( x > lossth )
	{
		//mtherr( "sinf", PLOSS );
		x = x - y * PIO4F;
	}
	else
	{
		/* Extended precision modular arithmetic */
		x = ((x - y * DP1) - y * DP2) - y * DP3;
	}
	/*einits();*/
	z = x * x;
	//printf("my_sinf: corrected oldx, x, y = %14.10g, %14.10g, %14.10g\n", oldx, x, y);
	if( (j==1) || (j==2) )
	{
		/* measured relative error in +/- pi/4 is 7.8e-8 */
		/*
		 y = ((  2.443315711809948E-005 * z
		 - 1.388731625493765E-003) * z
		 + 4.166664568298827E-002) * z * z;
		 */
		p = coscof;
		y = *p++;
		y = y * z + *p++;
		y = y * z + *p++;
		y *= z; y *= z;
		y -= 0.5 * z;
		y += 1.0;
	}
	else
	{
		/* Theoretical relative error = 3.8e-9 in [-pi/4, +pi/4] */
		/*
		 y = ((-1.9515295891E-4 * z
		 + 8.3321608736E-3) * z
		 - 1.6666654611E-1) * z * x;
		 y += x;
		 */
		p = sincof;
		y = *p++;
		y = y * z + *p++;
		y = y * z + *p++;
		y *= z; y *= x;
		y += x;
	}
	/*einitd();*/
	//printf("my_sinf: j=%d result = %14.10g * %d\n", j, y, sign);
	if(sign < 0)
		y = -y;
	return( y);
}


/* Single precision circular cosine
 * test interval: [-pi/4, +pi/4]
 * trials: 10000
 * peak relative error: 8.3e-8
 * rms relative error: 2.2e-8
 */

float cephes_cosf( float xx )
{
	float x, y, z;
	int j, sign;
	
	/* make argument positive */
	sign = 1;
	x = xx;
	if( x < 0 )
		x = -x;
	
	if( x > T24M1 )
	{
		//mtherr( "cosf", TLOSS );
		return(0.0);
	}
	
	j = FOPI * x; /* integer part of x/PIO4 */
	y = j;
	/* integer and fractional part modulo one octant */
	if( j & 1 )	/* map zeros to origin */
	{
		j += 1;
		y += 1.0;
	}
	j &= 7;
	if( j > 3)
	{
		j -=4;
		sign = -sign;
	}
	
	if( j > 1 )
		sign = -sign;
	
	if( x > lossth )
	{
		//mtherr( "cosf", PLOSS );
		x = x - y * PIO4F;
	}
	else
	/* Extended precision modular arithmetic */
		x = ((x - y * DP1) - y * DP2) - y * DP3;
	
	//printf("xx = %g -> x corrected = %g sign=%d j=%d y=%g\n", xx, x, sign, j, y);
	
	z = x * x;
	
	if( (j==1) || (j==2) )
	{
		y = (((-1.9515295891E-4f * z
			   + 8.3321608736E-3f) * z
			  - 1.6666654611E-1f) * z * x)
		+ x;
	}
	else
	{
		y = ((  2.443315711809948E-005f * z
			  - 1.388731625493765E-003f) * z
			 + 4.166664568298827E-002f) * z * z;
		y -= 0.5 * z;
		y += 1.0;
	}
	if(sign < 0)
		y = -y;
	return( y );
}

/*							expf.c
 *
 *	Exponential function
 *
 *
 *
 * SYNOPSIS:
 *
 * float x, y, expf();
 *
 * y = expf( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns e (2.71828...) raised to the x power.
 *
 * Range reduction is accomplished by separating the argument
 * into an integer k and fraction f such that
 *
 *     x    k  f
 *    e  = 2  e.
 *
 * A polynomial is used to approximate exp(f)
 * in the basic range [-0.5, 0.5].
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      +- MAXLOG   100000      1.7e-7      2.8e-8
 *
 *
 * Error amplification in the exponential function can be
 * a serious matter.  The error propagation involves
 * exp( X(1+delta) ) = exp(X) ( 1 + X*delta + ... ),
 * which shows that a 1 lsb error in representing X produces
 * a relative error of X times 1 lsb in the function.
 * While the routine gives an accurate result for arguments
 * that are exactly represented by a double precision
 * computer number, the result contains amplified roundoff
 * error for large arguments not exactly represented.
 *
 *
 * ERROR MESSAGES:
 *
 *   message         condition      value returned
 * expf underflow    x < MINLOGF         0.0
 * expf overflow     x > MAXLOGF         MAXNUMF
 *
 */

/*
 Cephes Math Library Release 2.2:  June, 1992
 Copyright 1984, 1987, 1989 by Stephen L. Moshier
 Direct inquiries to 30 Frost Street, Cambridge, MA 02140
 */

/* Single precision exponential function.
 * test interval: [-0.5, +0.5]
 * trials: 80000
 * peak relative error: 7.6e-8
 * rms relative error: 2.8e-8
 */

static float MAXNUMF = 3.4028234663852885981170418348451692544e38;
static float MAXLOGF = 88.72283905206835;
static float MINLOGF = -103.278929903431851103; /* log(2^-149) */


static float LOG2EF = 1.44269504088896341;

static float C1 =   0.693359375;
static float C2 =  -2.12194440e-4;



float cephes_expf(float xx) {
	float x, z;
	int n;
	
	x = xx;
	
	
	if( x > MAXLOGF)
	{
		//mtherr( "expf", OVERFLOW );
		return( MAXNUMF );
	}
	
	if( x < MINLOGF )
	{
		//mtherr( "expf", UNDERFLOW );
		return(0.0);
	}
	
	/* Express e**x = e**g 2**n
	 *   = e**g e**( n loge(2) )
	 *   = e**( g + n loge(2) )
	 */
	z = floorf( LOG2EF * x + 0.5 ); /* floor() truncates toward -infinity. */
	
	x -= z * C1;
	x -= z * C2;
	n = z;
	
	z = x * x;
	/* Theoretical peak relative error in [-0.5, +0.5] is 4.2e-9. */
	z =
	((((( 1.9875691500E-4f  * x
   + 1.3981999507E-3f) * x
		+ 8.3334519073E-3f) * x
	   + 4.1665795894E-2f) * x
   + 1.6666665459E-1f) * x
	 + 5.0000001201E-1f) * z
	+ x
	+ 1.0;
	
	/* multiply by power of 2 */
	x = ldexpf( z, n );
	
	return( x );
}

/*							logf.c
 *
 *	Natural logarithm
 *
 *
 *
 * SYNOPSIS:
 *
 * float x, y, logf();
 *
 * y = logf( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns the base e (2.718...) logarithm of x.
 *
 * The argument is separated into its exponent and fractional
 * parts.  If the exponent is between -1 and +1, the logarithm
 * of the fraction is approximated by
 *
 *     log(1+x) = x - 0.5 x**2 + x**3 P(x)
 *
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      0.5, 2.0    100000       7.6e-8     2.7e-8
 *    IEEE      1, MAXNUMF  100000                  2.6e-8
 *
 * In the tests over the interval [1, MAXNUM], the logarithms
 * of the random arguments were uniformly distributed over
 * [0, MAXLOGF].
 *
 * ERROR MESSAGES:
 *
 * logf singularity:  x = 0; returns MINLOG
 * logf domain:       x < 0; returns MINLOG
 */

/*
 Cephes Math Library Release 2.2:  June, 1992
 Copyright 1984, 1987, 1988, 1992 by Stephen L. Moshier
 Direct inquiries to 30 Frost Street, Cambridge, MA 02140
 */

/* Single precision natural logarithm
 * test interval: [sqrt(2)/2, sqrt(2)]
 * trials: 10000
 * peak relative error: 7.1e-8
 * rms relative error: 2.7e-8
 */
float LOGE2F = 0.693147180559945309;
float SQRTHF = 0.707106781186547524;
float PIF = 3.141592653589793238;
float PIO2F = 1.5707963267948966192;
float MACHEPF = 5.9604644775390625E-8;

float cephes_logf( float xx ) {
	register float y;
	float x, z, fe;
	int e;
	
	x = xx;
	fe = 0.0;
	/* Test for domain */
	if( x <= 0.0 )
	{
		// ERROR
		return( MINLOGF );
	}
	
	x = frexpf( x, &e );
	// printf("\nmy_logf: frexp -> e = %d x = %g\n", e, x);
	if( x < SQRTHF )
	{
		e -= 1;
		x = x + x - 1.0; /*  2x - 1  */
	}	
	else
	{
		x = x - 1.0;
	}
	z = x * x;
	/* 3.4e-9 */
	/*
	 p = logfcof;
	 y = *p++ * x;
	 for( i=0; i<8; i++ )
	 {
	 y += *p++;
	 y *= x;
	 }
	 y *= z;
	 */
	
	y =
	(((((((( 7.0376836292E-2f * x
			- 1.1514610310E-1f) * x
		   + 1.1676998740E-1f) * x
		  - 1.2420140846E-1f) * x
		 + 1.4249322787E-1f) * x
		- 1.6668057665E-1f) * x
	   + 2.0000714765E-1f) * x
	  - 2.4999993993E-1f) * x
	 + 3.3333331174E-1f) * x * z;
	
	// printf("my_logf: poly = %g\n", y);
	
	if( e )
	{
		fe = e;
		y += -2.12194440e-4f * fe;
	}
	y +=  -0.5 * z;  /* y - 0.5 x^2 */
	
	// printf("my_logf: x = %g y = %g\n", x, y);
	z = x + y;   /* ... + x  */
	
	if( e )
		z += 0.693359375f * fe;
	
	
	return( z );
}