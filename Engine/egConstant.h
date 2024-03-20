#pragma once

namespace Engine
{
  using DirectX::SimpleMath::Vector3;

  // Graphic Constants
  constexpr int  g_max_lights                   = 8;
  constexpr int  g_max_shadow_cascades          = 3;
  constexpr int  g_max_bone_count               = 4;
  constexpr int  g_max_shadow_map_size          = 512;
  constexpr int  g_max_reflect_refract_map_size = 512;
  constexpr UINT g_max_frame_latency_second     = 1;
  constexpr UINT g_max_frame_latency_ms         = g_max_frame_latency_second * 1000;
  constexpr UINT g_max_slot_per_texture         = 8;
  constexpr UINT g_max_slot_per_uav             = 2;
  constexpr UINT g_reserved_struct_buffer_slot  = 32;
  constexpr UINT g_reserved_bind_slot           = 48;
  constexpr UINT g_uav_slot_limit               = 8;
  constexpr UINT g_reserved_uav_slot            = 6;

  // Physics Constants
  constexpr float   g_epsilon                             = 0.001f;
  constexpr float   g_epsilon_squared                     = g_epsilon * g_epsilon;
  constexpr float   g_gravity_acc                         = 9.81f;
  constexpr float   g_fixed_update_interval               = 1.f / 128.f;
  constexpr Vector3 g_gravity_vec                         = Vector3(0.0f, -g_gravity_acc, 0.0f);
  constexpr float   g_restitution_coefficient             = 0.66f;
  constexpr float   g_drag_coefficient                    = 0.25f;
  constexpr size_t  g_gjk_max_iteration                   = 64;
  constexpr size_t  g_epa_max_iteration                   = 64;
  constexpr size_t  g_speculation_bisection_max_iteration = 64;
  constexpr bool    g_speculation_enabled                 = true;

  // Debug Constants
  constexpr int    g_debug_y_movement        = 15;
  constexpr int    g_debug_y_initial         = 0;
  constexpr float  g_debug_message_life_time = 1.0f;
  constexpr size_t g_debug_message_max       = 1000;
  constexpr bool   g_debug_observer          = false;

  // Misc
  constexpr LONG_PTR g_invalid_id               = -1;
  constexpr Vector3  g_forward                  = {0.f, 0.f, 1.f};
  constexpr Vector3  g_backward                 = {0.f, 0.f, -1.f};
  constexpr size_t   g_max_map_size             = 2048; // only in power of 2
  constexpr size_t   g_octree_negative_round_up = g_max_map_size / 2;

  static_assert(g_max_map_size % 2 == 0, "Map size should be in the power of 2");
}
