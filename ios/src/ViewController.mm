//
//  ViewController.m
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import "ViewController.h"

@interface ViewController () {
    
}
@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;
- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer;
- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer;
- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer;
- (void)respondToPinchGesture:(UIPanGestureRecognizer *)pinchRecognizer;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    /* Construct Gesture Recognizers */
    //1. Tap
    UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToTapGesture:)];
    tapRecognizer.numberOfTapsRequired = 1;
    // TODO: Figure a way to have a delay set for it not to tap gesture not to wait long enough for a doubletap gesture to be recognized
    tapRecognizer.delaysTouchesEnded = NO;
    
    //2. DoubleTap
    UITapGestureRecognizer *doubleTapRecognizer = [[UITapGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToDoubleTapGesture:)];
    doubleTapRecognizer.numberOfTapsRequired = 2;
    // Distanle single tap when double tap occurs
    [tapRecognizer requireGestureRecognizerToFail:doubleTapRecognizer];
    
    //3. Pan
    UIPanGestureRecognizer *panRecognizer = [[UIPanGestureRecognizer alloc]
                                            initWithTarget:self action:@selector(respondToPanGesture:)];
    
    //4. Pinch
    UIPinchGestureRecognizer *pinchRecognizer = [[UIPinchGestureRecognizer alloc]
                                                 initWithTarget:self action:@selector(respondToPinchGesture:)];
    
    //5. Rotate
    UIRotationGestureRecognizer *rotationRecognizer = [[UIRotationGestureRecognizer alloc]
                                                        initWithTarget:self action:@selector(respondToRotationGesture:)];
    
    /* Setup gesture recognizers */
    [self.view addGestureRecognizer:tapRecognizer];
    [self.view addGestureRecognizer:doubleTapRecognizer];
    [self.view addGestureRecognizer:panRecognizer];
    [self.view addGestureRecognizer:pinchRecognizer];
    [self.view addGestureRecognizer:rotationRecognizer];
    
    [self setupGL];
    
}

- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer {
    CGPoint location = [tapRecognizer locationInView:self.view];
    Tangram::handleTapGesture(location.x, location.y);
}

- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer {
    CGPoint location = [doubleTapRecognizer locationInView:self.view];
    Tangram::handleDoubleTapGesture(location.x, location.y);
}

- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer {
    CGPoint displacement = [panRecognizer translationInView:self.view];
    [panRecognizer setTranslation:{0, 0} inView:self.view];
    Tangram::handlePanGesture(displacement.x, displacement.y);
}

- (void)respondToPinchGesture:(UIPinchGestureRecognizer *)pinchRecognizer {
    CGPoint location = [pinchRecognizer locationInView:self.view];
    CGFloat scale = pinchRecognizer.scale;
    [pinchRecognizer setScale:1.0];
    Tangram::handlePinchGesture(location.x, location.y, scale);
}

- (void)respondToRotationGesture:(UIRotationGestureRecognizer *)rotationRecognizer {
    CGFloat rotation = rotationRecognizer.rotation;
    [rotationRecognizer setRotation:0.0];
    Tangram::handleRotateGesture(rotation);
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    Tangram::initialize();
    
    int width = self.view.bounds.size.width;
    int height = self.view.bounds.size.height;

    CGFloat scale = [[UIScreen mainScreen] scale];

    Tangram::resize(width * scale, height * scale);

    Tangram::setPixelScale(scale);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    Tangram::teardown();
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    CGFloat scale = [[UIScreen mainScreen] scale];

    Tangram::resize(size.width * scale, size.height * scale);
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    Tangram::update([self timeSinceLastUpdate]);
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    Tangram::render();
}

@end
