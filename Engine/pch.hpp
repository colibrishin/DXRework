// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for
// future builds. This also affects IntelliSense performance, including code
// completion and many code browsing features. However, files listed here are
// ALL re-compiled if any one of them is updated between builds. Do not add
// files here that you will be updating frequently as this negates the
// performance advantage.

#ifndef PCH_HPP
#define PCH_HPP

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

// add headers that you want to pre-compile here
#include <algorithm>
#include <atomic>
#include <exception>
#include <execution>
#include <fmod_common.h>
#include <functional>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>
#define WIN32_LEAN_AND_MEAN

#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>

#include <wrl/client.h>
#include "framework.h"

#include "Audio.h"
#define _USE_MATH_DEFINES

#include "BufferHelpers.h"
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DebugDraw.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PostProcess.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "StepTimer.hpp"
#include "VertexTypes.h"
#include "WICTextureLoader.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d2d1.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxcapi.h>
#include <dxgi1_3.h>

#include <fmod.hpp>
#include <fmod_common.h>

#pragma comment(lib, "fmod_vc.lib")

#include "egCommon.hpp"
#include "egDXCommon.h"
#include "egHelper.hpp"
#include "egSerialization.hpp"
#include "egType.hpp"

#include "egActor.hpp"
#include "egEntity.hpp"
#include "egManager.hpp"
#include "egRenderable.hpp"

#include "egResource.hpp"
#include "egLayer.hpp"
#include "egScene.hpp"
#include "egComponent.hpp"
#include "egObject.hpp"

#include "egCollision.h"
#include "egElastic.h"
#include "egFriction.h"
#include "egKinetic.h"
#include "egPhysics.h"

#include "egD3Device.hpp"
#include "egRenderPipeline.hpp"

#include "egIShader.hpp"
#include "egIStateController.hpp"
#include "egShader.hpp"
#include "egStateController.hpp"
#include "egVertexShaderInternal.hpp"

#endif // PCH_HPP
