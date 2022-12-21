Texture2D    srcTex : register(t0);
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
float4 main(in float4 position: SV_POSITION, 
	        in float2 uv: TEXCOORD0) :SV_TARGET
{
	float2 realUV = uv * dynamicResScale;

	return float4(srcTex.Sample(srcSampler, realUV).xyz, 0.5f);
}
