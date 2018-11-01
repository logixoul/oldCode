// adapted from https://www.shadertoy.com/view/MscGWS

#include "precompiled.h"
#if 0
#include "gpufft.h"
#include "shade.h"
#include "gpgpu.h"

#define GLSL(...) #__VA_ARGS__

static const string lib =
	/*"float tau = atan(1.0)*8.0;"
	"vec2 iResolution = vec2(texWidth, texHeight);"
	""
	"vec2 polar(float m, float a) {"
	"	return m*vec2(cos(a), sin(a));   "
	"}"
	""
	"vec2 cmul(vec2 a,vec2 b) {"
	"	return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);"
	"}";*/
	GLSL(
		vec2 iResolution = vec2(texWidth, texHeight);
		vec2 cmul (vec2 a,float b) { return mat2(a,-a.y,a.x) * vec2(cos(b),sin(b)); }
	);

// !!!!!!!!!!!!!! NOTE: in the orig, w was equal to h.
//                      i removed stuff like " - texHeight/2.0" from w
gl::TextureRef fft(gl::TextureRef& in) {
	globaldict["texWidth"] = in->getWidth();
	globaldict["texHeight"] = in->getHeight();
	/*auto horzFftd = shade2(in,
		"vec2 uv = gl_FragCoord.xy;"
		"float w = uv.x - texWidth / 2.0;"
		"vec2 xw = vec2(0);"
		"for(float n = 0.0;n < texWidth;n++) {"
		"	 float a = -(tau * w * n) / texWidth;"
		"	 vec2 xn = textureLod(tex, vec2(n, uv.y) / iResolution.xy, 0.0).xy;"
		"	 xn.y = 0.0;"
		"	 xw += cmul(xn, polar(1.0, a));"
		"}"
		"_out = vec3(xw,0.0);"
		, ShadeOpts().ifmt(GL_RGBA32F), lib);
	auto bidirFftd = shade2(horzFftd,
		"vec2 uv = gl_FragCoord.xy;"
		"float w = uv.y - texHeight / 2.0;"
		"vec2 xw = vec2(0);"
		"for(float n = 0.0;n < texHeight;n++) {"
		"    float a = -(tau * w * n) / texHeight;"
		"    vec2 xn = textureLod(tex, vec2(uv.x, n) / iResolution.xy, 0.0).xy;"
		"    xw += cmul(xn, polar(1.0, a));"
		"}"
		"_out = vec3(xw,0.0) / sqrt(texWidth * texHeight);"
		, ShadeOpts().ifmt(GL_RGBA32F), lib);*/
	static gl::TextureRef dummy;
	static string shader = GLSL(
			vec2 uv = gl_FragCoord.xy;
		    vec4 O = vec4(0.0);
    
			for(float n = 0.; n < texWidth; n++)  {
				vec2 xn = texture(tex, vec2(n+.5, uv.y) / iResolution.xy).xy,
					 yn = texture(tex2, vec2(uv.x, n+.5) / iResolution.xy).zw,
					 a = - 6.2831853 * (uv-.5 -texWidth/2.) * n/texWidth;
        
				O.zw += cmul(xn, a.x);
				O.xy += cmul(yn, a.y);
			}
			_out = O.rgb;
			);

	auto bidirFftd = shade2(in, dummy, shader, ShadeOpts().ifmt(GL_RGBA32F), lib);

	return shade2(bidirFftd,
		"_out = fetch3(tex, mod(tc + .5, vec2(1.0)));");
}
gl::TextureRef ifft(gl::TextureRef& in) {
	return gl::TextureRef();
}
#endif