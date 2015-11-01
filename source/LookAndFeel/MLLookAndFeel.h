
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// Portions of this software originate from JUCE, 
// copyright 2004-2013 by Raw Material Software ltd.
// JUCE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// MLLookAndFeel is Juce-based, JuceLookAndFeel with extensions.
// Madrona Labs 2009-2013.

#ifndef __ML_LOOKANDFEEL_H__
#define __ML_LOOKANDFEEL_H__

#include "JuceHeader.h"
#include "MLUI.h"
#include "MLWidget.h"
#include "MLSeparator.h"
#include "MLLabel.h"
//#include "MLDial.h"
#include "MLMultiSlider.h"
#include "MLButton.h"
#include "MLDrawableButton.h"
#include "MLTextButton.h"
#include "MLUIBinaryData.h"

#include <vector>

using namespace juce;

MLPoint adjust(MLPoint p);

typedef std::shared_ptr<Drawable> DrawablePtr;

const float kPopupMenuTextScale = 0.85f;
const int kBackgroundBorder = 32;

class MLLookAndFeel : public LookAndFeel_V3, public DeletedAtShutdown
{
public:
    //==============================================================================
    MLLookAndFeel();
    ~MLLookAndFeel();
    
    // call this after setting up an application's custom colors to spread them
    // back to the JUCE defaults. 
    void sendMLColorsToJUCE();
    
	enum ColourIds
	{
		// Madrona Colors to change for themes
		backgroundColor					= 0x10A0001,    // middle of gradient or flat bg
		backgroundColor2				= 0x10A0002,    // top and bottom of bg gradient
		defaultFillColor                = 0x10A0003,    // positive fill for dials and others
		outlineColor					= 0x10A0004,    // outlines of controls
		labelColor						= 0x10A0005,    // small text labels
		textHeaderColor					= 0x10A0015,    // big text headers
		darkLabelColor					= 0x10A0006,    // sections outlines, drawings
		highlightColor					= 0x10A0007,    // color to lighten things
		shadowColor						= 0x10A0008,    // color to darken things
		markColor						= 0x10A0009,    // marks like separators, default text color
		lightFillColor                  = 0x10A000C,    // for clickable areas like button backgrounds
		darkFillColor					= 0x10A000D,    // negative empty things like dial tracks
		darkerFillColor					= 0x10A000E,
		darkestFillColor				= 0x10A000F,
        
		// reference hues, written with IDs for consistency
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
    void drawButtonGlow (Graphics& g,
                         juce::Button& button,
									   const Colour& glowColor);

    /** Draws the lozenge-shaped background for a standard button. */
    void drawButtonBackground (Graphics& g,
                                       MLButton& button,
                                       const Colour& backgroundColour,
                                       bool isMouseOverButton,
                                       bool isButtonDown,
                                        float outlineThickness = kMLButtonOutlineThickness);

    /** Draws the text for a TextButton. */
    void drawButtonText (Graphics& g,
                                 MLButton& button,
								const Colour& textColor,
                                 bool isMouseOverButton,
                                 bool isButtonDown);
								 
	// draw text for a MenuButton.
    void drawMenuButtonText (Graphics& g,
                                 MLButton& button,
								const Colour& textColor);

    /** Draws the contents of a standard ToggleButton. */
    void drawToggleButton (Graphics& g,
                                   ToggleButton& button,
                                   bool isMouseOverButton,
                                   bool isButtonDown);

    void changeToggleButtonWidthToFitText (ToggleButton& button);

