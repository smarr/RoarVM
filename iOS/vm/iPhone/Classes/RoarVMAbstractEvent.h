//
//  RoarVMAbstractEvent.h
//  RoarVMOnIPad
//

#import <Foundation/Foundation.h>
#import "sq.h"


@interface RoarVMAbstractEvent : NSObject {
  int touches;
  CGPoint location;
}
+ (RoarVMAbstractEvent*) newFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view;
- (void) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view;

- (void) processInto: (sqInputEvent*)evt;

@property (nonatomic,assign) CGPoint location;
@property (nonatomic,assign) int touches;
@end
