
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLWidget.h"
#include "MLDebug.h"
#include "MLAppView.h"

MLWidget::MLWidget(MLWidget* pC) :
	MLPropertyListener(this),
	pComponent(nullptr),
	mpContainer(pC),
	mSize(1.f),
	mGridBounds(),
	mGridUnitSize(0),
	mLabelOffset(),
//	pGLContext(nullptr),
	mWantsResizeLast(false)
{
}

MLWidget::~MLWidget()
{
	/*
    if(pGLContext)
    {
        pGLContext->detach();
        delete pGLContext;
    }
	 */
}

void MLWidget::doPropertyChangeAction(ml::Symbol param, const MLProperty& newVal)
{
	if(pComponent)
	{
		pComponent->repaint();
	}
}

void MLWidget::addListener(MLWidget::Listener* pL)
{
	// TODO check for duplicates
	mpListeners.push_back(pL);
}

void MLWidget::sendAction(ml::Symbol msg, ml::Symbol targetProperty, const MLProperty& val)
{
	std::vector<MLWidget::Listener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLWidget::Listener* pL = *it;
		pL->handleWidgetAction(this, msg, targetProperty, val);
	}
}


// TODO this looks pretty bad! A Widget should not have its own context. There should
// be something like a Scene object that draws a bunch of GL Widgets. 
void MLWidget::setupGL(Component* pC)
{
    if(pComponent)
    {
        pGLContext = new OpenGLContext();
        pGLContext->setRenderer (this);
        pGLContext->setComponentPaintingEnabled (false);
        pGLContext->setContinuousRepainting(true);
    }
}


float MLWidget::getRenderingScale() const
{
    float t = 1.0f;
	/*
    if(pGLContext)
    {
        t = pGLContext->getRenderingScale();
    }
	 */
    return t;
}

int MLWidget::getBackingLayerWidth() const
{
    return getComponent()->getWidth() * getRenderingScale();
}

int MLWidget::getBackingLayerHeight() const
{
    return getComponent()->getHeight() * getRenderingScale();
}

void MLWidget::setGridBounds(const MLRect& p)
{
	mGridBounds = p;
}

const MLRect& MLWidget::getGridBounds() const
{
	return mGridBounds;
}

void MLWidget::setWidgetBounds(const MLRect& b)
{
	// adapt vrect to juce rect
	if(pComponent)
	{
		pComponent->setBounds(b.left(), b.top(), b.width(), b.height());
	}
}

MLRect MLWidget::getWidgetBounds()
{
	// adapt JUCE rect to MLRect
	if(pComponent)
	{
		Rectangle<int> jRect = pComponent->getBounds();
		return MLRect(jRect.getX(), jRect.getY(), jRect.getWidth(), jRect.getHeight());
	}
	return MLRect();
}

MLRect MLWidget::getWidgetLocalBounds()
{
	// adapt JUCE rect to MLRect
	if(pComponent)
	{
		Rectangle<int> jRect = pComponent->getBounds();
		return MLRect(0, 0, jRect.getWidth(), jRect.getHeight());
	}
	return MLRect();
}

// walk upwards and add widget locations to get bounds relative to enclosing window
MLRect MLWidget::getWidgetBoundsInWindow()
{
	MLRect bounds = getWidgetBounds();	
	MLWidget* pC = this;
	MLRect parentBounds = bounds;
	
	while(1)
	{
		MLWidget* pParent = pC->getContainer();
		parentBounds = pParent->getWidgetBounds();
		if(pC == pParent) break;

		bounds += parentBounds.getTopLeft();
		pC = pParent;
	}
	
	return bounds;
}

// get bounds of top-level Component containing the widget.
//
MLRect MLWidget::getTopLevelWindowBounds()
{
	MLWidgetContainer* pC = static_cast<MLWidgetContainer*>(getContainer());		
	return juceToMLRect(pC->getRootView()->getBounds());
}

void MLWidget::resizeWidget(const MLRect& b, const int)
{
	// adapt vrect to juce rect
	if(pComponent)
	{
		pComponent->setBounds(b.left(), b.top(), b.width(), b.height());
		/*
        if(pGLContext)
        {
            pGLContext->attachTo (*pComponent);
        }
		 */
	}
}

void MLWidget::setWidgetVisible(bool v)
{
	if(pComponent)
	{
		pComponent->setVisible(v);
	}
}


void MLWidget::setWidgetEnabled(bool v)
{
	if(pComponent)
	{
		pComponent->setEnabled(v);
		pComponent->repaint();
	}
}



