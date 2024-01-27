#include "common.hlsli"

#define PARAM_NUM_PARTICLE g_iParam[0].y
#define PARAM_DT g_fParam[0].z

groupshared int global_lock = 0;

[numthreads(32, 32, 1)]
void cs_main(
  uint3 dispatchThreadId : SV_DispatchThreadID
)
{
  const uint flat_idx = dispatchThreadId.x + dispatchThreadId.y * 32;

  int l = 0;
  do { InterlockedCompareExchange(global_lock, 0, 1, l); }
  while (l == 0);

  const uint  total_particle_count = PARAM_NUM_PARTICLE;
  const float dt                   = PARAM_DT;

  do { InterlockedCompareExchange(global_lock, 1, 0, l); }
  while (l == 1);

  if (flat_idx >= total_particle_count) { return; }

#define INST_LIFE       uavInstance[flat_idx].fParam[0].y
#define INST_VELOCITY   uavInstance[flat_idx].vParam[0]
#define INST_LOCAL      uavInstance[flat_idx].mParam[0]

  const float4 velocity = INST_VELOCITY;
  float4 curr_pos = GetTranslation(INST_LOCAL);
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
