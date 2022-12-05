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

float4 FastBlur(Texture2D tex, float2 st, float2 stepSize, float blur)
{
	float4 o;
	for (int i = -1; i < 2; i++)
		for (int j = -1; j < 2; j++)
			o += tex.SampleLevel(srcSampler, st + stepSize * float2(i, j) * blur, blur);
	return o / 9.0;
}

float4 main(in float4 position: SV_POSITION, 
	        in float2 texcoord: TEXCOORD0) :SV_TARGET
{
	const float2 kernelStepSize = float2(1.0, 1.0) / screenSize;

	float2 uv = texcoord;
	float2 uv_centered = uv - float2(0.5, 0.5);
	float2 temp = (float2((leftRect.x + leftRect.z) / 2, (leftRect.y + leftRect.w) / 2) - float2(0.5, 0.5));
	float2 uv_mouse_centered = (uv_centered - temp) * float2(2.0, 1.0);
	float2 uv_mouse_centered2 = (uv_centered - float2(-temp.x, temp.y)) * float2(2.0, 1.0);
	float4 outColor;
	if (uv.x < 0.5) {
		float r2 = dot(uv_mouse_centered, uv_mouse_centered);
		float blur = 18.0 * r2 * blurIntensity;
		outColor = FastBlur(srcTex, uv, kernelStepSize, blur);
	} else {
		float r2 = dot(uv_mouse_centered2, uv_mouse_centered2);
		float blur = 18.0 * r2 * blurIntensity;
		outColor = FastBlur(srcTex, uv, kernelStepSize, blur);
	}
	return outColor;
}
