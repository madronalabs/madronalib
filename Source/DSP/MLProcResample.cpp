
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// lots of optimization to be done here.  

// Note Sean's comment:
// It is worth noting that if these filters are being used for upsampling/downsampling, 
// the "noble identity" can be used to reduce the CPU cost. The basic idea is that 
// operations that can be expressed in the form: filter that uses z^-N for its states -> 
// downsample by N can be rearranged to use the form downsample by N -> filter that uses 
// z^-1 for its states The same property holds true for upsampling. 
// See http://mue.music.miami.edu/thesis/jvandekieft/jvchapter3.htm for more details. 
// For the above code, this would entail creating an alternative allpass process function, 
// that uses the z^-1 for its states, and then rearranging some of the operations.

#include "MLProc.h"

// ----------------------------------------------------------------
// allpass
// ----------------------------------------------------------------
// class definition

#pragma mark MLProcResample

class MLProcResample : public MLProc
{
public:

	class CAllPassFilter
	{
	public:
		CAllPassFilter(const MLSample coefficient)
		{
			clear();
			a=coefficient;
		}
		
		~CAllPassFilter(){};
		
		inline MLSample process(MLSample input)
		{
			//shuffle history
			x2=x1;
			y2=y1;
			x1=x0;
			y1=y0;
			x0=input;

			//allpass filter 1
			y0 = x2 + (x0-y2)*a;
			return y0;
		}
		
		void clear()
		{
			x0=0.0;
			x1=0.0;
			x2=0.0;
			y0=0.0;
			y1=0.0;
			y2=0.0;
		}

	private:
		MLSample a;
		MLSample x0;
		MLSample x1;
		MLSample x2;
		MLSample y0;
		MLSample y1;
		MLSample y2;
	};

	// ----------------------------------------------------------------
	// allpass cascade

	class CAllPassFilterCascade
	{
	public:
		CAllPassFilterCascade(const MLSample* coefficient, const int N) : allpassfilter(0)
		{
			allpassfilter = new CAllPassFilter*[N];
			for (int i = 0;i < N; i++)
			{
				allpassfilter[i] = new CAllPassFilter(coefficient[i]);
			}
			numfilters = N;
		}
		
		~CAllPassFilterCascade()
		{
			for (int i = 0;i < numfilters; i++)
			{
				if (allpassfilter[i]) delete allpassfilter[i];
			}
			if (allpassfilter) delete[] allpassfilter;
		}

		MLSample process(const MLSample input)
		{
			MLSample x, y = input;
			CAllPassFilter* pFilter;
			x = input;
			for(int i=0; i<numfilters; ++i)
			{
				pFilter = allpassfilter[i];
				y = pFilter->process(x);
				x = y;
			}
			return y;
		}
		
		void clear()
		{
			for(int i=0; i<numfilters; ++i)
			{
				CAllPassFilter* pFilter = allpassfilter[i];
				pFilter->clear();
			}
		}

	private:
		CAllPassFilter** allpassfilter; // TODO modernize
		int numfilters;
	};

	// ----------------------------------------------------------------
	// a half band filter class. 
	// polyphase two-path structure due to fred harris, A. G. Constantinides and Valenzuela.
	// adapted from code by Dave Waugh of Muon Software.
	
