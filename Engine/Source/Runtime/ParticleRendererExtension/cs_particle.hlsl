#include "common.hlsli"

#define PARAM_NUM_PARTICLE bufLocalParam[0].iParam[0].x
#define PARAM_SCALING      bufLocalParam[0].iParam[0].y
#define RANDOM_NUMBER      bufLocalParam[0].iParam[0].z

#define PARAM_DURATION     bufLocalParam[0].fParam[0].x
#define PARAM_SIZE         bufLocalParam[0].fParam[0].y
#define PARAM_DT           bufLocalParam[0].fParam[0].z
#define PARAM_SCALING_MIN  bufLocalParam[0].fParam[0].w
#define PARAM_SCALING_MAX  bufLocalParam[0].fParam[1].x


#define RANDOM_TEX0 tex[0]
#define RANDOM_TEX1 tex[1]
#define RANDOM_TEX2 tex[2]

groupshared int global_lock = 0;

[numthreads(32, 32, 1)]
void cs_main(
	uint3 dispatchThreadId : SV_DispatchThreadID
)
{
	const uint  flat_idx             = dispatchThreadId.x + dispatchThreadId.y * 32;
	const uint  total_particle_count = PARAM_NUM_PARTICLE;
	const float dt                   = PARAM_DT;
	const float duration             = PARAM_DURATION;

	if (flat_idx >= total_particle_count)
	{
		return;
	}

	// Skipping inactive particles
	if (INST_PARTICLE_ACTIVE(flat_idx) == 0)
	{
		return;
	}

	// disable particles that have exceeded their lifetime
	if (INST_PARTICLE_LIFE(flat_idx) > duration)
	{
		INST_PARTICLE_ACTIVE(flat_idx) = 0;
		return;
	}

	const float life_normalized = INST_PARTICLE_LIFE(flat_idx) / duration;
	float4      curr_pos        = GetTranslation(INST_PARTICLE_WORLD(flat_idx));

	const float3  w = float3(0.f, 1.f, 0.f);
	const float3  r = curr_pos.xyz;
	const float3  v = cross(w, r);
	INST_PARTICLE_VELOCITY(flat_idx)  = float4(v, 0.f);

	curr_pos += float4(v * dt, 0.f);
	matrix mat = INST_PARTICLE_WORLD(flat_idx);

	if (PARAM_SCALING)
	{
		const float scale_normalized = lerp(PARAM_SCALING_MIN, PARAM_SCALING_MAX, life_normalized);

		mat._11 = scale_normalized;
		mat._22 = scale_normalized;
		mat._33 = scale_normalized;
	}

	mat._41 = curr_pos.x;
	mat._42 = curr_pos.y;
	mat._43 = curr_pos.z;

	INST_PARTICLE_WORLD(flat_idx) = mat;
	INST_PARTICLE_LIFE(flat_idx) += dt;
}
