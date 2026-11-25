#pragma once
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "TypeLibrary.h"

namespace Engine
{
    // Runs (destroys) holder after the frame being recorded now has finished on the GPU, on the main thread at the
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
