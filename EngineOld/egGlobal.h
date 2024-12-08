#pragma once

namespace Engine
{
	// Graphic Modifier
	inline std::atomic<float> g_fov           = DirectX::XM_PI / 4.f;
	inline std::atomic<bool>  g_full_screen   = false;
	inline std::atomic<bool>  g_vsync_enabled = true;
	inline std::atomic<UINT>  g_window_width  = 1920;
	inline std::atomic<UINT>  g_window_height = 1080;
	inline std::atomic<float> g_screen_near   = 1.f;
	inline std::atomic<float> g_screen_far    = 100.0f;
	inline std::atomic<UINT>  g_frame_buffer  = 2;
	inline std::atomic<bool>  g_raytracing    = false;

	// Debugging Modifier
	inline std::atomic<bool> g_paused      = false;
	inline std::atomic<bool> g_camera_lock = false;
	// Not thread safe!
	inline float g_camera_speed = 0.1f;
}
