Texture2D    srcTex : register(t0);
Texture2D    accumulateTex : register(t1);
Texture2D    motionTex : register(t2);
SamplerState srcSampler : register(s0);

cbuffer constrant : register(b0)
{
	float2 jitterOffset;
	float2 dynamicResScale;
	float2 screenSize;
	float  motionSensitivity;
	float  blendScale;
	float4 leftRect;
	float4 rightRect;
}

float4 GetColor(float2 coord)
{
	if (((coord.x > leftRect.x + 0.03f && coord.y > leftRect.y + 0.03f) && (coord.x < leftRect.z - 0.03f && coord.y < leftRect.w - 0.03f)) 
		|| ((coord.x > rightRect.x + 0.03f && coord.y > rightRect.y + 0.03f) && (coord.x < rightRect.z - 0.03f && coord.y < rightRect.w - 0.03f)))
		return float4(0, 0, 0, 0);

	float2 unJitteredUV = coord * dynamicResScale + jitterOffset;
	float2 motionVector = motionTex.Sample(srcSampler, unJitteredUV).xy * float2(0.5f, 1);
	float2 acCoord = coord + motionVector;
	float  weight = min(1.0f, 0.25f + (abs(motionVector.x) + abs(motionVector.y)) * 10.0f * motionSensitivity);
	float3 color = srcTex.Sample(srcSampler, unJitteredUV).xyz * weight + accumulateTex.Sample(srcSampler, acCoord).xyz * (1.0f - weight);

	return float4(color, 1.0f);
}

float4 main(in float4 position: SV_POSITION, 
	        in float2 texcoord: TEXCOORD0) :SV_TARGET
{
	return GetColor(texcoord);
}
