//
//  RoarVMAbstractEvent.m
//  RoarVMOnIPad
//

#import "sq.h"
#import "RoarVMAbstractEvent.h"
#import "sqSqueakIPhoneApplication.h"
#import "sqSqueakIPhoneApplication+events.h"
#import "SqueakNoOGLIPhoneAppDelegate.h"
#import "UIGestureRecognizer+RoarVMEvents.h"

extern SqueakNoOGLIPhoneAppDelegate *gDelegateApp;

static const char* typeString(RoarVMEventLocationType t) {
  switch (t) {
    case RoarVMEventLocationRelative: return "relative";
    case RoarVMEventLocationAbsolute: return "absolute";
    case RoarVMEventLocationPrevious: return "previous";
    case RoarVMEventLocationOffset:   return "offset";
  }
}


@implementation RoarVMAbstractEvent

@synthesize location, touches;

- (void ) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where {
  touches = recognizer.numberOfTouches;
  taps = recognizer.numberOfTapsRequired;
  location = [self computeLocationFrom: recognizer view: view where: where];
}

- (BOOL) resetsRelativeOffset { return NO; }

- (CGPoint) computeLocationFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where
{
  static BOOL inited = NO;
  static BOOL nextWillEstablishRelativeOffset = YES;
  
  static CGPoint lastLocation;
  static CGPoint prevTouchLocation;
  if (!inited) {
    inited = YES;
    lastLocation = CGPointMake(CGRectGetMidX(view.frame), CGRectGetMidY(view.frame));
  }
  CGPoint touchLocation = [[self class] leftmostLocationInView: view recognizer: recognizer];
  switch (where) {
    case RoarVMEventLocationPrevious:  
      break;
    case RoarVMEventLocationAbsolute:  
      lastLocation = touchLocation;
      break;
      
    case RoarVMEventLocationOffset:    
      lastLocation = [[self class] adjustLocation: touchLocation size: view.frame.size];
      break;
      
    case RoarVMEventLocationRelative: 
      if (!nextWillEstablishRelativeOffset) {
        lastLocation.x += touchLocation.x - prevTouchLocation.x;
        lastLocation.y += touchLocation.y - prevTouchLocation.y;
      }
      break;
  }
  prevTouchLocation = touchLocation;
  nextWillEstablishRelativeOffset = where != RoarVMEventLocationRelative  ||  [self resetsRelativeOffset];

  return lastLocation;
}


- (void) processInto: (sqInputEvent*)evt {
}


+ (RoarVMAbstractEvent*) newFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where {
  RoarVMAbstractEvent* evt = [self new];
  [evt initFrom: recognizer view: view where: where];
  return evt;
}

+ (void) enqueueFrom: (UIGestureRecognizer*) recognizer controller: (SqueakUIController*) controller where: (RoarVMEventLocationType) where {
  RoarVMAbstractEvent* e = [self newFrom: recognizer view: controller.view where: where];
  controller.cursor.center = e.location;
  [(sqSqueakIPhoneApplication *) gDelegateApp.squeakApplication enqueueRoarVMEvent: e];
}

+ (CGPoint) adjustLocation: (CGPoint) p size: (CGSize) s {
  static const float offsetUp = 100;
  
  return CGPointMake(p.x, s.height - p.y  >  offsetUp  ?  p.y - offsetUp  :  2 * p.y  -  s.height);
}


+ (CGPoint) leftmostLocationInView: (UIView*) view recognizer: (UIGestureRecognizer*) recognizer {
  
  CGPoint result = CGPointMake(INFINITY, INFINITY);
  NSUInteger nt = [recognizer numberOfTouches];
  for (NSUInteger i = 0; i < nt; ++i) {
    CGPoint p = [recognizer locationOfTouch:i inView: view];
    if (p.x < result.x) result = p;
  }
  return result;
}
@end
