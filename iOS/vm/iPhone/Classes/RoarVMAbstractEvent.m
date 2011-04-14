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



@implementation RoarVMAbstractEvent

@synthesize location, touches;

- (void ) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMAbstractEventLocationType) where {
  touches = recognizer.numberOfTouches;
  taps = recognizer.numberOfTapsRequired;
  static CGPoint lastLocation;
  lastLocation = location = 
    where == RoarVMEventReuseLocation  ?  lastLocation 
  : where == RoarVMEventUseLocation    ? [recognizer locationInView: view] 
  : where == RoarVMEventAdjustLocation ? [self adjustLocation: [recognizer locationInView: view] size: view.frame.size]
  : CGPointMake(0, 0);
}

- (void) processInto: (sqInputEvent*)evt {
}

+ (RoarVMAbstractEvent*) newFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMAbstractEventLocationType) where {
  RoarVMAbstractEvent* evt = [self new];
  [evt initFrom: recognizer view: view where: where];
  return evt;
}

+ (void) enqueueFrom: (UIGestureRecognizer*) recognizer controller: (SqueakUIController*) controller where: (RoarVMAbstractEventLocationType) where {
  RoarVMAbstractEvent* e = [self newFrom: recognizer view: controller.view where: where];
  controller.cursor.center = e.location;
  [(sqSqueakIPhoneApplication *) gDelegateApp.squeakApplication enqueueRoarVMEvent: e];
}

- (CGPoint) adjustLocation: (CGPoint) p size: (CGSize) s {
  static const float offsetUp = 100;
  
  return CGPointMake(p.x, s.height - p.y  >  offsetUp  ?  p.y - offsetUp  :  2 * p.y  -  s.height);
}
@end
