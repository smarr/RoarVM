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
#import "RoarVMMouseDownEvent.h"
#import "RoarVMMouseMoveEvent.h"
#import "RoarVMSwipeEvent.h"


extern struct	VirtualMachine* interpreterProxy;
extern SqueakNoOGLIPhoneAppDelegate *gDelegateApp;
static	sqWindowEvent evt;

@implementation SqueakUIController



- (void) setupRecognizers {
  printf("VIEWDIDLOAD\n");
  /*
   Create and configure the four recognizers. Add each to the view as a gesture recognizer.
   */
  for (int taps = 1; taps <= 1 /* 2 just gives two tap events */; ++taps)
    for (int touches = 1;  touches <= 3;  ++touches) {
      UITapGestureRecognizer *tr;
       tr = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapFrom:)];
      tr.numberOfTapsRequired = taps;
      tr.numberOfTouchesRequired = touches;
      tr.delaysTouchesBegan = NO;  tr.delaysTouchesEnded = NO; tr.cancelsTouchesInView = NO;
      [self.view addGestureRecognizer:tr];
       tr.delegate = self;
    }
      
	for (int touches = 1;  touches <= 3;  ++touches) {
    UILongPressGestureRecognizer *lpr;
    lpr = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPressFrom:)];
    lpr.delaysTouchesBegan = NO; lpr.delaysTouchesEnded = NO; lpr.cancelsTouchesInView = NO;  lpr.numberOfTouchesRequired = touches;
    CFTimeInterval minSecs = 0.1; // if zero don't get taps, or long press changed
    lpr.minimumPressDuration = minSecs;
    [self.view addGestureRecognizer: lpr];
    lpr.delegate = self;
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
  // [RoarVMMouseUpEvent enqueueFrom: recognizer view: self.view ];
  [RoarVMMouseDownEvent enqueueFrom: recognizer view: self.view];
  
  // RoarVMMouseMoveEvent enqueFrom: recognizer view: self.view ];

  [RoarVMMouseUpEvent enqueueFrom: recognizer view: self.view ];
}



- (void)handleSwipeFrom:(UISwipeGestureRecognizer *)recognizer {
  [RoarVMSwipeEvent enqueueFrom: recognizer view: self.view ];
}




- (void)handleLongPressFrom:(UILongPressGestureRecognizer *)recognizer {
  switch ( recognizer.state ) {
    case UIGestureRecognizerStateBegan:   
      // [RoarVMMouseUpEvent enqueueFrom: recognizer view: self.view ];
      [RoarVMMouseDownEvent enqueueFrom: recognizer view: self.view]; 
      break;
    case UIGestureRecognizerStateChanged: [RoarVMMouseMoveEvent enqueueFrom: recognizer view: self.view]; break;
    case UIGestureRecognizerStateEnded:   [RoarVMMouseUpEvent   enqueueFrom: recognizer view: self.view]; break;
    default: break;
  }
 }


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

@end
