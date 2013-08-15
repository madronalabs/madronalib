
#import "MLEffect_UIView.h"


#pragma mark ____ LISTENER CALLBACK DISPATCHER ____

// This listener responds to parameter changes, gestures, and property notifications
void EventListenerDispatcher (void *inRefCon, void *inObject, const AudioUnitEvent *inEvent, UInt64 inHostTime, Float32 inValue)
{
	MLDemoEffect_UIView *SELF = (MLDemoEffect_UIView *)inRefCon;
	[SELF priv_eventListener:inObject event: inEvent value: inValue];
}

@implementation MLDemoEffect_UIView

-(void) awakeFromNib
{
	NSString *path = [[NSBundle bundleForClass: [MLDemoEffect_UIView class]] pathForImageResource: @"SectionPatternLight"];
	NSImage *pattern = [[NSImage alloc] initByReferencingFile: path];
	mBackgroundColor = [[NSColor colorWithPatternImage: [pattern autorelease]] retain];
}

#pragma mark ____ (INIT /) DEALLOC ____
- (void)dealloc
{
    [self priv_removeListeners];
	[mBackgroundColor release];
		
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	
    [super dealloc];
}

#pragma mark ____ PUBLIC FUNCTIONS ____

- (void)setAU:(AudioUnit)inAU
{
	// remove previous listeners
	if (mAU) 
		[self priv_removeListeners];
	
	mAU = inAU;
    
	// add new listeners
	[self priv_addListeners];
	
	// initial setup
	[self priv_synchronizeUIWithParameterValues];
}

- (void)drawRect:(NSRect)rect
{
	[mBackgroundColor set];
	NSRectFill(rect);		// this call is much faster than using NSBezierPath, but it doesn't handle non-opaque colors
	
	[super drawRect: rect];	// we call super to draw all other controls after we have filled the background
}

#pragma mark ____ INTERFACE ACTIONS ____

- (void) beginGesture:(NSNotification *) aNotification {

}

- (void) endGesture:(NSNotification *) aNotification {
}

void addParamListener (AUEventListenerRef listener, void* refCon, AudioUnitEvent *inEvent)
{
	inEvent->mEventType = kAudioUnitEvent_BeginParameterChangeGesture;
	verify_noerr ( AUEventListenerAddEventType(	listener, refCon, inEvent));
	
	inEvent->mEventType = kAudioUnitEvent_EndParameterChangeGesture;
	verify_noerr ( AUEventListenerAddEventType(	listener, refCon, inEvent));
	
	inEvent->mEventType = kAudioUnitEvent_ParameterValueChange;
	verify_noerr ( AUEventListenerAddEventType(	listener, refCon, inEvent));
}

#pragma mark ____ PRIVATE FUNCTIONS ____
- (void)priv_addListeners 
{
	if (mAU) {
		verify_noerr( AUEventListenerCreate(EventListenerDispatcher, self,
											CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0.05, 0.05, 
											&mAUEventListener));

	}
}

- (void)priv_removeListeners 
{
	if (mAUEventListener) verify_noerr (AUListenerDispose(mAUEventListener));
	mAUEventListener = NULL;
	mAU = NULL;
}


- (void)priv_synchronizeUIWithParameterValues
{

}

#pragma mark ____ LISTENER CALLBACK DISPATCHEE ____
// Handle kAudioUnitProperty_PresentPreset event
- (void)priv_eventListener:(void *) inObject event:(const AudioUnitEvent *)inEvent value:(Float32)inValue {
	switch (inEvent->mEventType) {
		case kAudioUnitEvent_ParameterValueChange:					// Parameter Changes
			break;
		case kAudioUnitEvent_BeginParameterChangeGesture:			// Begin gesture
			break;
		case kAudioUnitEvent_EndParameterChangeGesture:				// End gesture
			break;
		case kAudioUnitEvent_PropertyChange:						// custom property changed
			break;
	}
}

/* If we get a mouseDown, that means it was not in the graph view, or one of the text fields. 
   In this case, we should make the window the first responder. This will deselect our text fields if they are active. */
- (void) mouseDown: (NSEvent *) theEvent {
	[super mouseDown: theEvent];
	[[self window] makeFirstResponder: self];
}

- (BOOL) acceptsFirstResponder {
	return YES;
}

- (BOOL) becomeFirstResponder {	
	return YES;
}

- (BOOL) isOpaque {
	return YES;
}

//==============================================================================
- (void) keyDown: (NSEvent*) ev
{
//	NSString *keyStr = [ev charactersIgnoringModifiers];
//	unichar keyChar = [keyStr characterAtIndex:0];

	printf("keydown \n");
	
	
	[super keyDown: ev];
	
	
//	[[self nextResponder] keyDown: ev];

}





@end