	class HalfBandFilter
	{
	public:
		HalfBandFilter(const int order, const bool steep)
		{
			if (steep==true)
			{
				if (order==12)	//rejection=104dB, transition band=0.01
				{
					MLSample a_coefficients[6]=
					{0.036681502163648017
					,0.2746317593794541
					,0.56109896978791948
					,0.769741833862266
					,0.8922608180038789
					,0.962094548378084
					};

					MLSample b_coefficients[6]=
					{0.13654762463195771
					,0.42313861743656667
					,0.6775400499741616
					,0.839889624849638
					,0.9315419599631839
					,0.9878163707328971
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,6);
					filter_b=new CAllPassFilterCascade(b_coefficients,6);
				}
				else if (order==10)	//rejection=86dB, transition band=0.01
				{
					MLSample a_coefficients[5]=
					{0.051457617441190984
					,0.35978656070567017
					,0.6725475931034693
					,0.8590884928249939
					,0.9540209867860787
					};

					MLSample b_coefficients[5]=
					{0.18621906251989334
					,0.529951372847964
					,0.7810257527489514
					,0.9141815687605308
					,0.985475023014907
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,5);
					filter_b=new CAllPassFilterCascade(b_coefficients,5);
				}
				else if (order==8)	//rejection=69dB, transition band=0.01
				{
					MLSample a_coefficients[4]=
					{0.07711507983241622
					,0.4820706250610472
					,0.7968204713315797
					,0.9412514277740471
					};

					MLSample b_coefficients[4]=
					{0.2659685265210946
					,0.6651041532634957
					,0.8841015085506159
					,0.9820054141886075
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,4);
					filter_b=new CAllPassFilterCascade(b_coefficients,4);
				}
				else if (order==6)	//rejection=51dB, transition band=0.01
				{
					MLSample a_coefficients[3]=
					{0.1271414136264853
					,0.6528245886369117
					,0.9176942834328115
					};

					MLSample b_coefficients[3]=
					{0.40056789819445626
					,0.8204163891923343
					,0.9763114515836773
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,3);
					filter_b=new CAllPassFilterCascade(b_coefficients,3);
				}
				else if (order==4)	//rejection=53dB,transition band=0.05
				{
					MLSample a_coefficients[2]=
					{0.12073211751675449
					,0.6632020224193995
					};

					MLSample b_coefficients[2]=
					{0.3903621872345006
					,0.890786832653497
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,2);
					filter_b=new CAllPassFilterCascade(b_coefficients,2);
				}
			
				else	//order=2, rejection=36dB, transition band=0.1
				{
					MLSample a_coefficients=0.23647102099689224;
					MLSample b_coefficients=0.7145421497126001;

					filter_a=new CAllPassFilterCascade(&a_coefficients,1);
					filter_b=new CAllPassFilterCascade(&b_coefficients,1);
				}
			}
			else	//softer slopes, more attenuation and less stopband ripple
			{
				if (order==12)	//rejection=150dB, transition band=0.05
				{
					MLSample a_coefficients[6]=
					{0.01677466677723562
					,0.13902148819717805
					,0.3325011117394731
					,0.53766105314488
					,0.7214184024215805
					,0.8821858402078155
					};

					MLSample b_coefficients[6]=
					{0.06501319274445962
					,0.23094129990840923
					,0.4364942348420355
					,0.6329609551399348
					,0.80378086794111226
					,0.9599687404800694
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,6);
					filter_b=new CAllPassFilterCascade(b_coefficients,6);
				}
				else if (order==10)	//rejection=133dB, transition band=0.05
				{
					MLSample a_coefficients[5]=
					{0.02366831419883467
					,0.18989476227180174
					,0.43157318062118555
					,0.6632020224193995
					,0.860015542499582
					};

					MLSample b_coefficients[5]=
					{0.09056555904993387
					,0.3078575723749043
					,0.5516782402507934
					,0.7652146863779808
					,0.95247728378667541
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,5);
					filter_b=new CAllPassFilterCascade(b_coefficients,5);
				}
				else if (order==8)	//rejection=106dB, transition band=0.05
				{
					MLSample a_coefficients[4]=
					{0.03583278843106211
					,0.2720401433964576
					,0.5720571972357003
					,0.827124761997324
					};

					MLSample b_coefficients[4]=
					{0.1340901419430669
					,0.4243248712718685
					,0.7062921421386394
					,0.9415030941737551
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,4);
					filter_b=new CAllPassFilterCascade(b_coefficients,4);
				}
				else if (order==6)	//rejection=80dB, transition band=0.05
				{
					MLSample a_coefficients[3]=
					{0.06029739095712437
					,0.4125907203610563
					,0.7727156537429234
					};

					MLSample b_coefficients[3]=
					{0.21597144456092948
					,0.6043586264658363
					,0.9238861386532906
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,3);
					filter_b=new CAllPassFilterCascade(b_coefficients,3);
				}
				else if (order==4)	//rejection=70dB,transition band=0.1
				{
					MLSample a_coefficients[2]=
					{0.07986642623635751
					,0.5453536510711322
					};

					MLSample b_coefficients[2]=
					{0.28382934487410993
					,0.8344118914807379
					};
			
					filter_a=new CAllPassFilterCascade(a_coefficients,2);
					filter_b=new CAllPassFilterCascade(b_coefficients,2);
				}
			
				else	//order=2, rejection=36dB, transition band=0.1
				{
					MLSample a_coefficients=0.23647102099689224f;
					MLSample b_coefficients=0.7145421497126001f;
					
					filter_a=new CAllPassFilterCascade(&a_coefficients,1);
					filter_b=new CAllPassFilterCascade(&b_coefficients,1);
				}
			}
			clear();
		}
		
