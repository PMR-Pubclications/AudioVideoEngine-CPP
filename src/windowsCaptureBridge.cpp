#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>

#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Ole32.lib")

class WindowsMultimediaBridge {
private:
    IMFSourceReader* sourceReader = nullptr;
    std::atomic<bool> isRunning{false};
    std::atomic<bool> isAuthenticated{false};
    
    // 3 FPS throttle interval for the authentication phase (~333ms)
    const std::chrono::milliseconds authSampleInterval{333};
    std::chrono::steady_clock::time_point lastSampleTime;

public:
    WindowsMultimediaBridge() = default;
    ~WindowsMultimediaBridge() {
        shutdown();
    }

    bool initialize() {
        // Initialize COM and Media Foundation
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) return false;

        hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) return false;

        // Create a media source activator for the first available webcam
        IMFAttributes* pAttributes = nullptr;
        hr = MFCreateAttributes(&pAttributes, 1);
        if (FAILED(hr)) return false;

        hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        
        IMFActivate** ppDevices = nullptr;
        UINT32 count = 0;
        hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
        pAttributes->Release();

        if (FAILED(hr) || count == 0) {
            std::cerr << "[Windows Bridge] No video capture devices found.\n";
            return false;
        }

        // Activate the first camera device
        IMFMediaSource* pSource = nullptr;
        hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
        
        // Clean up device list pointers
        for (UINT32 i = 0; i < count; ++i) { ppDevices[i]->Release(); }
        CoTaskMemFree(ppDevices);

        if (FAILED(hr)) return false;

        // Create the Source Reader to pull frames from the camera
        hr = MFCreateSourceReaderFromMediaSource(pSource, nullptr, &sourceReader);
        pSource->Release();

        if (FAILED(hr)) return false;

        isRunning = true;
        lastSampleTime = std::chrono::steady_clock::now();
        std::cout << "[Windows Bridge] Media Foundation capture initialized successfully.\n";
        return true;
    }

    // Main polling/capture loop running on a dedicated C++ thread
    void startCaptureLoop() {
        while (isRunning) {
            IMFSample* pSample = nullptr;
            DWORD streamIndex = 0;
            DWORD flags = 0;
            LONGLONG timestamp = 0;

            // Read the next sample from the Windows webcam pipeline
            HRESULT hr = sourceReader->ReadSample(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0, &streamIndex, &flags, &timestamp, &pSample
            );

            if (FAILED(hr) || !pSample) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Extract the raw buffer pointer from the Windows media sample
            IMFMediaBuffer* pBuffer = nullptr;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            if (SUCCEEDED(hr)) {
                BYTE* pAudioData = nullptr;
                DWORD currentLength = 0;
                hr = pBuffer->Lock(&pAudioData, nullptr, &currentLength);

                if (SUCCEEDED(hr)) {
                    auto now = std::chrono::steady_clock::now();

                    if (!isAuthenticated) {
                        // --- ENFORCE 3 FPS THROTTLE DURING AUTH ---
                        if (now - lastSampleTime >= authSampleInterval) {
                            lastSampleTime = now;
                            
                            // TODO: Pass `pAudioData` (pointer) and `currentLength` 
                            // directly into your C++ multi-modal liveness verification engine.
                            std::cout << "[Windows Auth] Processing 3 FPS frame sample (" << currentLength << " bytes)...\n";
                        }
                    } else {
                        // --- POST-AUTH: UNRESTRICTED FULL STREAM TO WINDOWS SHELL ---
                        // Stream raw pointers straight to the UI display / secure logging pipeline.
                    }

                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }
            pSample->Release();
        }
    }

    void grantSessionAccess() {
        isAuthenticated = true;
        std::cout << "[Windows Bridge] Authentication confirmed. Lifting 3 FPS throttle to full-rate stream.\n";
    }

    void shutdown() {
        isRunning = false;
        if (sourceReader) {
            sourceReader->Release();
            sourceReader = nullptr;
        }
        MFShutdown();
        CoUninitialize();
    }
};
