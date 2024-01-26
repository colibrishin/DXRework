#include "common.hlsli"

#define PARAM_NUM_PARTICLE g_iParam[0].y

[numthreads(32, 32, 1)]
void cs_main(
  uint3 dispatchThreadId : SV_DispatchThreadID
)
{
  const uint flat_idx = dispatchThreadId.x + dispatchThreadId.y * 32;

  if (flat_idx >= PARAM_NUM_PARTICLE) { return; }

  InstanceElement instance = uavInstance.Load(flat_idx);

#define INST_DELTATIME  instance.fParam[0].x
#define INST_LIFE       instance.fParam[0].y
#define INST_VELOCITY   instance.vParam[0]
#define INST_LOCAL      instance.mParam[0]

  float4 curr_pos = GetTranslation(INST_LOCAL);
  curr_pos += INST_VELOCITY * INST_DELTATIME;
  INST_LOCAL = matrix
    (
     INST_LOCAL._11, INST_LOCAL._12, INST_LOCAL._13, curr_pos.x,
     INST_LOCAL._21, INST_LOCAL._22, INST_LOCAL._23, curr_pos.y,
     INST_LOCAL._31, INST_LOCAL._32, INST_LOCAL._33, curr_pos.z,
     INST_LOCAL._41, INST_LOCAL._42, INST_LOCAL._43, INST_LOCAL._44
    );
  INST_LIFE += INST_DELTATIME;

#undef INST_DELTATIME
#undef INST_LIFE
#undef INST_VELOCITY
#undef INST_LOCAL

  uavInstance[flat_idx] = instance;
}
