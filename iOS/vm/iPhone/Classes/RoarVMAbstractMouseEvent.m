//
//  RoarVMAbstractMouseEvent.m
//  RoarVMOnIPad
//

#import "RoarVMAbstractMouseEvent.h"


@implementation RoarVMAbstractMouseEvent

- (void) processInto: (sqInputEvent*)evt {
  
  sqMouseEvent* me = (sqMouseEvent*)evt;
  me->type = EventTypeMouse;
  me->timeStamp = 0; // fix later
  me->x = lround(location.x);
  me->y = lround(location.y);
  me->buttons = [self buttonBit]; 
  me->modifiers = 0; // fix later
  me->reserved1 = 0; 
  me->windowIndex = 0; // is this right?

}

- (int) buttonBit { 
  switch (touches) {
    case 1: return RedButtonBit;
    case 2: return YellowButtonBit;
    case 3: return BlueButtonBit;
    default: return RedButtonBit;
  }
}


@end
