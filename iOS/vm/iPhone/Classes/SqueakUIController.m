//
//  SqueakUIController.m
//  SqueakNoOGLIPhone
//
//  Created by John M McIntosh on 6/8/08.SqueakNoOGLIPhoneAppDelegate.m: 	[[[self squeakApplication] eventQueue] addItem: data];

/*
Some of this code was funded via a grant from the European Smalltalk User Group (ESUG)
 Copyright (c) 2008 Corporate Smalltalk Consulting Ltd. All rights reserved.
 MIT License
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 
 The end-user documentation included with the redistribution, if any, must include the following acknowledgment: 
 "This product includes software developed by Corporate Smalltalk Consulting Ltd (http://www.smalltalkconsulting.com) 
 and its contributors", in the same place and form as other third-party acknowledgments. 
 Alternately, this acknowledgment may appear in the software itself, in the same form and location as other 
 such third-party acknowledgments.
 */

#import "SqueakNoOGLIPhoneAppDelegate.h"
#import "SqueakUIController.h"
#import "sqiPhoneScreenAndWindow.h"
#import "sq.h"

#import "sqSqueakIPhoneApplication.h"
#import "sqSqueakIPhoneApplication+events.h"
#import "RoarVMMouseUpEvent.h"
#import "RoarVMSwipeEvent.h"


extern struct	VirtualMachine* interpreterProxy;
extern SqueakNoOGLIPhoneAppDelegate *gDelegateApp;
static	sqWindowEvent evt;

@implementation SqueakUIController

@synthesize cursor;



- (void) setupRecognizers {

  for ( int touches = 1;  touches <= 1;   ++touches ) {
    UIGestureRecognizer *fewerTapRecognizer = nil;
    for ( int taps = 1;  taps <= 2;  ++taps) {
      UITapGestureRecognizer *r;
      r = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapFrom:)];
      r.numberOfTapsRequired = taps;  r.numberOfTouchesRequired = touches;
      r.delaysTouchesBegan = NO;  r.delaysTouchesEnded = NO; r.cancelsTouchesInView = NO;
      [self.view addGestureRecognizer:r];
      r.delegate = self;
      
      if (fewerTapRecognizer != nil) [fewerTapRecognizer requireGestureRecognizerToFail: r];
      fewerTapRecognizer = r;
    }
  }
    
  for ( int taps = 0; taps <= 0; ++taps) {
    for (int touches = 1;  touches <= sizeof(longRecognizers)/sizeof(longRecognizers[0]);  ++touches) {
      UILongPressGestureRecognizer *r;
      r = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPressFrom:)];
      r.delaysTouchesBegan = NO; r.delaysTouchesEnded = NO; r.cancelsTouchesInView = NO;  
      r.numberOfTouchesRequired = touches; r.numberOfTapsRequired = taps;
      CFTimeInterval minSecs = 0.1; // if zero don't get taps, or long press changed
      r.minimumPressDuration = minSecs;
      r.allowableMovement = 10000;
      [self.view addGestureRecognizer: r];
      longRecognizers[touches-1] = r;
      r.delegate = self;
    }
  }

  static int dirs[] = {
    UISwipeGestureRecognizerDirectionDown, 
    UISwipeGestureRecognizerDirectionLeft, 
    UISwipeGestureRecognizerDirectionRight, 
    UISwipeGestureRecognizerDirectionUp};
  
  for (int touches = 1;  touches <= 3;  ++touches)
    for (int i = 0;  i < sizeof(dirs)/sizeof(dirs[0]); ++i) {
      UISwipeGestureRecognizer *spr;
      spr = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(handleSwipeFrom:)];
      spr.direction = dirs[i];
      spr.delaysTouchesBegan = NO; spr.delaysTouchesEnded = NO; spr.cancelsTouchesInView = NO;
      spr.numberOfTouchesRequired = touches;
      [self.view addGestureRecognizer: spr];
       spr.delegate = self;
     }
}

static const char* stateString(UIGestureRecognizerState s) {
  switch (s) {
    case UIGestureRecognizerStatePossible: return  "Possible"; 
    case UIGestureRecognizerStateBegan: return  "Began"; 
    case UIGestureRecognizerStateChanged: return  "Changed"; 
    case UIGestureRecognizerStateRecognized: return  "Recognized"; 
    case UIGestureRecognizerStateCancelled: return  "Cancelled"; 
    case UIGestureRecognizerStateFailed: return  "Failed"; 
    default:  return "Unknown state";
  }
}


- (void)handleTapFrom:(UITapGestureRecognizer *)recognizer {
  [RoarVMMouseUpEvent enqueueFrom: recognizer controller: self where: RoarVMEventUseLocation];

  [RoarVMMouseEvent enqueueFrom: recognizer controller: self where: RoarVMEventUseLocation];
  
  [self performSelector: @selector(finishTapFrom:) withObject: recognizer afterDelay: 0.3];
}

