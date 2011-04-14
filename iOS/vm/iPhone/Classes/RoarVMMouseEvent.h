//
//  RoarVMMouseEvent.h
//  RoarVMOnIPad
//

#import <Foundation/Foundation.h>
#import "RoarVMAbstractEvent.h"

@interface RoarVMMouseEvent : RoarVMAbstractEvent {
  int buttonBits; // RedButtonBit YellowButtonBit BlueButtonBit
}




- (int) buttonBitsFor: (UIGestureRecognizer*) recognizer;

@property (nonatomic,assign) int buttonBits;

@end