		~HalfBandFilter()
		{
			delete filter_a;
			delete filter_b;
		}

		void clear()
		{
			mb1=0.0f;
			filter_a->clear();
			filter_b->clear();
		}

		MLSample process(const MLSample input)
		{
			MLSample a0, b0, out;
			
			a0 = filter_a->process(input);
			b0 = filter_b->process(input);
			out = (a0 + mb1)*0.5f;
			mb1 = b0;
			return out;
		}
		
	private:
		CAllPassFilterCascade* filter_a;
		CAllPassFilterCascade* filter_b;
		MLSample mb1;
	};

	MLProcResample();
	~MLProcResample();
	
	void setup();
	MLProc::err resize();
	void clear();
	void process(const int frames);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcResample> mInfo;
	MLRatio mRatio;
	int mUpOrder;
	int mDownOrder;
	float mx1; // prev input value
	HalfBandFilter* mFilters[4]; // for second order downsampling
	MLSignal mUp; // temp buffer for resampling up then down.
	
	void upsample0(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio);
	void upsample1(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio);
	void upsample2(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio);
	void downsample0(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio);
	void downsample1(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio);
	void downsample2(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio);
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcResample> classReg("resample");
	ML_UNUSED MLProcParam<MLProcResample> params[4] = { "ratio_top", "ratio_bottom", "up_order", "down_order" };
	ML_UNUSED MLProcInput<MLProcResample> inputs[] = {"in"};	
	ML_UNUSED MLProcOutput<MLProcResample> outputs[] = {"out"};
}


// ----------------------------------------------------------------
// implementation


MLProcResample::MLProcResample() 
{
	int halfBandOrder = 8; // not the overall resampling order
	int steep = 1;
	mx1 = 0.f;
	for(int n=0; n<4; ++n)
	{
		mFilters[n] = 0;
		mFilters[n] = new HalfBandFilter(halfBandOrder, steep);
	}
	// setup defaults
	setParam("ratio_top", 1);
	setParam("ratio_bottom", 1);
	setParam("up_order", 0);
	setParam("down_order", 0);
}

MLProcResample::~MLProcResample()
{
	for(int n=0; n<4; ++n)
	{
		if (mFilters[n])
		{
			delete(mFilters[n]);
			mFilters[n] = 0;
		}
	}
}

// set changes based on startup parameters, before prepareToProcess() is called.
void MLProcResample::setup()
{
	const int up = (int)getParam("ratio_top");
	const int down = (int)getParam("ratio_bottom");
	mRatio.set(up, down);
	mUpOrder = (int)getParam("up_order");
	mDownOrder = (int)getParam("down_order");
}

MLProc::err MLProcResample::resize() 
{	
	MLProc::err e = OK;
	int upsize = getContextVectorSize() * mRatio.top;
	if (!mUp.setDims(upsize))
	{
		e = MLProc::memErr;
	}
	return e;
}

void MLProcResample::clear()
{
	mx1 = 0.f;
	for(int n=0; n<4; ++n)
	{
		if (mFilters[n])
		{
			mFilters[n]->clear();
		}
	}
}

