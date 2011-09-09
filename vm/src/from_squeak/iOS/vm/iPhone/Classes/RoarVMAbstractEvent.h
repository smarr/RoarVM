//
//  RoarVMAbstractEvent.h
//  RoarVMOnIPad
//

#import <Foundation/Foundation.h>
#import "sq.h"
#import "SqueakUIController.h"

typedef enum  {RoarVMEventLocationOffset, RoarVMEventLocationPrevious, RoarVMEventLocationAbsolute, RoarVMEventLocationRelative} RoarVMEventLocationType;

@interface RoarVMAbstractEvent : NSObject {
  int touches;
  int taps;
  CGPoint location;
}

+ (void) enqueueFrom: (UIGestureRecognizer*) recognizer controller: (SqueakUIController*) controller where: (RoarVMEventLocationType) where;
+ (RoarVMAbstractEvent*) newFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where;
- (void) initFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where;

- (void) processInto: (sqInputEvent*)evt;
+ (CGPoint) adjustLocation: (CGPoint) p size: (CGSize) s;
+ (CGPoint) leftmostLocationInView: (UIView*) view recognizer: (UIGestureRecognizer*) recognizer ;
- (CGPoint) computeLocationFrom: (UIGestureRecognizer*) recognizer view: (UIView*) view where: (RoarVMEventLocationType) where;
- (BOOL) resetsRelativeOffset;

@property (nonatomic, readonly) CGPoint location;
@property (nonatomic,assign) int touches;
@end