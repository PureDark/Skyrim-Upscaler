Texture2D    srcTex : register(t0);
Texture2D    accumulateTex : register(t1);
Texture2D    motionTex : register(t2);
SamplerState srcSampler : register(s0);

cbuffer constrant : register(b0)
{
	float2 jitterOffset;
	float2 dynamicResScale;
	float2 screenSize;
	float  blurIntensity;
	float  blendScale;
	float4 leftRect;
	float4 rightRect;
}

float4 GetColor(float2 coord)
{
	//if (((coord.x > leftRect.x + 0.05f && coord.y > leftRect.y + 0.05f) && (coord.x < leftRect.z - 0.05f && coord.y < leftRect.w - 0.05f)) 
	//	|| ((coord.x > rightRect.x + 0.05f && coord.y > rightRect.y + 0.05f) && (coord.x < rightRect.z - 0.05f && coord.y < rightRect.w - 0.05f)))
	//	return float4(0, 0, 0, 0);

	//if (((coord.x < leftRect.x - 0.1f || coord.y < leftRect.y - 0.1f) && (coord.x > leftRect.z + 0.1f || coord.y > leftRect.w - 0.1f)) 
	//	|| ((coord.x < rightRect.x - 0.1f || coord.y < rightRect.y - 0.1f) && (coord.x > rightRect.z + 0.1f || coord.y > rightRect.w - 0.1f)))
	//	return float4(0, 0, 0, 0);

	const bool forceEnable = leftRect.x == 0 && leftRect.y == 0 && leftRect.z == 0 && leftRect.w == 0;

	if (!forceEnable) {
		float disX = min(abs(coord.x - leftRect.x), abs(coord.x - leftRect.z));
		float disY = min(abs(coord.y - leftRect.y), abs(coord.y - leftRect.w));

		if (min(disX, disY) > 0.05f)
			return float4(0, 0, 0, 0);
	}

	float2 unJitteredUV = coord * dynamicResScale + jitterOffset;
	float2 motionVector = motionTex.Sample(srcSampler, unJitteredUV).xy * float2(0.5f, 1);
	float2 acCoord = coord + motionVector;
	float  weight = 0.25f + max(motionVector.x, motionVector.y)*1.0f;
	float3 color = srcTex.Sample(srcSampler, unJitteredUV).xyz * weight + accumulateTex.Sample(srcSampler, acCoord).xyz * (1 - weight);

	return float4(color, 1.0f);
}

float4 main(in float4 position: SV_POSITION, 
	        in float2 texcoord: TEXCOORD0) :SV_TARGET
{
	return GetColor(texcoord);
}
