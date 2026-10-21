#include <android/image_reader.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <queue>
#include <atomic>

struct RealFrameData {
    uint64_t timestamp;
    uint8_t* dataPtr;     // Direct memory address from Android hardware buffer
    size_t dataSize;
    int width;
    int height;
};

class AndroidNativePipeline {
private:
    std::queue<RealFrameData> processingQueue;
    std::mutex queueMutex;
    std::atomic<bool> isRunning{true};
    std::atomic<bool> isAuthenticated{false};

    const std::chrono::milliseconds authSampleInterval{333}; // 3 FPS throttle
    std::chrono::steady_clock::time_point lastSampleTime = std::chrono::steady_clock::now();

    // Real processing worker replacing the mock
    void processingWorker() {
        while (isRunning) {
            if (!isAuthenticated) {
                RealFrameData frame;
                bool hasFrame = false;

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (!processingQueue.empty()) {
                        frame = std::move(processingQueue.front());
                        processingQueue.pop();
                        hasFrame = true;
                    }
                }

                if (hasFrame) {
                    // --- REAL PROCESSING HOOK ---
                    // Pass frame.dataPtr to your custom C++ feature extractor / liveness model here.
                    // e.g., analyze facial geometry, skin texture, or depth vectors.
                    
                    // Once your C++ math confirms valid multi-modal liveness:
                    // isAuthenticated = true;
                    
                    // Release the image buffer back to the Android image reader pool if required
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            } else {
                // Post-auth: Full speed streaming to Trust-Shell UI
                std::this_thread::sleep_for(std::chrono::milliseconds(33));
            }
        }
    }

public:
    std::thread workerThread;

    AndroidNativePipeline() {
        workerThread = std::thread(&AndroidNativePipeline::processingWorker, this);
    }

    ~AndroidNativePipeline() {
        isRunning = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    // THIS IS THE REAL NDK CALLBACK HOOK
    // Called directly by Android's AImageReader when a new hardware frame is ready
    void onImageAvailable(AImageReader* reader) {
        AImage* image = nullptr;
        media_status_t status = AImageReader_acquireLatestImage(reader, &image);
        if (status != AMEDIA_OK || image == nullptr) {
            return;
        }

        auto now = std::chrono::steady_clock::now();

        if (!isAuthenticated) {
            // Enforce 3 FPS throttle during authentication
            if (now - lastSampleTime >= authSampleInterval) {
                lastSampleTime = now;

                uint8_t* pixelData = nullptr;
                int datalength = 0;
                // Extract plane data directly from the native Android AImage structure
                AImage_getPlaneData(image, 0, &pixelData, &datalength);

                RealFrameData frame;
                frame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                frame.dataPtr = pixelData;
                frame.dataSize = datalength;

                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (processingQueue.size() > 3) {
                        processingQueue.pop(); // Drop stale frames
                    }
                    processingQueue.push(frame);
                }
            }
        } else {
            // Post-auth: Route AImage straight to texture renderers for the active session UI
        }

        // Always close the AImage to free the buffer back to the camera queue
        AImage_delete(image);
    }
};
