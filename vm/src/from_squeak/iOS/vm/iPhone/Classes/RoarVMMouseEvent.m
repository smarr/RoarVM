//
//  RoarVMMouseEvent.m
//  RoarVMOnIPad
//

#import "RoarVMMouseEvent.h"


@implementation RoarVMMouseEvent

@synthesize buttonBits;

- (void) processInto: (sqInputEvent*)evt {
  
  sqMouseEvent* me = (sqMouseEvent*)evt;
  me->type = EventTypeMouse;
  me->timeStamp = 0; // fix later
  me->x = lround(location.x);
  me->y = lround(location.y);
  me->buttons = buttonBits; 
  me->modifiers = 0; // fix later
  me->reserved1 = 0; 
  me->windowIndex = 0; // is this right?

}


- (void) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where {
  [super initFrom: recognizer view: view where: where];
  buttonBits = [self buttonBitsFor: recognizer];
}

- (int) buttonBitsFor: (UIGestureRecognizer*) recognizer {
  if ( [recognizer isKindOfClass: [UITapGestureRecognizer class]]) {
    switch (touches) {
      case 1: return RedButtonBit;
      case 2: return YellowButtonBit;
      case 3: return BlueButtonBit;
      default: return 0;
    }
  }
  if ( [recognizer isKindOfClass: [UILongPressGestureRecognizer class]]) {
    switch (touches) {
      case 1: return 0;
      case 2: return RedButtonBit;
      case 3: return YellowButtonBit;
      case 4: return BlueButtonBit;
      default: return 0;
    }
  }
  return 0;
}



@end;
        
