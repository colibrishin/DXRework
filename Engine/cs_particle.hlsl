#include "common.hlsli"

#define PARAM_NUM_PARTICLE g_iParam[0].y
#define PARAM_DT g_fParam[0].z

[numthreads(32, 32, 1)]
void cs_main(
  uint3 dispatchThreadId : SV_DispatchThreadID
)
{
  const uint flat_idx = dispatchThreadId.x + dispatchThreadId.y * 32;

  // todo: thread safety for global variable.
  if (flat_idx >= PARAM_NUM_PARTICLE) { return; }
    
  float dt = PARAM_DT;

#define INST_LIFE       uavInstance[flat_idx].fParam[0].y
#define INST_VELOCITY   uavInstance[flat_idx].vParam[0]
#define INST_LOCAL      uavInstance[flat_idx].mParam[0]

  float4 curr_pos = GetTranslation(INST_LOCAL);
  curr_pos += INST_VELOCITY * PARAM_DT;

  matrix mat = INST_LOCAL;

  mat._41 = curr_pos.x;
  mat._42 = curr_pos.y;
  mat._43 = curr_pos.z;

  INST_LOCAL = mat;
  INST_LIFE += PARAM_DT;

#undef INST_LIFE
#undef INST_VELOCITY
#undef INST_LOCAL
}
