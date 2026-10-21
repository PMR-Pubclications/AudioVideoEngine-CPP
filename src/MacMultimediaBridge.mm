#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

// Objective-C delegate helper to receive AVFoundation sample buffers
@interface MacCameraDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
    std::function<void(const uint8_t*, size_t, uint64_t)> frameCallback;
}
- (id)initWithCallback:(std::function<void(const uint8_t*, size_t, uint64_t)>)callback;
@end

@implementation MacCameraDelegate
- (id)initWithCallback:(std::function<void(const uint8_t*, size_t, uint64_t)>)callback {
    if (self = [super init]) {
        frameCallback = callback;
    }
    return self;
}

- (void)captureOutput:(AVCaptureOutput *)output 
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer 
fromConnection:(AVCaptureConnection *)connection {
    
    // Extract pixel buffer from CMSampleBuffer
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer) return;

    // Lock base address for zero-copy memory access
    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    
    uint8_t *baseAddress = (uint8_t *)CVPixelBufferGetBaseAddress(imageBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    size_t height = CVPixelBufferGetHeight(imageBuffer);
    size_t totalSize = bytesPerRow * height;

    // Get timestamp
    CMTime timestamp = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    uint64_t timestampMs = (uint64_t)(CMTimeGetSeconds(timestamp) * 1000.0);

    // Pass straight to our pipeline callback
    if (frameCallback && baseAddress) {
        frameCallback(baseAddress, totalSize, timestampMs);
    }

    CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
}
@end

class MacMultimediaBridge {
private:
    AVCaptureSession *captureSession;
    MacCameraDelegate *cameraDelegate;
    dispatch_queue_t captureQueue;
    
    std::atomic<bool> isAuthenticated{false};
    const std::chrono::milliseconds authSampleInterval{333}; // 3 FPS throttle (~333ms)
    std::chrono::steady_clock::time_point lastSampleTime;

public:
    MacMultimediaBridge() {
        captureSession = nil;
        cameraDelegate = nil;
        captureQueue = nil;
    }

    ~MacMultimediaBridge() {
        shutdown();
    }

    bool initialize() {
        @autoreleasepool {
            captureSession = [[AVCaptureSession alloc] init];
            captureSession.sessionPreset = AVCaptureSessionPreset1280x720;

            // Select default video device (built-in or USB webcam)
            AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
            if (!device) {
                std::cerr << "[Mac Bridge] No video capture device found.\n";
                return false;
            }

            NSError *error = nil;
            AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
            if (!input || error) {
                std::cerr << "[Mac Bridge] Failed to create capture device input.\n";
                return false;
            }

            if ([captureSession canAddInput:input]) {
                [captureSession addInput:input];
            } else {
                return false;
            }

            // Set up video data output
            AVCaptureVideoDataOutput *videoOutput = [[AVCaptureVideoDataOutput alloc] init];
            videoOutput.videoSettings = @{(id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};

            // Create serial dispatch queue for frame callbacks
            captureQueue = dispatch_queue_create("com.trustshell.cameraQueue", DISPATCH_QUEUE_SERIAL);
            
            // Bind C++ lambda to the Objective-C delegate
            auto frameHandler = [this](const uint8_t* data, size_t size, uint64_t ts) {
                this->handleIncomingFrame(data, size, ts);
            };

            cameraDelegate = [[MacCameraDelegate alloc] initWithCallback:frameHandler];
            [videoOutput setSampleBufferDelegate:cameraDelegate queue:captureQueue];

            if ([captureSession canAddOutput:videoOutput]) {
                [captureSession addOutput:videoOutput];
            } else {
                return false;
            }

            [captureSession startRunning];
            lastSampleTime = std::chrono::steady_clock::now();
            std::cout << "[Mac Bridge] AVFoundation capture initialized successfully.\n";
            return true;
        }
    }

    void handleIncomingFrame(const uint8_t* dataPtr, size_t size, uint64_t timestamp) {
        auto now = std::chrono::steady_clock::now();

        if (!isAuthenticated) {
            // --- ENFORCE 3 FPS THROTTLE DURING AUTHENTICATION ---
            if (now - lastSampleTime >= authSampleInterval) {
                lastSampleTime = now;
                
                // TODO: Pass `dataPtr` and `size` directly into your C++ liveness & biometric engine.
                std::cout << "[Mac Auth] Processing 3 FPS frame sample (" << size << " bytes)...\n";
            }
        } else {
            // --- POST-AUTH: UNRESTRICTED FULL STREAM TO SHELL ---
            // Route raw pixel pointers directly to UI rendering pipeline.
        }
    }

    void grantSessionAccess() {
        isAuthenticated = true;
        std::cout << "[Mac Bridge] Authentication confirmed. Lifting 3 FPS throttle to full-rate stream.\n";
    }

    void shutdown() {
        @autoreleasepool {
            if (captureSession && [captureSession isRunning]) {
                [captureSession stopRunning];
            }
            captureSession = nil;
            cameraDelegate = nil;
            captureQueue = nil;
        }
    }
};
