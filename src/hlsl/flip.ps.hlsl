Texture2D    srcTex;
SamplerState srcSampler;

cbuffer Jitter : register(b0)
{
	float4 jitterOffset;
}

float4 main(in float4 position: SV_POSITION, 
	        in float2 texcoord: TEXCOORD0) :SV_TARGET
{
	float2 unJitteredUV = texcoord + jitterOffset;
	return srcTex.Sample(srcSampler, unJitteredUV);
}
