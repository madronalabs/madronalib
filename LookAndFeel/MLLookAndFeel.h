
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

/*
  ==============================================================================

   This file was part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

// MLLookAndFeel is Juce-based, JuceLookAndFeel with minor extensions.
// Madrona Labs 2009-2013.

#ifndef __ML_LOOKANDFEEL_H__
#define __ML_LOOKANDFEEL_H__

#include "JuceHeader.h"
#include "MLUI.h"
#include "MLWidget.h"
#include "MLSeparator.h"
#include "MLLabel.h"
#include "MLDial.h"
#include "MLMultiSlider.h"
#include "MLButton.h"
#include "MLDrawableButton.h"
#include "MLTextButton.h"
#include "MLUIBinaryData.h"

#include <vector>

using namespace juce;

MLPoint adjust(MLPoint p);

typedef std::tr1::shared_ptr<Drawable> DrawablePtr;

const float kPopupMenuTextScale = 0.85f;

class  MLLookAndFeel : public LookAndFeel, public DeletedAtShutdown
{
public:
    //==============================================================================
    MLLookAndFeel();
    ~MLLookAndFeel();
	
	enum ColourIds
	{
		// Madrona Colors to change for themes
		outlineColor					= 0x10A0001, 
		buttonOffColor					= 0x10A0002, 
		buttonOnColor					= 0x10A0003, 
		backgroundColor					= 0x10A0004, 
		backgroundColorNeutral			= 0x10A0005, 
		shadowColor						= 0x10A0006, 
		labelColor						= 0x10A0007, 
		radioOffColor					= 0x10A0008, 
		radioOnColor					= 0x10A0009, 
		highlightColor					= 0x10A000A,
		lineColor						= 0x10A000B,
		unused1							= 0x10A000C,
		darkFillColor					= 0x10A000D,
		darkerFillColor					= 0x10A000E,
		darkestFillColor				= 0x10A000F,
		backgroundColor2				= 0x10A0010, 
		markColor						= 0x10A0011, 
		darkLabelColor					= 0x10A0012, 
		buttonBackgroundColor			= 0x10A0013, 
		defaultDialFillColor			= 0x10A0014, 
		//
		// Colors probably not to change, written with IDs for consistency
		redColor						= 0x10A002B, 
		redOrangeColor					= 0x10A002C, 
		orangeColor						= 0x10A002D, 
		yellowColor						= 0x10A002E, 
		yellowGreenColor				= 0x10A002F, 
		greenColor						= 0x10A0030, 
		greenBlueColor					= 0x10A0031, 
		blueColor						= 0x10A0032, 
		indigoColor						= 0x10A0033, 
		violetColor						= 0x10A0034, 
		violetRedColor					= 0x10A0035, 
		whiteColor						= 0x10A0036, 
		grayColor						= 0x10A0037, 
		darkGrayColor					= 0x10A0038, 
		brownColor						= 0x10A0039, 
		darkBrownColor					= 0x10A003A
	};
	
	const Font & getFont(int style);	
	
	// turn drawing of numbers for dials on or off.
	void setDrawNumbers(bool n);
	
	// turn signal viewer animations on or off.
	void setAnimate(bool n);
	bool getAnimate();	
	
    //==============================================================================
	// Drawing routines
	
    /** Draws the glow for an ML raw image button. */
    virtual void drawButtonGlow (Graphics& g,
                                       Button& button,
									   const Colour& glowColor);

    /** Draws the lozenge-shaped background for a standard button. */
    virtual void drawButtonBackground (Graphics& g,
                                       Button& button,
                                       const Colour& backgroundColour,
                                       bool isMouseOverButton,
                                       bool isButtonDown);

    /** Draws the text for a TextButton. */
    virtual void drawButtonText (Graphics& g,
                                 MLButton& button,
								const Colour& textColor,
                                 bool isMouseOverButton,
                                 bool isButtonDown);
								 
	// draw text for a MenuButton.
    virtual void drawMenuButtonText (Graphics& g,
                                 MLButton& button,
								const Colour& textColor);

    /** Draws the contents of a standard ToggleButton. */
    virtual void drawToggleButton (Graphics& g,
                                   ToggleButton& button,
                                   bool isMouseOverButton,
                                   bool isButtonDown);

    virtual void changeToggleButtonWidthToFitText (ToggleButton& button);

    virtual void drawTickBox (Graphics& g,
                              Component& component,
                              int x, int y, int w, int h,
                              const bool ticked,
                              const bool isEnabled,
                              const bool isMouseOverButton,
                              const bool isButtonDown);

  
    /** Draws a progress bar.

        (Used by progress bars in AlertWindow).
    */
    virtual void drawProgressBar (Graphics& g, ProgressBar& progressBar,
                                  int width, int height,
                                  double progress, const String& textToShow);
								  
    //==============================================================================
    /** Draws one of the buttons on a scrollbar.

        @param g                    the context to draw into
        @param scrollbar            the bar itself
        @param width                the width of the button
        @param height               the height of the button
        @param buttonDirection      the direction of the button, where 0 = up, 1 = right, 2 = down, 3 = left
        @param isScrollbarVertical  true if it's a vertical bar, false if horizontal
        @param isMouseOverButton    whether the mouse is currently over the button (also true if it's held down)
        @param isButtonDown         whether the mouse button's held down
    */
    virtual void drawScrollbarButton (Graphics& g,
                                      ScrollBar& scrollbar,
                                      int width, int height,
                                      int buttonDirection,
                                      bool isScrollbarVertical,
                                      bool isMouseOverButton,
                                      bool isButtonDown);

    /** Draws the thumb area of a scrollbar.

        @param g                    the context to draw into
        @param scrollbar            the bar itself
        @param x                    the x position of the left edge of the thumb area to draw in
        @param y                    the y position of the top edge of the thumb area to draw in
        @param width                the width of the thumb area to draw in
        @param height               the height of the thumb area to draw in
        @param isScrollbarVertical  true if it's a vertical bar, false if horizontal
        @param thumbStartPosition   for vertical bars, the y co-ordinate of the top of the
                                    thumb, or its x position for horizontal bars
        @param thumbSize            for vertical bars, the height of the thumb, or its width for
                                    horizontal bars. This may be 0 if the thumb shouldn't be drawn.
        @param isMouseOver          whether the mouse is over the thumb area, also true if the mouse is
                                    currently dragging the thumb
        @param isMouseDown          whether the mouse is currently dragging the scrollbar
    */
    virtual void drawScrollbar (Graphics& g,
                                ScrollBar& scrollbar,
                                int x, int y,
                                int width, int height,
                                bool isScrollbarVertical,
                                int thumbStartPosition,
                                int thumbSize,
                                bool isMouseOver,
                                bool isMouseDown);

    /** Returns the component effect to use for a scrollbar */
    virtual ImageEffectFilter* getScrollbarEffect();

    /** Returns the minimum length in pixels to use for a scrollbar thumb. */
    virtual int getMinimumScrollbarThumbSize (ScrollBar& scrollbar);

    /** Returns the default thickness to use for a scrollbar. */
    virtual int getDefaultScrollbarWidth();

    /** Returns the length in pixels to use for a scrollbar button. */
    virtual int getScrollbarButtonSize (ScrollBar& scrollbar);


    //==============================================================================
    /** Draws the + or - box in a treeview. */
	virtual void drawTreeviewPlusMinusBox (Graphics& g, int x, int y, int w, int h, bool isPlus, bool isMouseOver);

    //==============================================================================


    virtual void createFileChooserHeaderText (const String& title,
                                              const String& instructions,
                                              GlyphArrangement& destArrangement,
                                              int width);

    virtual Button* createFileBrowserGoUpButton();

    virtual void layoutFileBrowserComponent (FileBrowserComponent& browserComp,
                                             DirectoryContentsDisplayComponent* fileListComponent,
                                             FilePreviewComponent* previewComp,
                                             ComboBox* currentPathBox,
                                             TextEditor* filenameBox,
                                             Button* goUpButton);



    //==============================================================================
    /** Fills the background of a popup menu component. */
    virtual void drawPopupMenuBackground (Graphics& g, int width, int height);

    /** Draws one of the items in a popup menu. */
    virtual void drawPopupMenuItem (Graphics& g,
                                    int width, int height,
                                    const bool isSeparator,
                                    const bool isActive,
                                    const bool isHighlighted,
                                    const bool isTicked,
                                    const bool hasSubMenu,
                                    const String& text,
                                    const String& shortcutKeyText,
                                    Image* image,
                                    const Colour* const textColour);


    /** Returns the size and style of font to use in popup menus. */
    virtual const Font getPopupMenuFont();

    virtual void drawPopupMenuUpDownArrow (Graphics& g,
                                           int width, int height,
                                           bool isScrollUpArrow);

    /** Finds the best size for an item in a popup menu. */
    virtual void getIdealPopupMenuItemSize (const String& text,
                                            const bool isSeparator,
                                            int standardMenuItemHeight,
                                            int& idealWidth,
                                            int& idealHeight);

    virtual int getMenuWindowFlags();

    virtual void drawMenuBarBackground (Graphics& g, int width, int height,
                                        bool isMouseOverBar,
                                        MenuBarComponent& menuBar);

    virtual int getMenuBarItemWidth (MenuBarComponent& menuBar, int itemIndex, const String& itemText);

    virtual const Font getMenuBarFont (MenuBarComponent& menuBar, int itemIndex, const String& itemText);

    virtual void drawMenuBarItem (Graphics& g,
                                  int width, int height,
                                  int itemIndex,
                                  const String& itemText,
                                  bool isMouseOverItem,
                                  bool isMenuOpen,
                                  bool isMouseOverBar,
                                  MenuBarComponent& menuBar);

    //==============================================================================
    virtual void drawComboBox (Graphics& g, int width, int height,
                               const bool isButtonDown,
                               int buttonX, int buttonY,
                               int buttonW, int buttonH,
                               ComboBox& box);

    virtual const Font getComboBoxFont (ComboBox& box);

    virtual Label* createComboBoxTextBox (ComboBox& box);

    virtual Button* createDialButton (const bool isIncrement);
 //   virtual Label* createDialTextBox (MLDial& MLDial);

    virtual ImageEffectFilter* getDialEffect();


    //==============================================================================
    virtual Button* createFilenameComponentBrowseButton (const String& text);

    virtual void layoutFilenameComponent (FilenameComponent& filenameComp,
                                          ComboBox* filenameBox, Button* browseButton);

    //==============================================================================
    virtual void drawCornerResizer (Graphics& g,
                                    int w, int h,
                                    bool isMouseOver,
                                    bool isMouseDragging);

    virtual void drawResizableFrame (Graphics& g,
                                    int w, int h,
                                    const BorderSize<int>& borders);

    //==============================================================================
	/*
    virtual void drawResizableWindowBorder (Graphics& g,
                                            int w, int h,
                                            const BorderSize<int>& border,
                                            ResizableWindow& window);
	*/
    //==============================================================================
    virtual void drawDocumentWindowTitleBar (DocumentWindow& window,
                                             Graphics& g, int w, int h,
                                             int titleSpaceX, int titleSpaceW,
                                             const Image* icon,
                                             bool drawTitleTextOnLeft);


    virtual void positionDocumentWindowButtons (DocumentWindow& window,
                                                int titleBarX, int titleBarY,
                                                int titleBarW, int titleBarH,
                                                Button* minimiseButton,
                                                Button* maximiseButton,
                                                Button* closeButton,
                                                bool positionTitleBarButtonsOnLeft);

    virtual int getDefaultMenuBarHeight();

 
    //==============================================================================
    virtual void drawStretchableLayoutResizerBar (Graphics& g,
                                                  int w, int h,
                                                  bool isVerticalBar,
                                                  bool isMouseOver,
                                                  bool isMouseDragging);

    //==============================================================================
    virtual void drawGroupComponentOutline (Graphics& g, int w, int h,
                                            const String& text,
                                            const Justification& position,
                                            GroupComponent& group);

 


 
    //==============================================================================
    virtual void paintToolbarBackground (Graphics& g, int width, int height, Toolbar& toolbar);

    virtual Button* createToolbarMissingItemsButton (Toolbar& toolbar);

    virtual void paintToolbarButtonBackground (Graphics& g, int width, int height,
                                               bool isMouseOver, bool isMouseDown,
                                               ToolbarItemComponent& component);

    virtual void paintToolbarButtonLabel (Graphics& g, int x, int y, int width, int height,
                                          const String& text, ToolbarItemComponent& component);

 
    //==============================================================================
    //==============================================================================
    /* AlertWindow handling..
    */
	

    virtual AlertWindow* createAlertWindow (const String& title,
                                            const String& message,
                                            const String& button1,
                                            const String& button2,
                                            const String& button3,
                                            AlertWindow::AlertIconType iconType,
                                            int numButtons,
                                            Component* associatedComponent);

	/* //  Rectangle problem - clean up!
    virtual void drawAlertBox (Graphics& g,
                               AlertWindow& alert,
                               const Rectangle<int>& textArea,
                               TextLayout& textLayout);
	*/
	
    virtual int getAlertBoxWindowFlags();

    virtual int getAlertWindowButtonHeight();

    //==============================================================================
    /** Utility function to draw a shiny, glassy circle (for round LED-type buttons). */
    static void drawGlassSphere (Graphics& g,
                                 const float x, const float y,
                                 const float diameter,
                                 const Colour& colour,
                                 const float outlineThickness) throw();

    static void drawGlassPointer (Graphics& g,
                                  const float x, const float y,
                                  const float diameter,
                                  const Colour& colour, const float outlineThickness,
                                  const int direction) throw();

    /** Utility function to draw a shiny, glassy oblong (for text buttons). */
    static void drawGlassLozenge (Graphics& g,
                                  const float x, const float y,
                                  const float width, const float height,
                                  const Colour& colour,
                                  const float outlineThickness,
                                  const float cornerSize,
                                  const bool flatOnLeft, const bool flatOnRight,
                                  const bool flatOnTop, const bool flatOnBottom) throw();

	static void createMLRectangle (Path& p,
                               const float x, const float y,
                               const float w, const float h,
                               const float cs,
                               const unsigned flair, const float sx, const float sy,
							   const bool isOutline = false) throw();
				
	
