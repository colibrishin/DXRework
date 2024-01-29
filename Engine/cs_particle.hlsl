#include "common.hlsli"

#define PARAM_NUM_PARTICLE g_iParam[0].y
#define PARAM_DT g_fParam[0].z
#define PARAM_DURATION g_fParam[0].w

#define RANDOM_NUMBER g_iParam[1].z
#define RANDOM_TEX0 tex03
#define RANDOM_TEX1 tex04
#define RANDOM_TEX2 tex05

groupshared int global_lock = 0;

[numthreads(32, 32, 1)]
void cs_main(
  uint3 dispatchThreadId : SV_DispatchThreadID
)
{
    const uint flat_idx = dispatchThreadId.x + dispatchThreadId.y * 32;
    const uint total_particle_count = PARAM_NUM_PARTICLE;
    const float dt = PARAM_DT;
    const float duration = PARAM_DURATION;

    if (flat_idx >= total_particle_count)
    {
        return;
    }

#define INST_ACTIVE     uavInstance[flat_idx].iParam[0].x
#define INST_LIFE       uavInstance[flat_idx].fParam[0].x
#define INST_VELOCITY   uavInstance[flat_idx].vParam[0]
#define INST_LOCAL      uavInstance[flat_idx].mParam[0]

  // Skipping inactive particles
    if (INST_ACTIVE == 0)
    {
        return;
    }

  // disable particles that have exceeded their lifetime
    if (INST_LIFE > duration)
    {
    INST_ACTIVE = 0;
        return;
    }

    uint3 random_tex_size = uint3(500, 500, 0);
    const uint3 random_tex_idx = uint3(
    RANDOM_NUMBER % random_tex_size.x,
    RANDOM_NUMBER / random_tex_size.x, 0);

  // NOTE: I DON'T KNOW WHAT I AM DOING
    float4 r0 = RANDOM_TEX0.Load(random_tex_idx + int3(flat_idx, 0, 0));
    float4 r1 = RANDOM_TEX1.Load(random_tex_idx + int3(0, flat_idx, 0));
    float4 r2 = RANDOM_TEX2.Load(random_tex_idx + int3(flat_idx, flat_idx, 0));

    int4 flat_r0 = r0 * 255.f;
    int4 flat_r1 = r1 * 255.f;
    int4 flat_r2 = r2 * 255.f;
    
    const uint r0_v = flat_r0.x << 24 | flat_r0.y << 16 | flat_r0.z << 8 | ((flat_r0.w ^ flat_idx) & 0x000000FF);
    const uint r1_v = flat_r1.x << 24 | flat_r1.y << 16 | flat_r1.z << 8 | ((flat_r1.w ^ flat_idx) & 0x000000FF);
    const uint r2_v = flat_r2.x << 24 | flat_r2.y << 16 | flat_r2.z << 8 | ((flat_r2.w ^ flat_idx) & 0x000000FF);
    
    const uint rv = r0_v ^ r1_v ^ r2_v;
    const float r0_n = rv / (uint) 0xF0000000;
  // END OF I DON'T KNOW WHAT I AM DOING

    float4 curr_pos = GetTranslation(INST_LOCAL);

    const float3 w = float3(0.f, 1.f, 0.f);
    const float3 R = curr_pos;
    const float3 v = cross(w, R);
    const float3 a = INST_VELOCITY.xyz * r0_n * 5;
    const float4 velocity = float4(v * a, 0.f);

    curr_pos += velocity * dt;

    matrix mat = INST_LOCAL;

    mat._41 = curr_pos.x;
    mat._42 = curr_pos.y;
    mat._43 = curr_pos.z;

  INST_LOCAL = mat;
  INST_LIFE += dt;

#undef INST_LIFE
#undef INST_VELOCITY
#undef INST_LOCAL
}
