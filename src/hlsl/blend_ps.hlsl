Texture2D    srcTex : register(t0);
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
float4 main(in float4 position: SV_POSITION, 
	        in float2 uv: TEXCOORD0) :SV_TARGET
{
	if ((uv.x > leftRect.x && uv.y > leftRect.y) && (uv.x < leftRect.z && uv.y < leftRect.w)) {
		float  blendFactorX = min(uv.x - leftRect.x, leftRect.z - uv.x) * 10 * blendScale;
		float  blendFactorY = min(uv.y - leftRect.y, leftRect.w - uv.y) * 10 * blendScale;
		float  blendFactor = min(1.0f, min(blendFactorX, blendFactorY));
		return float4(srcTex.Sample(srcSampler, uv).xyz, blendFactor);
	} else if ((uv.x > rightRect.x && uv.y > rightRect.y) && (uv.x < rightRect.z && uv.y < rightRect.w)) {
		float  blendFactorX = min(uv.x - rightRect.x, rightRect.z - uv.x) * 10 * blendScale;
		float  blendFactorY = min(uv.y - rightRect.y, rightRect.w - uv.y) * 10 * blendScale;
		float  blendFactor = min(1.0f, min(blendFactorX, blendFactorY));
		return float4(srcTex.Sample(srcSampler, uv).xyz, blendFactor);
	}

	return float4(0, 0, 0, 0);
}
