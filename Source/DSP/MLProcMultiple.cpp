
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcMultiple.h"

// ----------------------------------------------------------------
// registry section

namespace{

MLProcRegistryEntry<MLProcMultiple> classReg("multiple");
ML_UNUSED MLProcParam<MLProcMultiple> params[5] = {"copies", "enable", "ratio", "up_order", "down_order"};
ML_UNUSED MLProcInput<MLProcMultiple> inputs[] = {"*"};	// variable
ML_UNUSED MLProcOutput<MLProcMultiple> outputs[] = {"*"};

}	// namespace

// ----------------------------------------------------------------
// implementation

MLProcMultiple::MLProcMultiple()
{
	setParam("copies", 1);
	setParam("enable", 1);
	setParam("ratio", 1);
	setParam("up_order", 0);
	setParam("down_order", 0);
//	debug() << "MLProcMultiple constructor\n";
}

MLProcMultiple::~MLProcMultiple()
{
//	debug() << "MLProcMultiple destructor\n";
}

// make a new proxy for multiple copies of the named class.
MLProc::err MLProcMultiple::addProc(const MLSymbol className, const MLSymbol procName)
{
	MLProc::err e = OK;
	MLProcPtr pTemplate, pProxyProc;
	int proxyCopies = (int)getParam("copies");

	// is name in map already?
	MLSymbolProcMapT::iterator it = mProcMap.find(procName);
	if (it == mProcMap.end())
	{
		// new template proc
		
// debug() << "MLProcMultiple (" << (void *)this << ") adding proc " << procName << " \n";
				
//		pTemplate = newProc(className, procName + "#1");		

//		MLSymbol templateName = MLSymbol(procName.getString() + "*");
//		pTemplate = newProc(className, templateName);

		pTemplate = newProc(className, procName);
		
		if (pTemplate)
		{
			// make appropriate proxy object. 
			if(pTemplate->isContainer())
			{
				pProxyProc = newProc("multicontainer", procName);
				// setup proxy to duplicate signal and parameter interface of container class.
				MLMultiContainer& proxy = static_cast<MLMultiContainer&>(*pProxyProc);	
						
				// set context of the template proc to the new proxy.
				pProxyProc->setContext(this);
				pTemplate->setContext(&proxy);
				
				proxy.setTemplate(pTemplate); 
				proxy.setCopies(proxyCopies);

                /*
                for(int i=0; i<proxyCopies; ++i)
                {
                    
                    mProcMap[proxy.getCopy(i)->getNameWithCopyIndex()] = proxy.getCopy(i);
                    mProcList.push_back(proxy.getCopy(i));
                    debug() << "PUSHING multicontainer " << proxy.getCopy(i)->getNameWithCopyIndex() << "\n";
                    
                }
                 */
            }
			else 
			{			
				pProxyProc = newProc("multiproc", procName);
				// setup proxy to duplicate signal and parameter interface of class.
				MLMultiProc& proxy = static_cast<MLMultiProc&>(*pProxyProc);	
						
				pProxyProc->setContext(this);
				
				proxy.setTemplate(pTemplate); 
				proxy.setCopies(proxyCopies);
			
                /*
                for(int i=0; i<proxyCopies; ++i)
                {
                    mProcMap[proxy.getCopy(i)->getNameWithCopyIndex()] = proxy.getCopy(i); 
                    mProcList.push_back(proxy.getCopy(i));                    
                    debug() << "PUSHING multiproc " << proxy.getCopy(i)->getNameWithCopyIndex() << "\n";                    
                }
                 */
            }
            
			// add proxy to this container.
			mProcMap[procName] = pProxyProc;
			mProcList.push_back(pProxyProc);
		}
		else
		{
			debug() << "MLProcMultiple: addProc: couldn't make proc!\n";
			e = newProcErr;
		}
	}
	else
	{
		debug() << "MLProcMultiple: addProc: name " << procName << " already in use!\n";
		e = nameInUseErr;
	}
	return e;
}

MLProcPtr MLProcMultiple::getProc(const MLPath & path)
{
	err e;
	MLProcPtr r;
	MLProcPtr proxyProc; // proxy proc in our container
	MLProcContainer* copyAsContainer;	// owned by proxy
	MLSymbolProcMapT::iterator it;
	int proxyCopies = (int)getParam("copies");
	
	MLSymbol head = path.head();
	MLPath tail = path.tail();
	int copy = path.getCopy();
	
	// debug() << "MLProcMultiple (" << (void *)this << ") getting Proc " << head << " / " << tail << "\n";
		
	// get the proxy proc we contain, matching head name
	it = mProcMap.find(head);
		
	// if found,
	if (it != mProcMap.end())
	{
		proxyProc = it->second;	
		if (!tail.empty())
		{
			if (proxyProc->isContainer()) // proxy is a MultiContainer
			{			
				if (copy > 0)
				{
					if (copy <= proxyCopies)
					{ 
						//  get numbered copy from proxy
						MLMultiContainer& proxyContainer = static_cast<MLMultiContainer&>(*proxyProc);	
						copyAsContainer = proxyContainer.getCopyAsContainer(copy - 1);
						 // recurse into there
						r = copyAsContainer->getProc(tail);
					}
				}
				else // return template.
				{
					MLMultiContainer& proxyContainer = static_cast<MLMultiContainer&>(*proxyProc);	
					copyAsContainer = proxyContainer.getCopyAsContainer(0);
					r = copyAsContainer->getProc(tail);			
				}
				/*
				else // tail not empty and no copy number is an error
				{
					debug() << " MLProcMultiple::getProc: error: no copy specified for proxy " << head << "\n";
				}
				*/
			}
			else // proxy is a MultiProc
			{
				debug() << "ack, head proc in name is not container!\n";
				e = headNotContainerErr;
			}		
		}
		else // tail is empty, so proc as container is not needed
		{
			 // proxy is a MultiContainer or MultiProc
			if ((copy > 0) && (copy <= (int)proxyCopies)) 
			{
				//  get numbered copy from proxy
				MLMultiContainer& proxyContainer = static_cast<MLMultiContainer&>(*proxyProc);	
				r = proxyContainer.mCopies[copy-1];
			}
			else // tail empty and no copy number OK, return proxy
			{
				r = proxyProc;
			}
		}
	}
	else // return null
	{
		e = nameNotFoundErr;
	}
	return r; 
}

void MLProcMultiple::doParams()
{		
	// TODO this ordering helps avoid a race on mParamsChanged that was preventing the voices to be enabled properly.
	// the real fix will be a queue of parameter changes, kept by each container or context.
	mParamsChanged = false;
	
	for (std::list<MLProcPtr>::iterator it = mProcList.begin(); it != mProcList.end(); ++it)
	{
		int enabled = (int)getParam("enable");

		MLProcPtr proc = *it;
		MLMultProxy& proxy = dynamic_cast<MLMultProxy&>(*proc);	
		proxy.setEnabledCopies(enabled);
	}	
}

void MLProcMultiple::process(const int samples)
{
	if (mParamsChanged) 
	{
		// here for enabling copies
		doParams();
	}
	
	MLProcContainer::process(samples);
}
