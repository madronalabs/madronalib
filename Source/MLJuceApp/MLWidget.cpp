
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLWidget.h"
#include "MLDebug.h"

static const std::string kNullStr;
static const MLSignal kNullSignal;

MLWidget::MLWidget() : 
	mGridBounds(),
	mSize(1.f),
    mLabelOffset(),
    pComponent(nullptr),
	mWantsResizeLast(false)
{

}

MLWidget::~MLWidget()
{
}

void MLWidget::setAttribute(MLSymbol attr, float val)
{
	mAttributes[attr] = val;
}

void MLWidget::setStringAttribute(MLSymbol attr, const std::string& val)
{
	mStringAttributes[attr] = val;
}

void MLWidget::setSignalAttribute(MLSymbol attr, const MLSignal& val)
{
	mSignalAttributes[attr] = val;
}

void MLWidget::setColorAttribute(MLSymbol attr, juce::Colour c)
{
    MLSignal colorSig;
    colorSig.setDims(4);
    colorSig[0] = c.getHue();
    colorSig[1] = c.getSaturation();
    colorSig[2] = c.getBrightness();
    colorSig[3] = c.getFloatAlpha();
	mSignalAttributes[attr] = colorSig;
}

// --------------------------------------------------------------------------------
// protected attribute getters, to be used only by subclasses.

float MLWidget::getAttribute(MLSymbol attr) const
{
	float result = 0.;
	std::map<MLSymbol, float>::const_iterator look = mAttributes.find(attr);
	if(look != mAttributes.end())
	{
		result = (look->second);
	}
	return result;
}

const std::string& MLWidget::getStringAttribute(MLSymbol attr) const
{
	std::map<MLSymbol, std::string>::const_iterator look = mStringAttributes.find(attr);
	if(look != mStringAttributes.end())
	{
		return look->second;
	}
	return kNullStr;
}

const MLSignal& MLWidget::getSignalAttribute(MLSymbol attr) const
{
	std::map<MLSymbol, MLSignal>::const_iterator look = mSignalAttributes.find(attr);
	if(look != mSignalAttributes.end())
	{
		return look->second;
	}
	return kNullSignal;
}

juce::Colour MLWidget::getColorAttribute(MLSymbol attr) const
{
    float h, s, v, a;
    const MLSignal& colorSig = getSignalAttribute(attr);
    h = colorSig[0];
    s = colorSig[1];
    v = colorSig[2];
    a = colorSig[3];
    return juce::Colour::fromHSV(h, s, v, a);
}

// --------------------------------------------------------------------------------

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
		MLRect r(juceToMLRect(peer->getBounds()));
		return r;
	}
	return MLRect();
}

double MLWidget::getBackingLayerScale() const
{
    ComponentPeer* peer = pComponent->getPeer();
    if(peer)
    {
        Rectangle<int> peerBounds = peer->getBounds();
        return Desktop::getInstance().getDisplays().getDisplayContaining(peerBounds.getCentre()).scale;
    }
    else
    {
        return Desktop::getInstance().getDisplays().getDisplayContaining(pComponent->getScreenBounds().getCentre()).scale;
    }
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



