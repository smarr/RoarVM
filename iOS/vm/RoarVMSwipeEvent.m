//
//  RoarVMSwipeEvent.m
//  RoarVMOnIPad
//

#import "RoarVMSwipeEvent.h"
#import "sq.h"


@implementation RoarVMSwipeEvent

- (void) initFrom: (UISwipeGestureRecognizer*) recognizer view: (UIView*) view {
  [super initFrom: recognizer view: view];
  direction = recognizer.direction;
}

- (void) processInto: (sqInputEvent*)evt {
  [super processInto: evt];
  // placeholder
}

@end
