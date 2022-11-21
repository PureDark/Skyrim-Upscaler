// xbr_edge_ps.hlsl
// Upscale image using a modified xBR algorithm
// This upscales the image by 2x2 and sets the 4 pixels
// pixels corresponding to the original to what the edge color should be
// if an edge is not detected, the original color is written

#pragma pack_matrix(row_major)

Texture2D<float4> tex : register(t0);

static const float3x3 yuv_xform = float3x3(0.299, 0.587, 0.144,
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081);

float dist(float4 a, float4 b)
{
	float3 rgb_delta;
	rgb_delta.r = abs(a.r - b.r);
	rgb_delta.g = abs(a.g - b.g);
	rgb_delta.b = abs(a.b - b.b);

	float3 yuv_delta = mul(yuv_xform, rgb_delta);
	return 48.0 * yuv_delta.r + 7.0 * yuv_delta.g + 6 * yuv_delta.b;
}

float4 edge_detect(float4 E, float4 B, float4 D, float4 C, float4 G, float4 F, float4 H,
	float4 F4, float4 I, float4 H5, float4 I4, float4 I5)
{
	float4 new_edge_color = (dist(E, F) <= dist(E, H)) ? F : H;

	float wd_red = dist(E, C) + dist(E, G) + dist(I, F4) + dist(I, H5) + 4.0 * dist(H, F);
	float wd_blue = dist(H, D) + dist(H, I5) + dist(F, I4) + dist(F, B) + 4.0 * dist(E, I);
	return (wd_red < wd_blue) ? new_edge_color : E;
}

float4 main(in float4 pos: SV_POSITION, 
	        in float2 texcoord: TEXCOORD0) :SV_TARGET
{
	int3 tex_point = int3(pos.xy, 0);
	int3 right_vec, down_vec;
	if (tex_point.x & 1) {
		if (tex_point.y & 1) {
			right_vec = int3(1, 0, 0);
			down_vec = int3(0, 1, 0);
		} else {
			right_vec = int3(0, -1, 0);
			down_vec = int3(1, 0, 0);
		}
	} else {
		if (tex_point.y & 1) {
			right_vec = int3(0, 1, 0);
			down_vec = int3(-1, 0, 0);
		} else {
			right_vec = int3(-1, 0, 0);
			down_vec = int3(0, -1, 0);
		}
	}
	tex_point = tex_point >> 1;

	float4 E = tex.Load(tex_point);
	float4 B = tex.Load(tex_point - down_vec);
	float4 D = tex.Load(tex_point - right_vec);
	float4 C = tex.Load(tex_point + right_vec - down_vec);
	float4 G = tex.Load(tex_point - right_vec + down_vec);
	float4 F = tex.Load(tex_point + right_vec);
	float4 H = tex.Load(tex_point + down_vec);
	float4 F4 = tex.Load(tex_point + 2 * right_vec);
	float4 I = tex.Load(tex_point + right_vec + down_vec);
	float4 H5 = tex.Load(tex_point + 2 * down_vec);
	float4 I4 = tex.Load(tex_point + 2 * right_vec + down_vec);
	float4 I5 = tex.Load(tex_point + right_vec + 2 * down_vec);

	float4 edge_color = edge_detect(E, B, D, C, G, F, H, F4, I, H5, I4, I5);
	return edge_color;
}
