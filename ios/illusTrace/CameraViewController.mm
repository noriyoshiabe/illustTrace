//
//  CameraViewController.mm
//  illusTrace
//
//  Created by abechan on 2016/06/30.
//  Copyright © 2016年 Noriyoshi Abe. All rights reserved.
//

#import "CameraViewController.h"
#import "Illustrace.h"
#import <AVFoundation/AVFoundation.h>

using namespace illustrace;

@interface CameraViewController () <AVCaptureVideoDataOutputSampleBufferDelegate> {
    AVCaptureDeviceInput *_videoInput;
    AVCaptureStillImageOutput *_stillImageOutput;
    AVCaptureVideoDataOutput *_videoDataOutput;
    AVCaptureSession *_session;
}
@property (weak, nonatomic) IBOutlet UIImageView *previewView;
@end

@implementation CameraViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    NSError *error = nil;
    _session = [[AVCaptureSession alloc] init];
    _session.sessionPreset = AVCaptureSessionPresetPhoto;
    
    AVCaptureDevice *camera = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    
    _videoInput = [[AVCaptureDeviceInput alloc] initWithDevice:camera error:&error];
    [_session addInput:_videoInput];
    
    _videoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [_session addOutput:_videoDataOutput];
    
    dispatch_queue_t queue = dispatch_queue_create("captureQueue", NULL);
    _videoDataOutput.alwaysDiscardsLateVideoFrames = YES;
    _videoDataOutput.videoSettings = @{(NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
    [_videoDataOutput setSampleBufferDelegate:self queue:queue];
    
    _stillImageOutput = [[AVCaptureStillImageOutput alloc] init];
    _stillImageOutput.outputSettings = @{(NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)};
    [_session addOutput:_stillImageOutput];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [_session startRunning];
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [_session stopRunning];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)takeButtonAction:(id)sender
{
    AVCaptureConnection *videoConnection = [_stillImageOutput connectionWithMediaType:AVMediaTypeVideo];
    if (videoConnection) {
        [_stillImageOutput captureStillImageAsynchronouslyFromConnection:videoConnection completionHandler:^(CMSampleBufferRef imageDataSampleBuffer, NSError *error) {
             if (imageDataSampleBuffer) {
                 CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(imageDataSampleBuffer);
                 CVPixelBufferLockBaseAddress(pixelBuffer, 0);
                 uint8_t *grayImage = (uint8_t *)CVPixelBufferGetBaseAddress(pixelBuffer);
                 
                 NSLog(@"#### %lu %lu", CVPixelBufferGetWidth(pixelBuffer), CVPixelBufferGetHeight(pixelBuffer));
                 
                 CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
             }
         }];
    }
}

#pragma mark AVCaptureVideoDataOutputSampleBufferDelegate

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    uint8_t *imageAdress = (uint8_t *)CVPixelBufferGetBaseAddress(pixelBuffer);
    
    // TODO scan
    
    uint8_t *baseAddress = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    size_t width = CVPixelBufferGetWidth(pixelBuffer);
    size_t height = CVPixelBufferGetHeight(pixelBuffer);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef bitmapContext = CGBitmapContextCreate(baseAddress, width, height, 8, bytesPerRow, colorSpace, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    
    CGImageRef cgImage = CGBitmapContextCreateImage(bitmapContext);
    UIImage *image = [UIImage imageWithCGImage:cgImage scale:1.0 orientation:UIImageOrientationRight];
    
    CGContextRelease(bitmapContext);
    CGColorSpaceRelease(colorSpace);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    CGImageRelease(cgImage);
    
    dispatch_async(dispatch_get_main_queue(), ^{
        _previewView.image = image;
    });
}

@end