- (void) finishTapFrom:(UITapGestureRecognizer *) recognizer {
  [RoarVMMouseUpEvent enqueueFrom: recognizer controller: self  where: RoarVMEventReuseLocation];
}



- (void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer {
  [RoarVMSwipeEvent enqueueFrom: recognizer controller: self   where: RoarVMEventUseLocation];
  [self startEnteringText];
}





- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (BOOL) hasText {return NO;}

- (void)startEnteringText
{
  [self becomeFirstResponder];
}

- (void) insertText:(NSString *)text {
  [(SqueakUIView*)self.view recordCharEvent: text];
}

- (void) deleteBackward {
  static NSString* backspace = nil;
  if (!backspace) {
    backspace = [NSString stringWithCString: "\b" encoding: NSASCIIStringEncoding];
    [backspace retain];
  }
  [self insertText: backspace];
}


- (void)handleLongPressFrom:(UILongPressGestureRecognizer *)recognizer {
  switch ( recognizer.state ) {
    case UIGestureRecognizerStateBegan:   
      [RoarVMMouseUpEvent enqueueFrom: recognizer controller: self where: RoarVMEventAdjustLocation];
      [RoarVMMouseEvent enqueueFrom: recognizer controller: self where: RoarVMEventAdjustLocation]; 
      break;
    case UIGestureRecognizerStateChanged: 
      [RoarVMMouseEvent enqueueFrom: recognizer controller: self where: RoarVMEventAdjustLocation]; 
      break;
    case UIGestureRecognizerStateEnded:   
      [RoarVMMouseUpEvent enqueueFrom: recognizer controller: self where: RoarVMEventAdjustLocation]; 
      //for (int i = 1;  i < recognizer.numberOfTouchesRequired; ++i)
      //  longRecognizers[i-1].enabled = YES;
      break;
    default: break;
  }
 }

# if 0
- (BOOL) gestureRecognizer: (UIGestureRecognizer*) r1 shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)r2 {
  if (![r1 isKindOfClass: [UILongPressGestureRecognizer class]]) return NO;
  if (![r2 isKindOfClass: [UILongPressGestureRecognizer class]]) return NO;
  return YES;
}

- (BOOL) gestureRecognizerShouldBegin: (UIGestureRecognizer*) r {
  if (![r isKindOfClass: [UILongPressGestureRecognizer class]]) return YES;
  UILongPressGestureRecognizer* lpr = (UILongPressGestureRecognizer*)r;
  for (int i = 1;  i < lpr.numberOfTouchesRequired; ++i)
    longRecognizers[i-1].enabled = NO;
  return YES;
}
# endif


// Subclasses override this method to define how the view they control will respond to device rotation 
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	//Called by Main Thread, beware of calling Squeak routines in Squeak Thread
	
	return YES;
}

- (void) pushEventToQueue {
	NSMutableArray* data = [NSMutableArray new];
	[data addObject: [NSNumber numberWithInteger: 8]];
	[data addObject: [NSData  dataWithBytes:(const void *) &evt length: sizeof(sqInputEvent)]];
	[[gDelegateApp.squeakApplication eventQueue]  addItem: data];
	[data release];	
}


- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
	[gDelegateApp zoomToOrientation: toInterfaceOrientation animated: YES];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	UIInterfaceOrientation o = [[UIApplication sharedApplication] statusBarOrientation];

/*	etoys rotate for keyboard! 
	if (UIInterfaceOrientationIsPortrait(o))
		[gDelegateApp.mainView becomeFirstResponder];
	else
		[gDelegateApp.mainView resignFirstResponder];
*/	
	
	CGRect mainScreenSize = [[UIScreen mainScreen] applicationFrame];
	CGRect f;

	f.origin.x = 0.0f;
	f.origin.y = 0.0f;
	f.size.width = UIInterfaceOrientationIsPortrait(o) ? mainScreenSize.size.width : mainScreenSize.size.height;
	f.size.height = UIInterfaceOrientationIsPortrait(o) ? mainScreenSize.size.height : mainScreenSize.size.width;
	evt.type = EventTypeWindow;
	evt.timeStamp = (int) ioMSecs();
	evt.action = WindowEventPaint;

	evt.value1 = (int) f.origin.x;
	evt.value2 = (int) f.origin.y;
	evt.value3 = (int) f.size.width;;
	evt.value4 = (int) f.size.height;
	evt.windowIndex = 1;

//	f.size.width *= 2.0;
//	f.size.height *= 2.0;
//	gDelegateApp.mainView.frame = f;
//	[gDelegateApp.scrollView sizeToFit];

	[self performSelector: @selector(pushEventToQueue) withObject: nil afterDelay: 1.0]; 

}

- (void) addCursor {
  cursor = [UIButton buttonWithType: UIButtonTypeRoundedRect];
  cursor.alpha = 0.66;
  cursor.frame = CGRectMake(100, 100, 20, 20);
  [self.view addSubview: cursor];
}

@end
