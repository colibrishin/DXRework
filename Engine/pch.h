// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for
// future builds. This also affects IntelliSense performance, including code
// completion and many code browsing features. However, files listed here are
// ALL re-compiled if any one of them is updated between builds. Do not add
// files here that you will be updating frequently as this negates the
// performance advantage.

#ifndef ENGINE_PCH_H
#define ENGINE_PCH_H

#define NOMINMAX

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

// add headers that you want to pre-compile here
#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <execution>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <ranges>
#include <set>
#include <stack>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>
#define WIN32_LEAN_AND_MEAN

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include <wrl/client.h>

#include "Audio.h"
#include "framework.h"
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

#include "egActor.h"
#include "egCollision.h"
#include "egCommon.hpp"
#include "egComponent.h"
#include "egD3Device.hpp"
#include "egDXCommon.h"
#include "egElastic.h"
#include "egEntity.hpp"
#include "egFriction.h"
#include "egHelper.hpp"
#include "egKinetic.h"
#include "egLayer.h"
#include "egManager.hpp"
#include "egObject.hpp"
#include "egPhysics.hpp"
#include "egRenderPipeline.h"
#include "egRenderable.h"
#include "egResource.h"
#include "egScene.hpp"
#include "egSerialization.hpp"
#include "egShader.hpp"
#include "egStateController.hpp"
#include "egType.h"

#endif // ENGINE_PCH_H
