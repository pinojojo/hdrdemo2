#pragma once

#include <memory>

#include "ThreadSafeImage.hpp"
#include "ICamera.hpp"
#include "TripleBuffer.h"
#include "Frame.h"
#include "framerenderer.hpp"
#include "maskwindow.hpp"

class GlobalResourceManager
{
private:
    // Private constructor to prevent instantiation
    GlobalResourceManager()
        : image(new ThreadSafeImage(1024, 768, 3)),
          camera(nullptr),
          frameRenderer(nullptr)
    {
        tripleBuffer = std::make_unique<lzx::TripleBuffer<lzx::Frame>>();
        frameRenderer = FrameRenderer::instance(tripleBuffer.get());
        maskWindow = MaskWindow::instance();
    }

public:
    // Get the singleton instance of the GlobalResourceManager
    static GlobalResourceManager &getInstance()
    {
        static GlobalResourceManager instance;
        return instance;
    }

    // Global reosurces here
    std::unique_ptr<lzx::ICamera> camera;                        // The camera
    std::unique_ptr<ThreadSafeImage> image;                      // The image buffer (low latency mode)
    std::unique_ptr<lzx::TripleBuffer<lzx::Frame>> tripleBuffer; // The triple buffer (non low latency mode)
    FrameRenderer *frameRenderer;                                // The frame renderer
    MaskWindow *maskWindow;                                      // The mask window

    // Disable copy constructor and assignment operator
    GlobalResourceManager(const GlobalResourceManager &) = delete;
    GlobalResourceManager &operator=(const GlobalResourceManager &) = delete;

    GlobalResourceManager(const GlobalResourceManager &&) = delete;
    GlobalResourceManager &operator=(const GlobalResourceManager &&) = delete;
};
