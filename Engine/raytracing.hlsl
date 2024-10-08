#include "common.hlsli"

RWTexture2D<float4> g_output : register(u0);

RaytracingAccelerationStructure g_tlas : register(t0);

struct Payload
{
	float4 colorAndDist;
	bool   isShadow;
	bool   isReflect;
};

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
	const float2 mid_idx = index + 0.5f; // center of the pixel
	float2       ndc     = mid_idx / DispatchRaysDimensions().xy * 2.f - 1.f;
	ndc.y                = -ndc.y;

	float4 world = mul(float4(ndc, 0, 1), g_camInvVP); // unproject
	world /= world.w;

	origin    = mul(float4(0, 0, 0, 1), g_camWorld).xyz;
	direction = normalize(world.xyz - origin.xyz);
}

float3 RayPlaneIntersection(float3 planeOrigin, float3 planeNormal, float3 rayOrigin, float3 rayDirection)
{
	const float t = dot(-planeNormal, rayOrigin - planeOrigin) / dot(planeNormal, rayDirection);
	return rayOrigin + rayDirection * t;
}

float3 BarycentricCoordinates(float3 pt, float3 v0, float3 v1, float3 v2)
{
	float3 e0    = v1 - v0;
	float3 e1    = v2 - v0;
	float3 e2    = pt - v0;
	float  d00   = dot(e0, e0);
	float  d01   = dot(e0, e1);
	float  d11   = dot(e1, e1);
	float  d20   = dot(e2, e0);
	float  d21   = dot(e2, e1);
	float  denom = 1.0 / (d00 * d11 - d01 * d01);
	float  v     = (d11 * d20 - d01 * d21) * denom;
	float  w     = (d00 * d21 - d01 * d20) * denom;
	float  u     = 1.0 - v - w;
	return float3(u, v, w);
}

float4 CalculateSpecular(in float3 hitPosition, in float3 lightDir, in float3 normal, in float specularPower)
{
	const float3 reflected = normalize(reflect(lightDir, normal));
	return pow(saturate(dot(reflected, normalize(-WorldRayDirection()))), specularPower);
}

