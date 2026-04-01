#ifndef XBRZ_SHADER_H
#define XBRZ_SHADER_H

#if RETRO_PLATFORM == RETRO_PS3

static const char *xbrz_vshader = 
"struct out_vertex {\n"
"    float4 position : POSITION;\n"
"    float4 color    : COLOR;\n"
"    float2 texCoord : TEXCOORD0;\n"
"};\n"
"uniform float4x4 modelViewProj;\n"
"out_vertex vmain(float4 position : POSITION, float4 color : COLOR, float2 texCoord : TEXCOORD0)\n"
"{\n"
"    out_vertex OUT;\n"
"    OUT.position = mul(modelViewProj, position);\n"
"    OUT.color = color;\n"
"    OUT.texCoord = texCoord;\n"
"    return OUT;\n"
"}\n";

static const char *xbrz_fshader = 
"struct out_vertex {\n"
"    float4 color    : COLOR;\n"
"    float2 texCoord : TEXCOORD0;\n"
"};\n"
"uniform sampler2D decal : TEXUNIT0;\n"
"uniform float2 texture_size;\n"

"float wd(half3 c1, half3 c2) {\n"
"    return dot(abs(c1 - c2), half3(0.21, 0.72, 0.07));\n"
"}\n"

"float4 fmain(in out_vertex VAR) : COLOR\n"
"{\n"
"    float2 ts = (texture_size.x > 1.0) ? texture_size : float2(424.0, 240.0);\n"
"    float2 ps = 1.0 / ts;\n"
"    float2 uv = VAR.texCoord;\n"
"    float2 f = frac(uv * ts);\n"
"    float2 pc = (floor(uv * ts) + 0.5) * ps;\n"
"    \n"
"    half3 e = tex2D(decal, pc).rgb;\n"
"    half3 b = tex2D(decal, pc + float2( 0, -ps.y)).rgb;\n"
"    half3 d = tex2D(decal, pc + float2(-ps.x,  0)).rgb;\n"
"    half3 f_ = tex2D(decal, pc + float2( ps.x,  0)).rgb;\n"
"    half3 h = tex2D(decal, pc + float2( 0,  ps.y)).rgb;\n"
"    half3 a = tex2D(decal, pc + float2(-ps.x, -ps.y)).rgb;\n"
"    half3 c = tex2D(decal, pc + float2( ps.x, -ps.y)).rgb;\n"
"    half3 g = tex2D(decal, pc + float2(-ps.x,  ps.y)).rgb;\n"
"    half3 i = tex2D(decal, pc + float2( ps.x,  ps.y)).rgb;\n"
"    float d_eb = wd(e, b); float d_ed = wd(e, d); \n"
"    float d_ef = wd(e, f_); float d_eh = wd(e, h); \n"
"    \n"
"    half3 res = e;\n"
"    float2 dist_c = abs(f - 0.5);\n"
"    float total_dist = dist_c.x + dist_c.y;\n"
"    if (wd(f_, h) < wd(e, i)) {\n"
"        half3 target = (d_ef <= d_eh) ? f_ : h;\n"
"        float blend = saturate((total_dist - 0.2) * 4.0);\n"
"        if (f.x > 0.5 && f.y > 0.5) res = lerp(res, target, blend * 0.85);\n"
"    }\n"
"    if (wd(d, b) < wd(e, a)) {\n"
"        half3 target = (d_ed <= d_eb) ? d : b;\n"
"        float blend = saturate((total_dist - 0.2) * 4.0);\n"
"        if (f.x < 0.5 && f.y < 0.5) res = lerp(res, target, blend * 0.85);\n"
"    }\n"
"    if (wd(b, f_) < wd(e, c)) {\n"
"        half3 target = (d_eb <= d_ef) ? b : f_;\n"
"        float blend = saturate((total_dist - 0.2) * 4.0);\n"
"        if (f.x > 0.5 && f.y < 0.5) res = lerp(res, target, blend * 0.85);\n"
"    }\n"
"    if (wd(h, d) < wd(e, g)) {\n"
"        half3 target = (d_eh <= d_ed) ? h : d;\n"
"        float blend = saturate((total_dist - 0.2) * 4.0);\n"
"        if (f.x < 0.5 && f.y > 0.5) res = lerp(res, target, blend * 0.85);\n"
"    }\n"

"    return float4(res, 1.0) * VAR.color;\n"
"}\n";

#endif // RETRO_PLATFORM == RETRO_PS3
#endif // XBRZ_SHADER_H