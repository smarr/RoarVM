//
//  RoarVMAbstractEvent.m
//  RoarVMOnIPad
//

#import "sq.h"
#import "RoarVMAbstractEvent.h"
#import "sqSqueakIPhoneApplication.h"
#import "sqSqueakIPhoneApplication+events.h"
#import "SqueakNoOGLIPhoneAppDelegate.h"

extern SqueakNoOGLIPhoneAppDelegate *gDelegateApp;



@implementation RoarVMAbstractEvent

@synthesize location, touches;

- (void ) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view {
  touches = recognizer.numberOfTouches;
  location = [recognizer locationInView: view];
}

- (void) processInto: (sqInputEvent*)evt {
}

+ (RoarVMAbstractEvent*) newFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view {
  RoarVMAbstractEvent* evt = [self new];
  [evt initFrom: recognizer view: view];
  return evt;
}

+ (void) enqueueFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view {
  [(sqSqueakIPhoneApplication *) gDelegateApp.squeakApplication enqueueRoarVMEventAndInterpolateMouseEvents: 
   [self newFrom: recognizer view: view]];
}
@end
