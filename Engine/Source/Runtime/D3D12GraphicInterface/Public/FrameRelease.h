#pragma once
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "TypeLibrary.h"

namespace Engine
{
    // Frames the GPU may still run while the CPU records the next one. Per-frame copies (FrameUploadSlots) use this
    // many slots.
    constexpr uint64_t g_max_frames_in_flight = 2;

    // Index of the frame being recorded (frames presented so far).
    ENGINE_D3D12GRAPHICINTERFACE_API uint64_t RecordingFrame();
    // The frame finished on the GPU (true for frames too old to remember; false for the one being recorded).
    ENGINE_D3D12GRAPHICINTERFACE_API bool FrameDone(uint64_t frame);
    // Upload check (FrameUploadSlots): rewritten = a slot written twice in one frame; unfinished = a slot written while
    // the frame that wrote it before is still on the GPU. With KOLIBRI_FRAME_CHECK=1 an unfinished write ends the
    // process (__fastfail) after writing framecheck.txt; the counts, and the first rewritten names, are written there.
    ENGINE_D3D12GRAPHICINTERFACE_API void CountFrameUpload(bool rewritten, bool unfinished, const wchar_t* name);
    // Frames in flight in use: g_max_frames_in_flight, or 1 with KOLIBRI_FRAMES_IN_FLIGHT=1 (read once at startup).
    ENGINE_D3D12GRAPHICINTERFACE_API uint64_t FramesInFlight();

    // Calls holder, then destroys it, after the frame being recorded now has finished on the GPU, on the main thread at the
    // end of a later frame: for what lists of this frame may still read (a buffer being replaced, a descriptor block
    // given back). Without a running command task (before startup, after shutdown) holder runs right away.
    ENGINE_D3D12GRAPHICINTERFACE_API void ReleaseAfterFrame(std::function<void()>&& holder);

    // Keeps owned (a ComPtr, a unique_ptr, ...) alive until the frame being recorded now has finished on the GPU.
    template <typename T>
    void KeepUntilFrameDone(T&& owned)
    {
        if (!owned)
        {
            return;
        }
        ReleaseAfterFrame([held = std::make_shared<std::decay_t<T>>(std::forward<T>(owned))] {});
    }
}
