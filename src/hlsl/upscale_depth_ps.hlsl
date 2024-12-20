// ---- FNV Hash 11243098fc51979b

// ---- Created with 3Dmigoto v1.3.16 on Wed Dec 18 23:07:50 2024
Texture2D<float4> t0 : register(t0);

SamplerState s0_s : register(s0);

cbuffer cb12 : register(b12)
{
    float4 cb12[87];
}

// 3Dmigoto declarations
#define cmp -


void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float oDepth : SV_Depth)
{
    float4 r0, r1;
    uint4 bitmask, uiDest;
    float4 fDest;

    r0.x = cmp(v1.x >= 0.5);
    r0.y = r0.x ? 1.000000 : 0;
    r0.x = r0.x ? 2 : 1;
    r0.x = cb12[86].z * r0.x;
    r1.x = 0.5 * r0.x;
    r0.x = cb12[86].z * r0.y;
    r0.x = 0.5 * r0.x;
    r0.zw = cb12[85].xy * v1.xy;
    r0.y = 0;
    r0.xy = max(r0.zw, r0.xy);
    r1.y = cb12[85].y;
    r0.xy = min(r1.xy, r0.xy);
    r0.x = t0.Sample(s0_s, r0.xy).x;
    oDepth = r0.x;
    return;
}