    void drawTickBox (Graphics& g,
                              Component& component,
                              int x, int y, int w, int h,
                              const bool ticked,
                              const bool isEnabled,
                              const bool isMouseOverButton,
                              const bool isButtonDown);

  
    /** Draws a progress bar.

        (Used by progress bars in AlertWindow).
    */
    void drawProgressBar (Graphics& g, ProgressBar& progressBar,
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
    void drawScrollbarButton (Graphics& g,
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
    void drawScrollbar (Graphics& g,
                                ScrollBar& scrollbar,
                                int x, int y,
                                int width, int height,
                                bool isScrollbarVertical,
                                int thumbStartPosition,
                                int thumbSize,
                                bool isMouseOver,
                                bool isMouseDown);

    /** Returns the component effect to use for a scrollbar */
    ImageEffectFilter* getScrollbarEffect();

    /** Returns the minimum length in pixels to use for a scrollbar thumb. */
    int getMinimumScrollbarThumbSize (ScrollBar& scrollbar);

    /** Returns the default thickness to use for a scrollbar. */
    int getDefaultScrollbarWidth();

    /** Returns the length in pixels to use for a scrollbar button. */
    int getScrollbarButtonSize (ScrollBar& scrollbar);

    //==============================================================================
    /** Draws the + or - box in a treeview. */
	void drawTreeviewPlusMinusBox (Graphics& g, int x, int y, int w, int h, bool isPlus, bool isMouseOver);

    //==============================================================================

/*
    void createFileChooserHeaderText (const String& title,
                                              const String& instructions,
                                              GlyphArrangement& destArrangement,
                                              int width);
*/

    void layoutFileBrowserComponent (FileBrowserComponent& browserComp,
                                             DirectoryContentsDisplayComponent* fileListComponent,
                                             FilePreviewComponent* previewComp,
                                             ComboBox* currentPathBox,
                                             TextEditor* filenameBox,
                                             juce::Button* goUpButton);



    //==============================================================================
    /** Fills the background of a popup menu component. */
    void drawPopupMenuBackground (Graphics& g, int width, int height);

    /** Draws one of the items in a popup menu. */
    void drawPopupMenuItem (Graphics& g, const juce::Rectangle<int>& area,
                                           const bool isSeparator, const bool isActive,
                                           const bool isHighlighted, const bool isTicked,
                                           const bool hasSubMenu, const String& text,
                                           const String& shortcutKeyText,
                                           const Drawable* icon, const Colour* const textColourToUse);

    /** Returns the size and style of font to use in popup menus. */
    Font getPopupMenuFont();

    void drawPopupMenuUpDownArrow (Graphics& g,
                                           int width, int height,
                                           bool isScrollUpArrow);

    /** Finds the best size for an item in a popup menu. */
    void getIdealPopupMenuItemSize (const String& text,
                                            const bool isSeparator,
                                            int standardMenuItemHeight,
                                            int& idealWidth,
                                            int& idealHeight);

    int getMenuWindowFlags();

    void drawMenuBarBackground (Graphics& g, int width, int height,
                                        bool isMouseOverBar,
                                        MenuBarComponent& menuBar);

    int getMenuBarItemWidth (MenuBarComponent& menuBar, int itemIndex, const String& itemText);

    Font getMenuBarFont (MenuBarComponent& menuBar, int itemIndex, const String& itemText);

    void drawMenuBarItem (Graphics& g,
                                  int width, int height,
                                  int itemIndex,
                                  const String& itemText,
                                  bool isMouseOverItem,
                                  bool isMenuOpen,
                                  bool isMouseOverBar,
                                  MenuBarComponent& menuBar);

    //==============================================================================
    void drawComboBox (Graphics& g, int width, int height,
                               const bool isButtonDown,
                               int buttonX, int buttonY,
                               int buttonW, int buttonH,
                               ComboBox& box);

    Font getComboBoxFont (ComboBox& box);

    Label* createComboBoxTextBox (ComboBox& box);
	void positionComboBoxText (ComboBox& box, Label& label);

    
    #pragma mark TreeView

    void drawTreeviewPlusMinusBox (Graphics&, const juce::Rectangle<float>& area,
                                   Colour backgroundColour, bool isItemOpen, bool isMouseOver);    
    bool areLinesDrawnForTreeView (TreeView&);
    int getTreeViewIndentSize (TreeView&);

    void drawFileBrowserRow (Graphics& g, int width, int height,
                                            const String& filename, Image* icon,
                                            const String& fileSizeDescription,
                                            const String& fileTimeDescription,
                                            const bool isDirectory, const bool isItemSelected,
                             const int /*itemIndex*/, DirectoryContentsDisplayComponent& dcc);
    

    juce::Button* createDialButton (const bool isIncrement);
    ImageEffectFilter* getDialEffect();

    //==============================================================================
 
    Font getLabelFont (Label&);
    Font getTextButtonFont (TextButton&, int buttonHeight);
    

    //==============================================================================
    juce::Button* createFilenameComponentBrowseButton (const String& text);

    void layoutFilenameComponent (FilenameComponent& filenameComp,
                                          ComboBox* filenameBox, juce::Button* browseButton);

    //==============================================================================
    void drawCornerResizer (Graphics& g,
                                    int w, int h,
                                    bool isMouseOver,
                                    bool isMouseDragging);

    void drawResizableFrame (Graphics& g,
                                    int w, int h,
                                    const BorderSize<int>& borders);
    
    void fillResizableWindowBackground (Graphics& g,
                                    int w, int h,
                                        const BorderSize<int>& /*border*/, ResizableWindow& window);

    void drawResizableWindowBorder (Graphics& g,
                                            int w, int h,
                                            const BorderSize<int>& border,
                                            ResizableWindow& window);
	
    //==============================================================================
    void drawDocumentWindowTitleBar (DocumentWindow& window,
                                             Graphics& g, int w, int h,
                                             int titleSpaceX, int titleSpaceW,
                                             const Image* icon,
                                             bool drawTitleTextOnLeft);


    void positionDocumentWindowButtons (DocumentWindow& window,
                                                int titleBarX, int titleBarY,
                                                int titleBarW, int titleBarH,
                                                juce::Button* minimiseButton,
                                                juce::Button* maximiseButton,
                                                juce::Button* closeButton,
                                                bool positionTitleBarButtonsOnLeft);

    int getDefaultMenuBarHeight();

 
    //==============================================================================
    void drawStretchableLayoutResizerBar (Graphics& g,
                                                  int w, int h,
                                                  bool isVerticalBar,
                                                  bool isMouseOver,
                                                  bool isMouseDragging);

    //==============================================================================
    void drawGroupComponentOutline (Graphics& g, int w, int h,
                                            const String& text,
                                            const Justification& position,
                                            GroupComponent& group);

 


 
    //==============================================================================
    void paintToolbarBackground (Graphics& g, int width, int height, Toolbar& toolbar);

    juce::Button* createToolbarMissingItemsButton (Toolbar& toolbar);

    void paintToolbarButtonBackground (Graphics& g, int width, int height,
                                               bool isMouseOver, bool isMouseDown,
                                               ToolbarItemComponent& component);

    void paintToolbarButtonLabel (Graphics& g, int x, int y, int width, int height,
                                          const String& text, ToolbarItemComponent& component);

 
    //==============================================================================
    //==============================================================================
    /* AlertWindow handling..
    */
	
    AlertWindow* createAlertWindow (const String& title, const String& message,
                                                   const String& button1, const String& button2, const String& button3,
                                                   AlertWindow::AlertIconType iconType,
                                                   int numButtons, Component* associatedComponent);
    void drawAlertBox (Graphics& g, AlertWindow& alert,const juce::Rectangle<int>& textArea, TextLayout& textLayout);
    int getAlertBoxWindowFlags();
    int getAlertWindowButtonHeight();
    Font getAlertWindowMessageFont();
    Font getAlertWindowFont();

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
                               const int flair, const float sx, const float sy,
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
	void setBackgroundGradient(Graphics& g, Point<int> gStart, Point<int> gEnd);
 	
	void makeBackgroundImage(MLRect r);
	
	void drawBackground(Graphics& g, MLWidget* pW);
	void drawBackgroundRect(Graphics& g, MLWidget* pW, MLRect r);
	void drawBackgroundRectAtOffset(Graphics& g, MLWidget* pW, MLRect r, MLPoint xyOffset);
	void drawEntireBackground(Graphics& g, MLPoint offset);
	
	void drawUnitGrid(Graphics& g, MLWidget* pW);
	void drawUnitGridRect(Graphics& g, MLWidget* pW, MLRect r);
	void drawUnitGridRectAtOffset(Graphics& g, MLWidget* pW, MLRect r, MLPoint offset);

	void setGlobalTextScale(float s);
	void setGridUnitSize(float s);
	float getGridUnitSize();
	void setGridUnits(double gx, double gy);

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
                                        const int flair,
										const float sx, 
										const float sy) throw();

	void drawMLButtonShape  (Graphics& g,
                                        float x, float y, float w, float h,
                                        float maxCornerSize,
                                        const Colour& baseColour,
                                        const Colour& outlineColour,
                                        const float strokeWidth,
                                        const int flair,
										const float sx, 
										const float sy) throw();
							   
	void drawShadowLine  (Graphics& g,
                                        float ax, float ay, float bx, float by,
                                        const Colour& color,
                                        const int width);

	float calcMaxNumberWidth( const int digits, const int precision, const bool doSign, const MLValueDisplayMode mode);
	int getDigitsAfterDecimal (const float number, const int digits, const int precision)  throw();
	
	char* formatNumber (const float number, const int digits, const int precision, 
		const bool doSign, MLValueDisplayMode mode)  throw();

	void drawNumber (Graphics& g, const char* number, const int x, const int y, 
		const int w, const int h, const Colour& c, const Justification& j = Justification::topLeft)  throw();
			
	float getNumberWidth (const float number, const int digits, const int precision, const bool doSign);

	// NO
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
	// TODO no L+F class in future, these go into top level drawing Context 
	void addPicture(MLSymbol name, const void* data, size_t dataSize);	
	const Drawable* getPicture(MLSymbol name);
	
	std::unique_ptr<Image> mBackgroundImage;

	std::map<MLSymbol, DrawablePtr> mPictures;
	std::map<MLSymbol, void *> mPictureData;
	
	juce_DeclareSingleton_SingleThreaded_Minimal (MLLookAndFeel)
	
};

#endif // __ML_LOOKANDFEEL_H__