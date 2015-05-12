//
//  ViewController.m
//  TangramiOS
//
//  Created by Matt Blair on 8/25/14.
//  Copyright (c) 2014 Mapzen. All rights reserved.
//

#import "ViewController.h"
#include "tangram.h"

@interface ViewController () {
    
}
@property (strong, nonatomic) EAGLContext *context;
@property CGFloat pixelScale;
@property bool renderRequested;
@property NSURLSession* defaultSession;

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
    
    /* Setup NSURLSession configuration : cache path and size */

    NSURLSessionConfiguration *defaultConfigObject = [NSURLSessionConfiguration defaultSessionConfiguration];
    NSString *cachePath = @"/tile_cache";
    NSURLCache *tileCache = [[NSURLCache alloc] initWithMemoryCapacity: 4 * 1024 * 1024 diskCapacity: 30 * 1024 * 1024 diskPath: cachePath];
    defaultConfigObject.URLCache = tileCache;
    defaultConfigObject.requestCachePolicy = NSURLRequestUseProtocolCachePolicy;
    defaultConfigObject.timeoutIntervalForRequest = 30;
    defaultConfigObject.timeoutIntervalForResource = 60;

    /* create a default NSURLSession using the defaultConfigObject*/
    self.defaultSession = [NSURLSession sessionWithConfiguration: defaultConfigObject ];

    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    self.pixelScale = [[UIScreen mainScreen] scale];
    self.renderRequested = true;
    self.continuous = false;

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }

    setViewController(self);
    
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
    panRecognizer.maximumNumberOfTouches = 1;
    
    //4. Pinch
    UIPinchGestureRecognizer *pinchRecognizer = [[UIPinchGestureRecognizer alloc]
                                                 initWithTarget:self action:@selector(respondToPinchGesture:)];
    
    //5. Rotate
    UIRotationGestureRecognizer *rotationRecognizer = [[UIRotationGestureRecognizer alloc]
                                                        initWithTarget:self action:@selector(respondToRotationGesture:)];
    
    //6. Shove
    UIPanGestureRecognizer *shoveRecognizer = [[UIPanGestureRecognizer alloc]
                                             initWithTarget:self action:@selector(respondToShoveGesture:)];
    shoveRecognizer.minimumNumberOfTouches = 2;
    
    // Use the delegate method 'shouldRecognizeSimultaneouslyWithGestureRecognizer' for gestures that can be concurrent
    panRecognizer.delegate = self;
    pinchRecognizer.delegate = self;
    rotationRecognizer.delegate = self;
    
    /* Setup gesture recognizers */
    [self.view addGestureRecognizer:tapRecognizer];
    [self.view addGestureRecognizer:doubleTapRecognizer];
    [self.view addGestureRecognizer:panRecognizer];
    [self.view addGestureRecognizer:pinchRecognizer];
    [self.view addGestureRecognizer:rotationRecognizer];
    [self.view addGestureRecognizer:shoveRecognizer];
    
    [self setupGL];
    
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

- (void)respondToTapGesture:(UITapGestureRecognizer *)tapRecognizer {
    CGPoint location = [tapRecognizer locationInView:self.view];
    Tangram::handleTapGesture(location.x * self.pixelScale, location.y * self.pixelScale);
    [self renderOnce];
}

- (void)respondToDoubleTapGesture:(UITapGestureRecognizer *)doubleTapRecognizer {
    CGPoint location = [doubleTapRecognizer locationInView:self.view];
    Tangram::handleDoubleTapGesture(location.x * self.pixelScale, location.y * self.pixelScale);
    [self renderOnce];
}

- (void)respondToPanGesture:(UIPanGestureRecognizer *)panRecognizer {
    CGPoint displacement = [panRecognizer translationInView:self.view];
    [panRecognizer setTranslation:{0, 0} inView:self.view];
    CGPoint end = [panRecognizer locationInView:self.view];
    CGPoint start = {end.x - displacement.x, end.y - displacement.y};
    Tangram::handlePanGesture(start.x * self.pixelScale, start.y * self.pixelScale, end.x * self.pixelScale, end.y * self.pixelScale);
    [self renderOnce];
}

- (void)respondToPinchGesture:(UIPinchGestureRecognizer *)pinchRecognizer {
    CGPoint location = [pinchRecognizer locationInView:self.view];
    CGFloat scale = pinchRecognizer.scale;
    [pinchRecognizer setScale:1.0];
    Tangram::handlePinchGesture(location.x * self.pixelScale, location.y * self.pixelScale, scale);
    [self renderOnce];
}

- (void)respondToRotationGesture:(UIRotationGestureRecognizer *)rotationRecognizer {
    CGPoint position = [rotationRecognizer locationInView:self.view];
    CGFloat rotation = rotationRecognizer.rotation;
    [rotationRecognizer setRotation:0.0];
    Tangram::handleRotateGesture(position.x * self.pixelScale, position.y * self.pixelScale, rotation);
    [self renderOnce];
}

- (void)respondToShoveGesture:(UIPanGestureRecognizer *)shoveRecognizer {
    CGPoint displacement = [shoveRecognizer translationInView:self.view];
    [shoveRecognizer setTranslation:{0, 0} inView:self.view];
    Tangram::handleShoveGesture(displacement.y / self.view.bounds.size.height);
    [self renderOnce];
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

    Tangram::resize(width * self.pixelScale, height * self.pixelScale);

    Tangram::setPixelScale(self.pixelScale);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    Tangram::teardown();
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
    Tangram::resize(size.width * self.pixelScale, size.height * self.pixelScale);
}

- (void)renderOnce
{
    if (!self.continuous) {
        self.renderRequested = true;
        self.paused = false;
    }
}

- (void)cancelNetworkRequestWithUrl:(NSString *)url
{
    [_defaultSession getTasksWithCompletionHandler:^(NSArray* dataTasks, NSArray* uploadTasks, NSArray* downloadTasks) {
        for(NSURLSessionTask* _task in dataTasks) {
            if ([[_task originalRequest].URL.absoluteString isEqualToString:url]) {
                [_task cancel];
                break;
            }
        }
    }];
}

- (BOOL)networkRequestWithUrl:(NSString *)url TileID:(TileID)tileID DataSourceID:(NSNumber*)dataSourceID
{
    void (^handler)(NSData*, NSURLResponse*, NSError*) = ^void (NSData* data, NSURLResponse* response, NSError* error) {
        
        if(error == nil) {
            
            int dataLength = [data length];
            std::vector<char> rawDataVec;
            rawDataVec.resize(dataLength);
            memcpy(rawDataVec.data(), (char *)[data bytes], dataLength);
            networkDataBridge(rawDataVec, tileID, [dataSourceID intValue]);
            
        } else {
            
            logMsg("ERROR: response \"%s\" with error \"%s\".\n", response, error);
            
        }
        
    };

    NSURLSessionDataTask* dataTask = [_defaultSession dataTaskWithURL:[NSURL URLWithString:url]
                                                    completionHandler:handler];

    [dataTask resume];
    return true;
}

- (void)setContinuous:(bool)c
{
    _continuous = c;
    self.paused = !c;
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    Tangram::update([self timeSinceLastUpdate]);
    
    if (!self.continuous && !self.renderRequested) {
        self.paused = true;
    }
    self.renderRequested = false;
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    Tangram::render();
}

@end
