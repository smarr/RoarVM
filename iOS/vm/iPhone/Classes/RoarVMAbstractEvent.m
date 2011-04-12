//
//  RoarVMAbstractEvent.m
//  RoarVMOnIPad
//

#import "sq.h"
#import "RoarVMAbstractEvent.h"


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

@end
