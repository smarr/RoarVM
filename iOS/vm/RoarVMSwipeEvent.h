//
//  RoarVMSwipeEvent.h
//  RoarVMOnIPad
//

#import <Foundation/Foundation.h>
#import "RoarVMAbstractMouseEvent.h"


@interface RoarVMSwipeEvent : RoarVMAbstractEvent {
  UISwipeGestureRecognizerDirection direction;
}

- (void) initFrom: (UISwipeGestureRecognizer*) recognizer view: (UIView*) view;



@end
