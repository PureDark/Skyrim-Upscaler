// xbr_ps.hlsl
// Upscale image using a modified xBR algorithm
// This shader upscales by arbitrary size, not just fixed
// multiples of the original resolution

#pragma pack_matrix(row_major)

Texture2D<float4> tex : register(t0);
Texture2D<float4> edge : register(t1);

static float2 screen = float2(0, 0);

float edge_weight(float2 norm_pix_pt, float2 norm_pix_dist, float2 scale)
{
	// only doing level 1 weight for now
	float2 corner = norm_pix_pt;
	float  l;
	l = dot(norm_pix_pt, scale);
	if (l < 0.5) {
		// compute lower right corner
		corner += norm_pix_dist;
		l = dot(corner, scale);
		if (l < 0.5) {
			// corner is fully inside edge
			return 1.0;
		} else {
			//float w = 0.5 - norm_pix_pt.x - norm_pix_pt.y;
			//w = w / (norm_pix_dist.x + norm_pix_dist.y);
			//w = clamp(w, 0.0, 1.0);
			//w = w * w;
			//w = 0.5 + 0.5 * w;
			return 0.5;  // 1.0 - w;
		}
		//return 1.0;
	} else {
		// compute upper left corner
		corner -= norm_pix_pt;
		l = dot(corner, scale);
		if (l < 0.5) {
			//float w = norm_pix_pt.x + norm_pix_pt.y - 0.5;
			//w = w / (norm_pix_dist.x + norm_pix_dist.y);
			//w = clamp(w, 0.0, 1.0);
			//w = w * w;
			//w = 0.5 + 0.5 * w;
			return 0.5;  // w;
		} else {
			// corner fully outside edge
			return 0.0;
		}
		//return 0.0;
	}
	//	return 0.5;
}

float4 main(in float4 position: SV_POSITION, 
	        in float2 texcoord: TEXCOORD0) :SV_TARGET
{
	if (screen.x == 0)
		tex.GetDimensions(screen.x, screen.y);
	float2 screen_point = texcoord * screen;
	float2 pixel_point = frac(screen_point);
	int3   tex_point = int3(screen_point, 0);
	int3   edge_point = int3(2.0 * screen_point, 0);
	;
	float2 pix_dist = 1.0 / screen;
	float2 norm_pix_dist;
	norm_pix_dist.x = ddx(texcoord.x);
	norm_pix_dist.y = ddy(texcoord.y);
	norm_pix_dist *= screen / 2.0;

	float2 norm_pix_pt;
	norm_pix_pt.x = (pixel_point.x >= 0.5) ? 1.0 - pixel_point.x : pixel_point.x;
	norm_pix_pt.y = (pixel_point.y >= 0.5) ? 1.0 - pixel_point.y : pixel_point.y;

	float4 orig_color = tex.Load(tex_point);
	float4 edge_color = edge.Load(edge_point);

	float2 scale = float2(1.0, 1.0);
	//scale.x = (F == G) ? 0.5 : 1.0;
	//scale.y = (C == H) ? 0.5 : 1.0;
	float weight = edge_weight(norm_pix_pt, norm_pix_dist, scale);

	//uint3 coord = uint3(pos.xy, 0);
	//return tex.Load(coord);
	float4 color = weight * edge_color + (1.0 - weight) * orig_color;
	//float4 xformed_color = mul(color_xform, color);
	//xformed_color = (weight == 0.5 && E != edge_color) ? float4(1.0, 0.0, 0.0, 1.0) : xformed_color;
	return color;
}
