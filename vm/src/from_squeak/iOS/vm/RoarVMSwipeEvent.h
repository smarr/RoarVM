//
//  RoarVMSwipeEvent.h
//  RoarVMOnIPad
//

#import <Foundation/Foundation.h>
#import "RoarVMMouseEvent.h"


@interface RoarVMSwipeEvent : RoarVMAbstractEvent {
  UISwipeGestureRecognizerDirection direction;
}



@end
