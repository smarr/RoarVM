//
//  RoarVMSwipeEvent.m
//  RoarVMOnIPad
//

#import "RoarVMSwipeEvent.h"
#import "sq.h"


@implementation RoarVMSwipeEvent

-(void) initFrom: (UISwipeGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where {
  [super initFrom: recognizer view: view where: where];
  direction = recognizer.direction;
}

- (void) processInto: (sqInputEvent*)evt {
  [super processInto: evt];
  // placeholder
}

@end
