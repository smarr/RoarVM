//
//  RoarVMAbstractEvent.h
//  RoarVMOnIPad
//

#import <Foundation/Foundation.h>
#import "sq.h"
#import "SqueakUIController.h"

typedef enum  {RoarVMEventAdjustLocation, RoarVMEventReuseLocation, RoarVMEventUseLocation} RoarVMAbstractEventLocationType;

@interface RoarVMAbstractEvent : NSObject {
  int touches;
  CGPoint location;
  

}

+ (void) enqueueFrom: (UIGestureRecognizer*) recognizer controller: (SqueakUIController*) controller where: (RoarVMAbstractEventLocationType) where;
+ (RoarVMAbstractEvent*) newFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMAbstractEventLocationType) where;
- (void) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMAbstractEventLocationType) where;

- (void) processInto: (sqInputEvent*)evt;
- (CGPoint) adjustLocation: (CGPoint) p size: (CGSize) s;

@property (nonatomic, readonly) CGPoint location;
@property (nonatomic,assign) int touches;
@end