void MLProcResample::upsample0(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio)
{
	const int upFrames = inFrames * ratio;
	switch(ratio)
	{
		case 2: 
			for (int n=0; n < upFrames; ++n)
			{
				pDest[n] = pSrc[n >> 1];
			}
		break;
		
		case 4: 
			for (int n=0; n < upFrames; ++n)
			{
				pDest[n] = pSrc[n >> 2];
			}
		break;
		
		case 8: 
			for (int n=0; n < upFrames; ++n)
			{
				pDest[n] = pSrc[n >> 3];
			}
		break;
		
		default: 
			debug() << "MLProcResample: invalid upsample ratio " << ratio << "!\n";
		break;
	}
}

void MLProcResample::upsample1(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio)
{
	const int upFrames = inFrames * ratio;
	MLSample x;
	int m = 0;
	switch(ratio)
	{
		case 2: 			
			for (int n=0; n < upFrames; )
			{
				x = pSrc[m++];
				pDest[n++] = (x + mx1) * 0.5f;
				pDest[n++] = x;
				mx1 = x;
			}
		break;
		
		case 4:
			for (int n=0; n < upFrames; )
			{
				x = pSrc[m++];
				pDest[n++] = x*0.25f + mx1*0.75f;
				pDest[n++] = x*0.5f + mx1*0.5f;
				pDest[n++] = x*0.75f + mx1*0.25f;
				pDest[n++] = x;
				mx1 = x;
			}
		break;
		
		case 8:
			for (int n=0; n < upFrames; )
			{
				x = pSrc[m++];
				pDest[n++] = x*0.125f + mx1*0.875f;
				pDest[n++] = x*0.25f + mx1*0.75f;
				pDest[n++] = x*0.375f + mx1*0.625f;
				pDest[n++] = x*0.5f + mx1*0.5f;
				pDest[n++] = x*0.625f + mx1*0.375f;
				pDest[n++] = x*0.75f + mx1*0.25f;
				pDest[n++] = x*0.875f + mx1*0.125f;
				pDest[n++] = x;
				mx1 = x;
			}
		break;
		
		default: 
			debug() << "MLProcResample: invalid upsample ratio " << ratio << "!\n";
		break;
	}
}

// TEMP order 0 placeholder
void MLProcResample::upsample2(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio)
{
	const int upFrames = inFrames * ratio;
	switch(ratio)
	{
		case 2: 
			for (int n=0; n < upFrames; ++n)
			{
				pDest[n] = pSrc[n >> 1];
			}
		break;
		
		case 4: 
			for (int n=0; n < upFrames; ++n)
			{
				pDest[n] = pSrc[n >> 2];
			}
		break;
		
		case 8: 
			for (int n=0; n < upFrames; ++n)
			{
				pDest[n] = pSrc[n >> 3];
			}
		break;
		
		default: 
			debug() << "MLProcResample: invalid upsample ratio " << ratio << "!\n";
		break;
	}
}

void MLProcResample::downsample0(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio)
{
	int m=0;
	switch(ratio)
	{
		case 2:
			for (int n=0; n < inFrames; n += 2)
			{
				pDest[m++] = pSrc[n];
			}
		break;
		case 4:
			for (int n=0; n < inFrames; n += 4)
			{
				pDest[m++] = pSrc[n];
			}
		case 8:
			for (int n=0; n < inFrames; n += 8)
			{
				pDest[m++] = pSrc[n];
			}
		break;
	}
}

void MLProcResample::downsample1(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio)
{
	int m=0;
	switch(ratio)
	{
		case 2:
			for (int n=0; n < inFrames; n += 2)
			{
				pDest[m++] = (pSrc[n] + pSrc[n+1])*0.5f;
			}			
		break;
		case 4:
			for (int n=0; n < inFrames; n += 4)
			{
				pDest[m++] = (pSrc[n] + pSrc[n+1] + pSrc[n+2] + pSrc[n+3]) * 0.25f ;
			}
		case 8:
			for (int n=0; n < inFrames; n += 8)
			{
				pDest[m++] = 
					(pSrc[n] + pSrc[n+1] + pSrc[n+2] + pSrc[n+3]
					+ pSrc[n+4] + pSrc[n+5] + pSrc[n+6] + pSrc[n+7]) * 0.125f;
			}
		break;
	}
}
	
