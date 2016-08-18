//
//  TGMapViewController.h
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Updated by Matt Smollinger on 7/29/16.
//  Copyright (c) 2016 Mapzen. All rights reserved.
//

#include "tangram.h"

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@protocol TangramGestureRecognizerDelegate <NSObject>
@optional
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeSingleTap:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeDoubleTap:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizePanGesture:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizePinchGesture:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeRotationGesture:(CGPoint)location;
- (void)recognizer:(UIGestureRecognizer *)recognizer didRecognizeShoveGesture:(CGPoint)location;
@end

struct TileID;
@interface TGMapViewController : GLKViewController <UIGestureRecognizerDelegate>

@property (assign, nonatomic) Tangram::Map* map;
@property (assign, nonatomic) BOOL continuous;
@property (weak, nonatomic) id<TangramGestureRecognizerDelegate> gestureDelegate;

- (void)renderOnce;

@end