[shader("raygeneration")]
void raygen_main()
{
	// Calculate the ray direction in screen space
	uint2 dispatchRaysIndex      = DispatchRaysIndex().xy;
	uint2 dispatchRaysDimensions = DispatchRaysDimensions().xy;

	RayDesc ray;
	GenerateCameraRay(dispatchRaysIndex, ray.Origin, ray.Direction);
	ray.TMin = 0.001f;
	ray.TMax = 1000.0f;

	Payload payload;
	payload.isShadow  = false;
	payload.isReflect = false;

	TraceRay(g_tlas, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	g_output[dispatchRaysIndex.xy] = float4(payload.colorAndDist.rgb, 1.f);
}

StructuredBuffer<LightElement>    l_light : register(t0, space1);
StructuredBuffer<MaterialElement> l_material : register(t1, space1);
StructuredBuffer<ParamElement>    l_param : register(t2, space1);
StructuredBuffer<VertexInputType> l_vertex : register(t3, space1);
ByteAddressBuffer                 l_index : register(t4, space1);
Texture2D                         l_texture : register(t5, space1);
Texture2D                         l_normal : register(t6, space1);

[shader("closesthit")]
void closest_hit_main(inout Payload payload, Attributes attr)
{
	// todo: too much stride
	const uint p_index = PrimitiveIndex() * 4 * 3; // which primitive are we intersecting

	float3 barycentrics = float3
			(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	// Indices
	uint3 v_index;
	v_index.x = l_index.Load(p_index);
	v_index.y = l_index.Load(p_index + 4);
	v_index.z = l_index.Load(p_index + 8);

	// Vertices
	VertexInputType v_vertex[3];
	v_vertex[0] = l_vertex[v_index.x];
	v_vertex[1] = l_vertex[v_index.y];
	v_vertex[2] = l_vertex[v_index.z];

	// Barycentric interpolation of normal
	const float3 baryNormal = v_vertex[0].normal * barycentrics.x +
	                          v_vertex[1].normal * barycentrics.y +
	                          v_vertex[2].normal * barycentrics.z;

	const float3 baryTangent = v_vertex[0].tangent * barycentrics.x +
	                           v_vertex[1].tangent * barycentrics.y +
	                           v_vertex[2].tangent * barycentrics.z;

	const float3 baryBinormal = v_vertex[0].binormal * barycentrics.x +
	                            v_vertex[1].binormal * barycentrics.y +
	                            v_vertex[2].binormal * barycentrics.z;

	float4 baryColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	// Barycentric interpolation of coloring
	baryColor = v_vertex[0].color * barycentrics.x +
	            v_vertex[1].color * barycentrics.y +
	            v_vertex[2].color * barycentrics.z;

	// Barycentric interpolation of texture coordinates
	float2 baryUV =
			barycentrics.x * v_vertex[0].tex +
			barycentrics.y * v_vertex[1].tex +
			barycentrics.z * v_vertex[2].tex;

	const uint instanceId = InstanceIndex();

	// World position of the hit
	const float3 hitPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

	// https://wickedengine.net/2022/05/derivatives-in-compute-shader/comment-page-1/
	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/35f6060f2e1884c9807a965a265fa5c6b0326995/Samples/Desktop/D3D12Raytracing/src/D3D12RaytracingMiniEngineSample/DiffuseHitShaderLib.hlsl#L200-L231
	//
	// Compute the derivatives of the UV coordinates.

	// A plane with given vertices
	float3 nonbary_normal = normalize
			(
			 cross
			 (
			  v_vertex[2].position - v_vertex[0].position,
			  v_vertex[1].position - v_vertex[0].position
			 )
			);

	// Prepare rays for ddx(right) and ddy(down).
	const uint2 right = DispatchRaysIndex().xy + uint2(1, 0);
	const uint2 down  = DispatchRaysIndex().xy + uint2(0, 1);

	float3 ddxOrigin, ddxDir, ddyOrigin, ddyDir;
	GenerateCameraRay(right, ddxOrigin, ddxDir);
	GenerateCameraRay(down, ddyOrigin, ddyDir);

	// Test the intersection of the plane with the rays
	float3 xOffset = RayPlaneIntersection(hitPos, nonbary_normal, ddxOrigin, ddxDir);
	float3 yOffset = RayPlaneIntersection(hitPos, nonbary_normal, ddyOrigin, ddyDir);

	// Compute the barycentric coordinates of the intersection points
	float3 baryX = BarycentricCoordinates(xOffset, v_vertex[0].position, v_vertex[1].position, v_vertex[2].position);
	float3 baryY = BarycentricCoordinates(yOffset, v_vertex[0].position, v_vertex[1].position, v_vertex[2].position);

	// Compute the UV derivatives
	float3x2 uvMat  = float3x2(v_vertex[0].tex, v_vertex[1].tex, v_vertex[2].tex);
	float2   ddx_uv = mul(baryX, uvMat) - baryUV;
	float2   ddy_uv = mul(baryY, uvMat) - baryUV;

	if (l_material[0].bindFlag.texFlag[0].x)
	{
		// Sampling the texture with the gradient changes.
		baryColor.rgb = l_texture.SampleGrad(PSSampler, baryUV, ddx_uv, ddy_uv).rgb;
	}

	float3 bumpNormal = float3(0.f, 0.f, 0.f);

	if (l_material[0].bindFlag.texFlag[1].x)
	{
		float3 normalMap = l_normal.SampleGrad(PSSampler, baryUV, ddx_uv, ddy_uv).rgb;
		normalMap        = (normalMap * 2.0f) - 1.0f;

		bumpNormal = (normalMap.x * baryTangent) +
		             (normalMap.y * baryBinormal) +
		             (normalMap.z * baryNormal);
		bumpNormal = normalize(bumpNormal);
	}

	float lightIntensity[MAX_NUM_LIGHTS];
	float normalLightIntensity[MAX_NUM_LIGHTS];

	float4 normalColorArray[MAX_NUM_LIGHTS];
	float4 colorArray[MAX_NUM_LIGHTS];

	float4 specularColor = float4(0.f, 0.f, 0.f, 0.f);

	float shadow = 0.f;

	int i = 0;
	for (i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		if (i >= g_iParam[0].x)
		{
			break;
		}

		float4 lightPos = GetTranslation(l_light[i].world);
		lightPos.xyz /= lightPos.w;

		const float3 lightDir = normalize(lightPos.xyz - hitPos);
		lightIntensity[i]     = saturate(dot(baryNormal, lightDir));

		const float sphere = 4.f * PI * l_light[i].radius.x * l_light[i].radius.x;

		if (l_material[0].bindFlag.texFlag[1].x)
		{
			normalLightIntensity[i] = saturate(dot(bumpNormal, lightDir));
			normalColorArray[i]     = l_light[i].color * normalLightIntensity[i] / sphere;
		}

		colorArray[i] = l_light[i].color * lightIntensity[i] / sphere;

		{
			RayDesc shadowRay;
			shadowRay.Origin    = hitPos;
			shadowRay.Direction = lightDir;
			shadowRay.TMin      = 0.001f;
			shadowRay.TMax      = FLT_MAX;

			Payload shadowPayload =
			{
				0.f, 0.f, 0.f, 1.f,
				true,
				false
			};

			TraceRay
					(
					 g_tlas,
					 RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | // First hit
					 RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | // Skip closest hit
					 RAY_FLAG_CULL_BACK_FACING_TRIANGLES | // Cull back facing triangles
					 RAY_FLAG_FORCE_OPAQUE, // Also skip the hit shader, we want to know if the ray misses
					 0xFF,
					 0,
					 0,
					 0,
					 shadowRay,
					 shadowPayload
					);

			if (shadowPayload.colorAndDist.w == 1.f)
			{
				shadow += 1.f * lightIntensity[i];
			}

			if (l_material[0].specularPower > 0.f && shadowPayload.colorAndDist.w == 0.f)
			{
				const float4 specular = CalculateSpecular(hitPos, lightDir, baryNormal, l_material[0].specularPower);
				specularColor += specular * l_material[0].specularColor;
			}
		}
	}

	shadow = saturate(shadow);

	float4 colorSum       = g_ambientColor;
	float4 normalColorSum = float4(0.f, 0.f, 0.f, 0.f);

	for (i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		if (i >= g_iParam[0].x)
		{
			break;
		}

		normalColorSum.r += normalColorArray[i].r;
		normalColorSum.g += normalColorArray[i].g;
		normalColorSum.b += normalColorArray[i].b;
	}

	for (i = 0; i < MAX_NUM_LIGHTS; ++i)
	{
		if (i >= g_iParam[0].x)
		{
			break;
		}

		colorSum.r += colorArray[i].r;
		colorSum.g += colorArray[i].g;
		colorSum.b += colorArray[i].b;
	}

	float4 lightColor = saturate(colorSum);

	if (l_material[0].bindFlag.texFlag[1].x)
	{
		lightColor += saturate(normalColorSum);
	}

	if (l_material[0].specularPower > 0.f)
	{
		lightColor += saturate(specularColor);
	}

	float4 finalColor = (1.f - shadow) * saturate(lightColor) * baryColor;

	if (l_material[0].reflectionScale > 0.f && !payload.isReflect)
	{
		float4 reflectionColor = float4(0.f, 0.f, 0.f, 1.f);

		RayDesc reflectionRay;
		reflectionRay.Origin    = hitPos;
		reflectionRay.Direction = reflect(WorldRayDirection(), baryNormal);
		reflectionRay.TMin      = 0.001f;
		reflectionRay.TMax      = 1000.f;

		Payload reflectionPayload =
		{
			0.f, 0.f, 0.f, 1000.f,
			false,
			true
		};

		TraceRay
				(
				 g_tlas,
				 RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
				 0xff,
				 0,
				 0,
				 0,
				 reflectionRay,
				 reflectionPayload
				);

		reflectionColor.xyz = reflectionPayload.colorAndDist.xyz;
		finalColor          = float4(finalColor.xyz + (l_material[0].reflectionScale * reflectionColor), 1.f);
	}

	payload.colorAndDist.xyz = finalColor.xyz;
	payload.colorAndDist.w   = RayTCurrent();
}

[shader("miss")]
void miss_main(inout Payload payload)
{
	if (payload.isShadow)
	{
		payload.colorAndDist = float4(0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}

	payload.colorAndDist = float4(g_ambientColor.xyz, RayTCurrent());
}