void MLProcResample::downsample2(MLSample* pSrc, MLSample* pDest, int inFrames, int ratio)
{
	int m=0;
	// TEMP 
	// TODO rewrite to figure out denorm problems and get rid of this hack.
	static const float noiseAmp = dBToAmp(-120.f);

	switch(ratio)
	{
		case 2:
			for (int n = 0; n < inFrames; n += 2)
			{
				MLSample sss = MLRand() * noiseAmp;
				mFilters[0]->process(pSrc[n] + sss);
				pDest[m++] = mFilters[0]->process(pSrc[n + 1] + sss);	
			}
		break;
		case 4:
			for (int n = 0; n < inFrames; n += 4)
			{
				MLSample temp1, temp2;
				// 4x->2x
				mFilters[1]->process(pSrc[n]);
				temp1 = mFilters[1]->process(pSrc[n + 1]);
				mFilters[1]->process(pSrc[n + 2]);
				temp2 = mFilters[1]->process(pSrc[n + 3]);
				// 2x->1x
				mFilters[0]->process(temp1);
				pDest[m++] = mFilters[0]->process(temp2);
			}
		break;
		case 8:
			for (int n = 0; n < inFrames; n += 8)
			{
				MLSample temp1, temp2, temp3, temp4;
				// 8x->4x
				mFilters[2]->process(pSrc[n]);
				temp1 = mFilters[2]->process(pSrc[n + 1]);
				mFilters[2]->process(pSrc[n + 2]);
				temp2 = mFilters[2]->process(pSrc[n + 3]);
				mFilters[2]->process(pSrc[n + 4]);
				temp3 = mFilters[2]->process(pSrc[n + 5]);
				mFilters[2]->process(pSrc[n + 6]);
				temp4 = mFilters[2]->process(pSrc[n + 7]);
				
				// 4x->2x
				mFilters[1]->process(temp1);
				temp1 = mFilters[1]->process(temp2);
				mFilters[1]->process(temp3);
				temp2 = mFilters[1]->process(temp4);
				
				// 2x->1x
				mFilters[0]->process(temp1);
				pDest[m++] = mFilters[0]->process(temp2);
			}
		break;
		case 16: 
			for (int n = 0; n < inFrames; n += 16)
			{
				MLSample temp1, temp2, temp3, temp4;
				MLSample temp5, temp6, temp7, temp8;
				// 16x->8x
				mFilters[3]->process(pSrc[n]);
				temp1 = mFilters[3]->process(pSrc[n + 1]);
				mFilters[3]->process(pSrc[n + 2]);
				temp2 = mFilters[3]->process(pSrc[n + 3]);
				mFilters[3]->process(pSrc[n + 4]);
				temp3 = mFilters[3]->process(pSrc[n + 5]);
				mFilters[3]->process(pSrc[n + 6]);
				temp4 = mFilters[3]->process(pSrc[n + 7]);
				mFilters[3]->process(pSrc[n + 8]);
				temp5 = mFilters[3]->process(pSrc[n + 9]);
				mFilters[3]->process(pSrc[n + 10]);
				temp6 = mFilters[3]->process(pSrc[n + 11]);
				mFilters[3]->process(pSrc[n + 12]);
				temp7 = mFilters[3]->process(pSrc[n + 13]);
				mFilters[3]->process(pSrc[n + 14]);
				temp8 = mFilters[3]->process(pSrc[n + 15]);

				// 8x->4x
				mFilters[2]->process(temp1);
				temp1 = mFilters[2]->process(temp2);
				mFilters[2]->process(temp3);
				temp2 = mFilters[2]->process(temp4);
				mFilters[2]->process(temp5);
				temp3 = mFilters[2]->process(temp6);
				mFilters[2]->process(temp7);
				temp4 = mFilters[2]->process(temp8);
				// 4x->2x
				mFilters[1]->process(temp1);
				temp1 = mFilters[1]->process(temp2);
				mFilters[1]->process(temp3);
				temp2 = mFilters[1]->process(temp4);
				// 2x->1x
				mFilters[0]->process(temp1);
				pDest[m++] = mFilters[0]->process(temp2);
			}
		break;
		default:
			debug() << "MLProcResample: invalid downsample ratio " << ratio << "!\n";
		break;
	}
}

