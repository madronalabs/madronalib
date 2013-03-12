
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLWidget.h"
#include "MLDebug.h"

static const std::string kNullStr;

MLWidget::MLWidget() : 
	pComponent(nullptr),
	mGridBounds(),
	mLabelOffset(),
	mSize(1.f),
	mWantsResizeLast(false)
{

}

MLWidget::~MLWidget()
{
}

float MLWidget::getAttribute(MLSymbol attr)
{
	float result = 0.;
	std::map<MLSymbol, float>::iterator look = mAttributes.find(attr);	
	if(look != mAttributes.end())
	{
		result = (look->second);
	}
	return result;
}

void MLWidget::setAttribute(MLSymbol attr, float val)
{
	mAttributes[attr] = val;
}

const std::string& MLWidget::getStringAttribute(MLSymbol attr)
{
	std::map<MLSymbol, std::string>::iterator look = mStringAttributes.find(attr);	
	if(look != mStringAttributes.end())
	{
		return look->second;
	}
	return kNullStr;
}

void MLWidget::setStringAttribute(MLSymbol attr, const std::string& val)
{
	mStringAttributes[attr] = val;
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

// get bounds of top-level window containing the widget.
//
MLRect MLWidget::getWidgetWindowBounds()
{
	// adapt JUCE rect to MLRect
	if(pComponent)
	{
		ComponentPeer *peer = pComponent->getPeer();
		MLPoint p(juceToMLPoint(peer->getScreenPosition()));
		MLRect r(juceToMLRect(peer->getBounds()));
		return r + p;
	}
	return MLRect();
}

void MLWidget::resizeWidget(const MLRect& b, const int)
{
	// adapt vrect to juce rect
	if(pComponent)
	{
		pComponent->setBounds(b.left(), b.top(), b.width(), b.height());
	}
}

void MLWidget::setWidgetVisible(bool v)
{
	if(pComponent)
	{
		pComponent->setVisible(v);
		if(v)
		{
			pComponent->repaint();
		}
	}
}

void MLWidget::enterPaint()
{
//	debug() << " paint: " << getWidgetName() << "\n";
}