//
#pragma mark MLLookAndFeel
//
	Typeface::Ptr pMadronaSans, pMadronaSansItalic;
	Font mPlainFont, mItalicFont, mTitleFont, mCaptionFont, mCaptionSmallFont, mNoticeFont; 
	Font mFallbackFont;
	Font mNumbersFont;

	bool getDefaultOpacity();
	bool getDefaultBufferMode();
	bool getDefaultUnclippedMode();
	
	// background
	void setGradientMode(int m) { mGradientMode = m;}	
	void setGradientSize(float f) { mGradientSize = f;}	
	void setBackgroundGradient(Graphics& g, Point<int>& gStart, Point<int>& gEnd);
	virtual void drawBackground(Graphics& g, Component* pC);
	void drawBackgroundRect(Graphics& g, Component* pC, MLRect r);
	void drawUnitGrid(Graphics& g);

	void setGlobalTextScale(float s);
	void setGridUnitSize(float s);
	float getGridUnitSize();
	void setGridUnits(double gx, double gy);
	
	int getGridUnitsX();
	int getGridUnitsY();

	float getMargin();
	float getSmallMargin();
	float getButtonTextSize(const MLButton& button);
	float getDialTextSize(const MLWidget& dial);
	float getLabelTextSize();
	float getLabelTextKerning(float textSize);
	float getButtonTextKerning(float textSize);
	float getLabelHeight();
	float getToggleButtonSize();

	void drawMLButtonShape  (Graphics& g,
                                        const MLRect& r,
                                        float maxCornerSize,
                                        const Colour& baseColor,
                                        const Colour& myOutlineColor,
                                        const float strokeWidth,
                                        const unsigned flair,
										const float sx, 
										const float sy) throw();

	void drawMLButtonShape  (Graphics& g,
                                        float x, float y, float w, float h,
                                        float maxCornerSize,
                                        const Colour& baseColour,
                                        const Colour& outlineColour,
                                        const float strokeWidth,
                                        const unsigned flair,
										const float sx, 
										const float sy) throw();
							   
	void drawShadowLine  (Graphics& g,
                                        float ax, float ay, float bx, float by,
                                        const Colour& color,
                                        const int width);

	float calcMaxNumberWidth( const int digits, const int precision, const bool doSign);
	int getDigitsAfterDecimal (const float number, const int digits, const int precision)  throw();
	
	char* formatNumber (const float number, const int digits, const int precision, 
		const bool doSign, MLValueDisplayMode mode)  throw();

	void drawNumber (Graphics& g, const char* number, const int x, const int y, 
		const int w, const int h, const Colour& c, const Justification& j = Justification::topLeft)  throw();
			
	float getNumberWidth (const float number, const int digits, const int precision, const bool doSign);

    MLLookAndFeel (const MLLookAndFeel&);
    const MLLookAndFeel& operator= (const MLLookAndFeel&);

	bool mDrawNumbers;
	bool mAnimate;
	int mGradientMode;
	float mGradientSize;
	
	int mGridUnitSize;
	double mGridUnitsX;
	double mGridUnitsY;
	float mGlobalTextScale;
	
		
	// visual resources for app. 
	void addPicture(MLSymbol name, const void* data, size_t dataSize);	
	const Drawable* getPicture(MLSymbol name);

	std::map<MLSymbol, DrawablePtr> mPictures;
	std::map<MLSymbol, void *> mPictureData;
	
	juce_DeclareSingleton_SingleThreaded_Minimal (MLLookAndFeel)
	
};

#endif // __ML_LOOKANDFEEL_H__