void MLProcResample::process(const int inFrames)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	if (mRatio.isUnity())
	{
		debug() << "MLProcResample: unity ratio!\n";
		return;
	}
    
  	if (x.isConstant())
	{
		y.setToConstant(x[0]);
	}
	else
	{
		// get buffer sizes. the checking in prepareToProcess() should insure
		// that all these numbers are integers.
		const int upRatio = mRatio.top;
		const int downRatio = mRatio.bottom;
		const int upFrames = inFrames*upRatio;
		MLSample* pUpSrc, *pUpDest;
		MLSample* pDownSrc, *pDownDest;
		
		// set sources and destinations for resampling operations
		if (upRatio == 1)
		{
			pDownSrc = x.getBuffer();
			pDownDest = y.getBuffer();
		}
		else if (downRatio == 1)
		{
			pUpSrc = x.getBuffer();
			pUpDest = y.getBuffer();
		}
		else
		{
			pUpSrc = x.getBuffer();
			pUpDest = mUp.getBuffer();
			pDownSrc = mUp.getBuffer();
			pDownDest = y.getBuffer();
		}
		
		if (upRatio != 1)
		{
			switch(mUpOrder)
			{
				case 0:
					upsample0(pUpSrc, pUpDest, inFrames, upRatio);
				break;
				case 1:
					upsample1(pUpSrc, pUpDest, inFrames, upRatio);
				break;
				case 2:
					upsample2(pUpSrc, pUpDest, inFrames, upRatio);
				break;
			}
		}
		
		if (downRatio != 1)
		{
			switch(mDownOrder)
			{
				case 0:
					downsample0(pDownSrc, pDownDest, upFrames, downRatio);
				break;
				case 1:
					downsample1(pDownSrc, pDownDest, upFrames, downRatio);
				break;
				case 2:
					downsample2(pDownSrc, pDownDest, upFrames, downRatio);
				break;
			}
		}
	}
}


/*
> ===== Fractional Delay 2X Upsampling =====
>
> Tested one more upsampling permutation, which worked the best, at 
> least when paired with the polyphase halfband filter. Very clean!
>
> Used JOS's simple fractional sample Allpass delay to guestimate the 
> intermediate samples. Something like this--
>
> //globals or class properties
> //Allpass delay vars
> double LastAPIn;
> double LastAPOut;
>
> //locals
> double tmpAPIn;
> double tmpAPOut;
> //buffer pointers
> float *PIndx;
> float *POutdx;
> float *PIndxTop;
>
> while (PIndx < PIndxTop) //oversample 2X
> {
>  tmpAPIn = *PIndx; //fetch insample
>  tmpAPOut = 0.33333333 * (tmpAPIn - LastAPOut) + LastAPIn;
>  //allpass delay by one-half sample
>  LastAPIn = tmpAPIn; //save previous values
>  LastAPOut = tmpAPOut;
>  *POutdx = tmpAPOut;
>  //write delay-interpolated insample to out
>  POutdx += 1; //inc out ptr
>  *POutdx = tmpAPIn;
>  //write original insample to out
>  POutdx += 1; //inc out ptr
>  PIndx += 1; //inc in ptr
> }
>
> When paired with the polyphase halfband filter, dunno why the 
> half-sample Allpass delay works all that much better than 
> zero-stuffing or repeat-sample.
>
*/

