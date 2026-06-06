/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/render/PsyX_render.cpp
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include "platform/native_renderer_types.h"
#include <SDL3/SDL.h>

#include "platform/native_gpu.h"
#include "platform/native_glad.h"
#include "platform/native_log.h"
#include "platform/native_renderer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/native_win32.h"

__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#endif // def WIN32

#define VRAM_FORMAT          GL_RG
#define VRAM_INTERNAL_FORMAT GL_RG32F

extern SDL_Window *g_window;

#define NATIVE_RENDERER_LOG(fmt, ...)   Platform_Log("[CTR Renderer] " fmt, ##__VA_ARGS__)
#define NATIVE_RENDERER_WARN(fmt, ...)  Platform_LogWarn("[CTR Renderer] " fmt, ##__VA_ARGS__)
#define NATIVE_RENDERER_ERROR(fmt, ...) Platform_LogError("[CTR Renderer] [%s] - " fmt, __func__, ##__VA_ARGS__)

#define MAX_NUM_VERTEX_BUFFERS          (2)
#define PSX_SCREEN_ASPECT               (240.0f / 320.0f) // PSX screen is mapped always to this aspect

global_variable int s_previousBlendMode = BM_NONE;
global_variable int s_previousDepthMode = 0;
global_variable int s_previousStencilMode = 0;
global_variable int s_previousScissorState = 0;
global_variable int s_previousOffscreenState = 0;
global_variable RECT16 s_previousFramebuffer = {0, 0, 0, 0};
global_variable RECT16 s_previousOffscreen = {0, 0, 0, 0};

global_variable ShaderID s_previousShader = -1;

global_variable TextureID s_vramTexturesDouble[2];
global_variable TextureID s_vramTexture;
global_variable TextureID s_rgLutTexture = -1;
global_variable int s_vramTextureIndex = 0;

global_variable TextureID s_framebufferTexture = -1;
global_variable TextureID s_offscreenRenderTexture = -1;

global_variable TextureID s_whiteTexture = -1;
global_variable TextureID s_lastBoundTexture = -1;

TextureID NativeRenderer_GetVRAMTexture(void)
{
	return s_vramTexture;
}

TextureID NativeRenderer_GetWhiteTexture(void)
{
	return s_whiteTexture;
}

int g_windowWidth = 0;
int g_windowHeight = 0;

int g_dbg_wireframeMode = 0;
int g_dbg_texturelessMode = 0;

int g_cfg_bilinearFiltering = 0;

global_variable int s_vramNeedsUpdate = 1;
global_variable int s_framebufferNeedsUpdate = 0;

typedef struct
{
	GLenum fmt;
	GLuint *pbos;
	u64 num_pbos;
	u64 dx;
	u64 num_downloads;

	int width;
	int height;
	int nbytes; /* number of bytes in the pbo buffer. */
	u8 *pixels; /* the downloaded pixels. */
} GrPBO;

internal int PBO_Init(GrPBO *pbo, GLenum format, int w, int h, int num);
internal void PBO_Destroy(GrPBO *pbo);
internal void PBO_Download(GrPBO *pbo);
internal int NativeRenderer_InitialiseGLContext(char *windowName, int fullscreen);
internal int NativeRenderer_InitialiseGLExt(void);
internal void NativeRenderer_DestroyTexture(TextureID texture);
internal void NativeRenderer_SetScissorState(int enable);
internal void NativeRenderer_EnableDepth(int enable);
internal void NativeRenderer_SetViewPort(int x, int y, int width, int height);
internal void NativeRenderer_SetWireframe(int enable);
internal void NativeRenderer_BindVertexBuffer(void);

internal int PBO_Init(GrPBO *pbo, GLenum format, int w, int h, int num)
{
	if (pbo->pbos)
	{
		NATIVE_RENDERER_ERROR("Already initialized. Not necessary to initialize again; or shutdown first.");
		return -1;
	}

	if (0 >= num)
	{
		NATIVE_RENDERER_ERROR("Invalid number of PBOs: %d", num);
		return -2;
	}

	pbo->fmt = format;
	pbo->width = w;
	pbo->height = h;
	pbo->num_pbos = num;

	if (GL_RED == pbo->fmt || GL_GREEN == pbo->fmt || GL_BLUE == pbo->fmt)
	{
		pbo->nbytes = pbo->width * pbo->height;
	}
	else if (GL_RGB == pbo->fmt || GL_BGR == pbo->fmt)
	{
		pbo->nbytes = pbo->width * pbo->height * 3;
	}
	else if (GL_RGBA == pbo->fmt || GL_BGRA == pbo->fmt)
	{
		pbo->nbytes = pbo->width * pbo->height * 4;
	}
	else
	{
		NATIVE_RENDERER_ERROR("Unhandled pixel format, use GL_R, GL_RG, GL_RGB or GL_RGBA.");
		return -3;
	}

	if (pbo->nbytes == 0)
	{
		NATIVE_RENDERER_ERROR("Invalid width or height given: %d x %d", pbo->width, pbo->height);
		return -4;
	}

	pbo->pbos = (GLuint *)malloc(sizeof(GLuint) * num);
	pbo->pixels = (u8 *)malloc(pbo->nbytes);

	glGenBuffers(num, pbo->pbos);
	for (int i = 0; i < num; ++i)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->pbos[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, pbo->nbytes, NULL, GL_STREAM_READ);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	return 0;
}

internal void PBO_Destroy(GrPBO *pbo)
{
	if (pbo->pbos)
	{
		glDeleteBuffers(pbo->num_pbos, pbo->pbos);

		free(pbo->pbos);
		pbo->num_pbos = 0;
		pbo->pbos = NULL;
	}

	if (pbo->pixels)
	{
		free(pbo->pixels);
		pbo->pixels = NULL;
	}

	pbo->num_downloads = 0;
	pbo->dx = 0;
	pbo->fmt = 0;
	pbo->nbytes = 0;
}

internal void PBO_Download(GrPBO *pbo)
{
	u8 *ptr;

	if (pbo->num_downloads < pbo->num_pbos)
	{
		/*
		   First we need to make sure all our pbos are bound, so glMap/Unmap will
		   read from the oldest bound buffer first.
		*/
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->pbos[pbo->dx]);

		glGetTexImage(GL_TEXTURE_2D, 0, pbo->fmt, GL_UNSIGNED_BYTE, 0);
	}
	else
	{
		/* Read from the oldest bound pbo */
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo->pbos[pbo->dx]);

		ptr = (u8 *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (NULL != ptr)
		{
			memcpy(pbo->pixels, ptr, pbo->nbytes);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
		else
			NATIVE_RENDERER_WARN("Failed to map the buffer\n");

		/* Trigger the next read. */
		glGetTexImage(GL_TEXTURE_2D, 0, pbo->fmt, GL_UNSIGNED_BYTE, 0);
	}

	++pbo->dx;
	pbo->dx = pbo->dx % pbo->num_pbos;

	pbo->num_downloads++;

	if (pbo->num_downloads == UINT64_MAX)
		pbo->num_downloads = pbo->num_pbos;

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

global_variable GLuint s_glVertexArray[2];
global_variable GLuint s_glVertexBuffer[2];
global_variable int s_curVertexBuffer = 0;

global_variable GLuint s_glBlitFramebuffer;
global_variable GrPBO s_glFramebufferPBO;

global_variable GLuint s_glVramFramebuffer;

global_variable GLuint s_glOffscreenFramebuffer;
global_variable GrPBO s_glOffscreenPBO;


internal int NativeRenderer_InitialiseGLContext(char *windowName, int fullscreen)
{
	SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	if (fullscreen)
		windowFlags |= SDL_WINDOW_FULLSCREEN;

	g_window = SDL_CreateWindow(windowName, g_windowWidth, g_windowHeight, windowFlags);

	if (g_window == NULL)
	{
		NATIVE_RENDERER_ERROR("Failed to initialise SDL window!\n");
		return 0;
	}

	int major_version = 3;
	int minor_version = 3;
	int profile = SDL_GL_CONTEXT_PROFILE_CORE;

	// find best OpenGL version
	do
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_version);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_version);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);

		if (SDL_GL_CreateContext(g_window))
			break;

		minor_version--;

	} while (minor_version >= 0);

	if (minor_version == -1)
	{
		NATIVE_RENDERER_ERROR("Failed to initialise - OpenGL 3.x is not supported. Please update video drivers.\n");
		return 0;
	}

	return 1;
}

internal int NativeRenderer_InitialiseGLExt(void)
{
	GLenum err = gladLoadGL();

	if (err == 0)
		return 0;

	const char *rend = (const char *)glGetString(GL_RENDERER);
	const char *vendor = (const char *)glGetString(GL_VENDOR);
	NATIVE_RENDERER_LOG("*Video adapter: %s by %s\n", rend, vendor);

	const char *versionStr = (const char *)glGetString(GL_VERSION);
	NATIVE_RENDERER_LOG("*OpenGL version: %s\n", versionStr);

	const char *glslVersionStr = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	NATIVE_RENDERER_LOG("*GLSL version: %s\n", glslVersionStr);

	return 1;
}

int NativeRenderer_InitialiseRender(char *windowName, int width, int height, int fullscreen)
{
	g_windowWidth = width;
	g_windowHeight = height;

	// Due to debugging in fullscreen
	SDL_SetHint(SDL_HINT_WINDOW_ALLOW_TOPMOST, "0");
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

	if (!NativeRenderer_InitialiseGLContext(windowName, fullscreen))
	{
		NATIVE_RENDERER_ERROR("Failed to Initialise GL Context!\n");
		return 0;
	}

	if (!NativeRenderer_InitialiseGLExt())
	{
		NATIVE_RENDERER_ERROR("Failed to Intialise GL extensions\n");
		return 0;
	}

	return 1;
}

void NativeRenderer_Shutdown(void)
{
	glDeleteVertexArrays(2, s_glVertexArray);
	glDeleteBuffers(2, s_glVertexBuffer);

	PBO_Destroy(&s_glFramebufferPBO);
	PBO_Destroy(&s_glOffscreenPBO);

	glDeleteFramebuffers(1, &s_glBlitFramebuffer);
	glDeleteFramebuffers(1, &s_glOffscreenFramebuffer);
	glDeleteFramebuffers(1, &s_glVramFramebuffer);

	NativeRenderer_DestroyTexture(s_vramTexturesDouble[0]);
	NativeRenderer_DestroyTexture(s_vramTexturesDouble[1]);

	NativeRenderer_DestroyTexture(s_whiteTexture);
	NativeRenderer_DestroyTexture(s_rgLutTexture);
	NativeRenderer_DestroyTexture(s_framebufferTexture);
	NativeRenderer_DestroyTexture(s_offscreenRenderTexture);
}

void NativeRenderer_UpdateSwapIntervalState(int swapInterval)
{
	SDL_GL_SetSwapInterval(swapInterval);
}

void NativeRenderer_BeginScene(void)
{
	s_lastBoundTexture = 0;

	glClear(GL_STENCIL_BUFFER_BIT);

	NativeRenderer_UpdateVRAM();
	NativeRenderer_SetViewPort(0, 0, g_windowWidth, g_windowHeight);

	if (g_dbg_wireframeMode)
	{
		NativeRenderer_SetWireframe(1);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

void NativeRenderer_EndScene(void)
{
	s_framebufferNeedsUpdate = 1;

	if (g_dbg_wireframeMode)
		NativeRenderer_SetWireframe(0);

	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------

u16 vram[VRAM_WIDTH * VRAM_HEIGHT];
global_variable u8 rgLUT[LUT_WIDTH * LUT_HEIGHT * sizeof(u32)];

void NativeRenderer_ResetDevice(void)
{
	NativeRenderer_UpdateSwapIntervalState(0);
}

typedef struct
{
	// shader itself
	ShaderID shader;

	GLint projectionLoc;
	GLint bilinearFilterLoc;
	GLint texelSizeLoc;
	GLint texLoc;
	GLint lutLoc;
	GLint psxSemiTransPassLoc;
	GLint psxDrawMaskSetLoc;
} GTEShader;

internal int NativeRenderer_Shader_CheckShaderStatus(GLuint shader);
internal int NativeRenderer_Shader_CheckProgramStatus(GLuint program);
internal ShaderID NativeRenderer_Shader_Compile(const char *source, bool isPsxShader);
internal void NativeRenderer_GenerateCommonTextures(void);
internal TextureID NativeRenderer_CreateRGBATexture(int width, int height, u8 *data);
internal void NativeRenderer_CompilePSXShader(GTEShader *sh, const char *source);
internal void NativeRenderer_InitialisePSXShaders(void);
internal void NativeRenderer_InitRG8LUT(void);
internal void NativeRenderer_Ortho2D(float left, float right, float bottom, float top, float znear, float zfar);
internal void NativeRenderer_SetShader(const ShaderID shader);
internal void NativeRenderer_CopyRGBAFramebufferToVRAM(u32 *src, int x, int y, int w, int h, int update_vram, int flip_y);

global_variable GTEShader s_gteShader4;
global_variable GTEShader s_gteShader8;
global_variable GTEShader s_gteShader16;
global_variable GTEShader s_gteShader32Rgba;

GLint u_projectionLoc;
GLint u_bilinearFilterLoc;
GLint u_texelSizeLoc;
GLint u_psxSemiTransPassLoc;
GLint u_psxDrawMaskSetLoc;

#define GPU_SAMPLE_TEXTURE_4BIT_FUNC                                             \
	"	// returns 16 bit colour\n"                                                \
	"	vec2 samplePSX(vec2 tc) {\n"                                               \
	"		vec2 uv = (tc * vec2(0.25, 1.0) + v_page_clut.xy) * c_VRAMTexel;\n"       \
	"		vec2 comp = VRAM(uv);\n"                                                  \
	"		int index = int(fract(tc.x / 4.0 + 0.0001) * 4.0);\n"                     \
	"		float v = _idx2(comp, index / 2) * (255.0 / 16.0);\n"                     \
	"		float f = floor(v + 0.001);\n"                                            \
	"		vec2 c = vec2((v - f) * 16.0, f);\n"                                      \
	"		vec2 clut_pos = v_page_clut.zw;\n"                                        \
	"		clut_pos.x += mix(c[0], c[1], mod(float(index), 2.0)) * c_VRAMTexel.x;\n" \
	"		return VRAM(clut_pos);\n"                                                 \
	"	}\n"

#define GPU_SAMPLE_TEXTURE_8BIT_FUNC                                      \
	"	// returns 16 bit colour\n"                                         \
	"	vec2 samplePSX(vec2 tc) {\n"                                        \
	"		vec2 uv = (tc * vec2(0.5, 1.0) + v_page_clut.xy) * c_VRAMTexel;\n" \
	"		vec2 comp = VRAM(uv);\n"                                           \
	"		vec2 clut_pos = v_page_clut.zw;\n"                                 \
	"		int index = int(mod(tc.x, 2.0));\n"                                \
	"		clut_pos.x += _idx2(comp, index) * 255.0 * c_VRAMTexel.x;\n"       \
	"		return VRAM(clut_pos);\n"                                          \
	"	}\n"

#define GPU_SAMPLE_TEXTURE_16BIT_FUNC                    \
	"	vec2 samplePSX(vec2 tc) {\n"                       \
	"		vec2 uv = (tc + v_page_clut.xy) * c_VRAMTexel;\n" \
	"		return VRAM(uv);\n"                               \
	"	}\n"

#define GPU_FETCH_VRAM_FUNC                                        \
	"	const vec2 c_VRAMTexel = vec2(1.0 / 1024.0, 1.0 / 512.0);\n" \
	"	uniform sampler2D s_texture;\n"                              \
	"	vec2 VRAM(vec2 uv) { return texture2D(s_texture, uv).rg; }\n"

#define GPU_STP_PASS_FUNC                                          \
	"	float stpWeight(vec2 rg) { return step(0.5, rg.y); }\n"      \
	"	bool discardForSemiTransPass(vec2 rg, float stp) {\n"        \
	"		if(rg.x + rg.y == 0.0) { return true; }\n"                  \
	"		if(psxSemiTransPass == 1 && stp >= 0.5) { return true; }\n" \
	"		if(psxSemiTransPass == 2 && stp < 0.5) { return true; }\n"  \
	"		return false;\n"                                            \
	"	}\n"

#define GPU_DITHERING                                             \
	"	const mat4 c_dither = mat4(\n"                              \
	"		-4.0,  +0.0,  -3.0,  +1.0,\n"                              \
	"		+2.0,  -2.0,  +3.0,  -1.0,\n"                              \
	"		-3.0,  +1.0,  -4.0,  +0.0,\n"                              \
	"		+3.0,  -1.0,  +2.0,  -2.0) / 255.0;\n"                     \
	"	vec4 dither(vec4 color) {\n"                                \
	"		ivec2 dc = ivec2(fract(gl_FragCoord.xy / 4.0) * 4.0);\n"   \
	"		color.xyz += vec3(c_dither[dc.x][dc.y] * v_texcoord.w);\n" \
	"		return color;\n"                                           \
	"	}\n"

#define GPU_ARRAY_FUNC "	float _idx2(vec2 array, int idx) { return array[idx]; }\n"

#define GPU_FRAGMENT_SAMPLE_SHADER(bit)                                                                                                               \
	GPU_FETCH_VRAM_FUNC                                                                                                                               \
	GPU_ARRAY_FUNC                                                                                                                                    \
	GPU_SAMPLE_TEXTURE_##bit##BIT_FUNC                                                                                                                \
	    "	uniform sampler2D s_rgLut;\n"                                                                                                               \
	    "	uniform int bilinearFilter;\n"                                                                                                              \
	    "	uniform int psxSemiTransPass;\n"                                                                                                            \
	    "	uniform int psxDrawMaskSet;\n"                                                                                                              \
	    "	const vec2 c_LUTTexel = vec2(1.0 / 256.0, 1.0 / 256.0);\n"                                                                                  \
	    "	vec4 lut(vec2 rg) { return texture2D(s_rgLut, rg - c_LUTTexel * 0.0001); }\n" GPU_STP_PASS_FUNC "	vec4 bilinearTextureSample(vec2 P) {\n" \
	    "		vec2 frac = fract(P);\n"                                                                                                                   \
	    "		vec2 pixel = floor(P);\n"                                                                                                                  \
	    "		vec2 C11 = samplePSX(pixel);\n"                                                                                                            \
	    "		vec2 C21 = samplePSX(pixel + vec2(1.0, 0.0));\n"                                                                                           \
	    "		vec2 C12 = samplePSX(pixel + vec2(0.0, 1.0));\n"                                                                                           \
	    "		vec2 C22 = samplePSX(pixel + vec2(1.0, 1.0));\n"                                                                                           \
	    "		float ax1 = mix(float(C11.r + C11.g > 0.0), float(C21.r + C21.g > 0.0), frac.x);\n"                                                        \
	    "		float ax2 = mix(float(C12.r + C12.g > 0.0), float(C22.r + C22.g > 0.0), frac.x);\n"                                                        \
	    "		float axm = mix(ax1, ax2, frac.y);\n"                                                                                                      \
	    "		if(axm < 0.5) { discard; }\n"                                                                                                              \
	    "		float sx1 = mix(stpWeight(C11), stpWeight(C21), frac.x);\n"                                                                                \
	    "		float sx2 = mix(stpWeight(C12), stpWeight(C22), frac.x);\n"                                                                                \
	    "		float stp = mix(sx1, sx2, frac.y);\n"                                                                                                      \
	    "		vec2 rg = mix(mix(C11, C21, frac.x), mix(C12, C22, frac.x), frac.y);\n"                                                                    \
	    "		if(discardForSemiTransPass(rg, stp)) { discard; }\n"                                                                                       \
	    "		vec4 x1 = mix(lut(C11), lut(C21), frac.x);\n"                                                                                              \
	    "		vec4 x2 = mix(lut(C12), lut(C22), frac.x);\n"                                                                                              \
	    "		vec4 t = mix(x1, x2, frac.y);\n"                                                                                                           \
	    "		t.w = 1.0 - t.w;\n"                                                                                                                        \
	    "		return t;\n"                                                                                                                               \
	    "	}\n"                                                                                                                                        \
	    "	vec4 nearestTextureSample(vec2 P) {\n"                                                                                                      \
	    "		vec2 rg = samplePSX(P);\n"                                                                                                                 \
	    "		if(discardForSemiTransPass(rg, stpWeight(rg))) { discard; }\n"                                                                             \
	    "		vec4 t = lut(rg);\n"                                                                                                                       \
	    "		t.w = 1.0 - t.w;\n"                                                                                                                        \
	    "		return t;\n"                                                                                                                               \
	    "	}\n"                                                                                                                                        \
	    "	void main() {\n"                                                                                                                            \
	    "		vec4 color = (bilinearFilter > 0) ? bilinearTextureSample(v_texcoord.xy) : nearestTextureSample(v_texcoord.xy);\n"                         \
	    "		fragColor = dither(color * v_color);\n"                                                                                                    \
	    "		fragColor.a = float(psxDrawMaskSet);\n"                                                                                                    \
	    "	}\n"

global_variable const char *gpu_shader_common = "	varying vec4 v_texcoord;\n"
                                                "	varying vec4 v_color;\n"
                                                "	varying vec4 v_page_clut;\n"
                                                "	varying float v_z;\n";

const char *gte_shader_4 = GPU_FRAGMENT_SAMPLE_SHADER(4);
const char *gte_shader_8 = GPU_FRAGMENT_SAMPLE_SHADER(8);
const char *gte_shader_16 = GPU_FRAGMENT_SAMPLE_SHADER(16);
const char *gte_shader_32_rgba = "	uniform sampler2D s_texture;\n"
                                 "	uniform int psxDrawMaskSet;\n"
                                 "	uniform vec2 texelSize;\n"
                                 "	void main() {\n"
                                 "		vec2 tc = v_texcoord.xy * texelSize + texelSize * 0.5;\n"
                                 "		vec4 color = texture2D(s_texture, tc);\n"
                                 "		fragColor = dither(color * v_color);\n"
                                 "		fragColor.a = float(psxDrawMaskSet);\n"
                                 "	}\n";

#define GTE_PERSPECTIVE_CORRECTION "	gl_Position = Projection * vec4(a_position.xy, 0.0, 1.0);\n"

#define GTE_VERTEX_SHADER                                                                                          \
	"	attribute vec4 a_position;\n"                                                                                \
	"	attribute vec4 a_texcoord; // uv, color multiplier, dither\n"                                                \
	"	attribute vec4 a_color;\n"                                                                                   \
	"	attribute vec4 a_extra; // texcoord.xy ofs, unused.xy\n"                                                     \
	"	uniform mat4 Projection;\n"                                                                                  \
	"	const vec2 c_UVFudge = vec2(0.00025, 0.00025);\n"                                                            \
	"	void main() {\n"                                                                                             \
	"		v_texcoord = a_texcoord;\n"                                                                                 \
	"		v_texcoord.xy += a_extra.xy * 0.5;\n"                                                                       \
	"		v_color = a_color;\n"                                                                                       \
	"		v_color.xyz *= a_texcoord.z;\n"                                                                             \
	"		v_page_clut.x = fract(a_position.z / 16.0) * 1024.0;\n"                                                     \
	"		v_page_clut.y = floor(a_position.z / 16.0) * 256.0;\n"                                                      \
	"		v_page_clut.z = fract(a_position.w / 64.0);\n"                                                              \
	"		v_page_clut.w = floor(a_position.w / 64.0) / 512.0;\n"                                                      \
	"		v_page_clut.xy += c_UVFudge;\n"                                                                             \
	"		v_page_clut.zw += c_UVFudge;\n" GTE_PERSPECTIVE_CORRECTION "		v_z = (gl_Position.z - 40.0) * 0.005;\n" \
	"	}\n"

internal int NativeRenderer_Shader_CheckShaderStatus(GLuint shader)
{
	char info[1024];
	GLint result;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

	if (result == GL_TRUE)
		return 1;

	glGetShaderInfoLog(shader, sizeof(info), NULL, info);
	if (info[0] && strlen(info) > 8)
	{
		NATIVE_RENDERER_ERROR("%s\n", info);
		assert(0);
	}

	return 0;
}

internal int NativeRenderer_Shader_CheckProgramStatus(GLuint program)
{
	char info[1024];
	GLint result;

	glGetProgramiv(program, GL_LINK_STATUS, &result);

	if (result == GL_TRUE)
		return 1;

	glGetProgramInfoLog(program, sizeof(info), NULL, info);
	if (info[0] && strlen(info) > 8)
	{
		NATIVE_RENDERER_ERROR("%s\n", info);
		assert(0);
	}

	return 0;
}

internal ShaderID NativeRenderer_Shader_Compile(const char *source, bool isPsxShader)
{
	const char *GLSL_HEADER_VERT = "	#version 140\n"
	                               "	precision lowp  int;\n"
	                               "	precision highp float;\n"
	                               "	#define varying   out\n"
	                               "	#define attribute in\n"
	                               "	#define texture2D texture\n";

	const char *GLSL_HEADER_FRAG = "	#version 140\n"
	                               "	precision lowp  int;\n"
	                               "	precision highp float;\n"
	                               "	#define varying     in\n"
	                               "	#define texture2D   texture\n"
	                               "	out vec4 fragColor;\n";

	char extra_vs_defines[1024];
	char extra_fs_defines[1024];
	extra_vs_defines[0] = 0;
	extra_fs_defines[0] = 0;

	strcat(extra_vs_defines, "#define VERTEX\n");
	strcat(extra_fs_defines, "#define FRAGMENT\n");
	if (g_cfg_bilinearFiltering)
	{
		strcat(extra_fs_defines, "#define BILINEAR_FILTER\n");
	}

	const char *vs_list_psx[] = {GLSL_HEADER_VERT, extra_vs_defines, gpu_shader_common, GTE_VERTEX_SHADER};
	const char *fs_list_psx[] = {GLSL_HEADER_FRAG, extra_fs_defines, gpu_shader_common, GPU_DITHERING, source};
	const char *vs_list_src[] = {
	    GLSL_HEADER_VERT,
	    extra_vs_defines,
	    source,
	};
	const char *fs_list_src[] = {GLSL_HEADER_FRAG, extra_fs_defines, source};

	const char **vs_list = isPsxShader ? vs_list_psx : vs_list_src;
	const char **fs_list = isPsxShader ? fs_list_psx : fs_list_src;
	const int vs_list_cnt = isPsxShader ? 4 : 3;
	const int fs_list_cnt = isPsxShader ? 5 : 3;

	GLuint program = glCreateProgram();

	{
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, vs_list_cnt, vs_list, NULL);
		glCompileShader(vertexShader);

		if (NativeRenderer_Shader_CheckShaderStatus(vertexShader) == 0)
			NATIVE_RENDERER_ERROR("Failed to compile Vertex Shader!\n");

		glAttachShader(program, vertexShader);
		glDeleteShader(vertexShader);
	}

	{
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, fs_list_cnt, fs_list, NULL);
		glCompileShader(fragmentShader);

		if (NativeRenderer_Shader_CheckShaderStatus(fragmentShader) == 0)
			NATIVE_RENDERER_ERROR("Failed to compile Fragment Shader!\n");

		glAttachShader(program, fragmentShader);
		glDeleteShader(fragmentShader);
	}

	glBindAttribLocation(program, a_position, "a_position");
	glBindAttribLocation(program, a_texcoord, "a_texcoord");
	glBindAttribLocation(program, a_color, "a_color");

	glLinkProgram(program);
	if (NativeRenderer_Shader_CheckProgramStatus(program) == 0)
		NATIVE_RENDERER_ERROR("Failed to link Shader!\n");

	GLint textureSampler = 0;
	GLint lutSampler = 1;
	glUseProgram(program);
	glUniform1iv(glGetUniformLocation(program, "s_texture"), 1, &textureSampler);
	glUniform1iv(glGetUniformLocation(program, "s_rgLut"), 1, &lutSampler);
	glUseProgram(0);

	return program;
}

//--------------------------------------------------------------------------------------------

internal void NativeRenderer_GenerateCommonTextures(void)
{
	u32 whitePixelData = 0xFFFFFFFF;

	glGenTextures(1, &s_whiteTexture);
	{
		glBindTexture(GL_TEXTURE_2D, s_whiteTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whitePixelData);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glGenTextures(1, &s_rgLutTexture);
	{
		glBindTexture(GL_TEXTURE_2D, s_rgLutTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, LUT_WIDTH, LUT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rgLUT);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

internal TextureID NativeRenderer_CreateRGBATexture(int width, int height, u8 *data)
{
	TextureID newTexture;
	glGenTextures(1, &newTexture);

	glBindTexture(GL_TEXTURE_2D, newTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_cfg_bilinearFiltering ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_cfg_bilinearFiltering ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return newTexture;
}

internal void NativeRenderer_CompilePSXShader(GTEShader *sh, const char *source)
{
	sh->shader = NativeRenderer_Shader_Compile(source, true);

	sh->bilinearFilterLoc = glGetUniformLocation(sh->shader, "bilinearFilter");
	sh->projectionLoc = glGetUniformLocation(sh->shader, "Projection");
	sh->texelSizeLoc = glGetUniformLocation(sh->shader, "texelSize");
	sh->texLoc = glGetUniformLocation(sh->shader, "s_texture");
	sh->lutLoc = glGetUniformLocation(sh->shader, "s_rgLut");
	sh->psxSemiTransPassLoc = glGetUniformLocation(sh->shader, "psxSemiTransPass");
	sh->psxDrawMaskSetLoc = glGetUniformLocation(sh->shader, "psxDrawMaskSet");
}

internal void NativeRenderer_InitialisePSXShaders(void)
{
	NativeRenderer_CompilePSXShader(&s_gteShader4, gte_shader_4);
	NativeRenderer_CompilePSXShader(&s_gteShader8, gte_shader_8);
	NativeRenderer_CompilePSXShader(&s_gteShader16, gte_shader_16);
	NativeRenderer_CompilePSXShader(&s_gteShader32Rgba, gte_shader_32_rgba);
}

internal void NativeRenderer_InitRG8LUT(void)
{
	for (u16 y = 0; y < LUT_HEIGHT; y++)
	{
		u8 *row = rgLUT + y * (LUT_WIDTH * 4);
		for (u16 x = 0; x < LUT_WIDTH; x++)
		{
			const u16 c = (y << 8) | x;
			u8 *pixel = row + x * 4;
			pixel[0] = (u8)((c & 31) << 3);
			pixel[1] = (u8)(((c >> 5) & 31) << 3);
			pixel[2] = (u8)(((c >> 10) & 31) << 3);
			pixel[3] = (u8)(((c >> 15) & 1) << 7);
		}
	}
}

int NativeRenderer_InitialisePSX(void)
{
	SDL_memset(vram, 0, VRAM_WIDTH * VRAM_HEIGHT * sizeof(u16));
	NativeRenderer_InitRG8LUT();
	NativeRenderer_GenerateCommonTextures();
	NativeRenderer_InitialisePSXShaders();

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_STENCIL_TEST);
	glBlendColor(0.5f, 0.5f, 0.5f, 0.25f);

	// gen framebuffer
	{
		memset(&s_glFramebufferPBO, 0, sizeof(s_glFramebufferPBO));
		PBO_Init(&s_glFramebufferPBO, GL_RGBA, VRAM_WIDTH, VRAM_HEIGHT, 2);

		// make a special texture
		// it will be resized later
		glGenTextures(1, &s_framebufferTexture);
		{
			glBindTexture(GL_TEXTURE_2D, s_framebufferTexture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			// default to VRAM size
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VRAM_WIDTH, VRAM_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glGenFramebuffers(1, &s_glBlitFramebuffer);
		{
			glBindFramebuffer(GL_FRAMEBUFFER, s_glBlitFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_framebufferTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	// gen offscreen RT
	{
		memset(&s_glOffscreenPBO, 0, sizeof(s_glOffscreenPBO));
		PBO_Init(&s_glOffscreenPBO, GL_RGBA, VRAM_WIDTH, VRAM_HEIGHT, 2);

		// offscreen texture render target
		glGenTextures(1, &s_offscreenRenderTexture);
		{
			glBindTexture(GL_TEXTURE_2D, s_offscreenRenderTexture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			// default to VRAM size
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VRAM_WIDTH, VRAM_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glGenFramebuffers(1, &s_glOffscreenFramebuffer);
		{
			glBindFramebuffer(GL_FRAMEBUFFER, s_glOffscreenFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_offscreenRenderTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	// gen VRAM textures.
	// double-buffered
	{
		int i;

		glGenTextures(2, s_vramTexturesDouble);

		for (i = 0; i < 2; i++)
		{
			glBindTexture(GL_TEXTURE_2D, s_vramTexturesDouble[i]);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			// set storage size
			glTexImage2D(GL_TEXTURE_2D, 0, VRAM_INTERNAL_FORMAT, VRAM_WIDTH, VRAM_HEIGHT, 0, VRAM_FORMAT, GL_UNSIGNED_BYTE, NULL);
		}

		s_vramTexture = s_vramTexturesDouble[0];

		glBindTexture(GL_TEXTURE_2D, 0);

		// VRAM framebuffer for offscreen blitting to VRAM
		glGenFramebuffers(1, &s_glVramFramebuffer);
		{
			glBindFramebuffer(GL_FRAMEBUFFER, s_glVramFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_vramTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	// gen vertex buffer and index buffer
	{
		int i;

		glGenBuffers(MAX_NUM_VERTEX_BUFFERS, s_glVertexBuffer);
		glGenVertexArrays(MAX_NUM_VERTEX_BUFFERS, s_glVertexArray);

		for (i = 0; i < MAX_NUM_VERTEX_BUFFERS; i++)
		{
			glBindVertexArray(s_glVertexArray[i]);

			glBindBuffer(GL_ARRAY_BUFFER, s_glVertexBuffer[i]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GrVertex) * MAX_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
		}

		glBindVertexArray(0);
	}

	NativeRenderer_ResetDevice();

	return 1;
}

internal void NativeRenderer_Ortho2D(float left, float right, float bottom, float top, float znear, float zfar)
{
	float a = 2.0f / (right - left);
	float b = 2.0f / (top - bottom);
	float c = 2.0f / (znear - zfar);

	float x = (left + right) / (left - right);
	float y = (bottom + top) / (bottom - top);

	// -1..1
	float z = (znear + zfar) / (znear - zfar);

	float ortho[16] = {a, 0, 0, 0, 0, b, 0, 0, 0, 0, c, 0, x, y, z, 1};

	glUniformMatrix4fv(u_projectionLoc, 1, GL_FALSE, ortho);
}

void NativeRenderer_SetupClipMode(const RECT16 *rect, int enable)
{
	// [A] isinterlaced dirty hack for widescreen
	const bool scissorOn = enable && (activeDispEnv.isinter || (rect->x - activeDispEnv.disp.x > 0 || rect->y - activeDispEnv.disp.y > 0 ||
	                                                            rect->w < activeDispEnv.disp.w - 1 || rect->h < activeDispEnv.disp.h - 1));

	NativeRenderer_SetScissorState(scissorOn);

	if (!scissorOn)
		return;

	const float emuScreenAspect = 1.0f;

	const float psxScreenWInv = 1.0f / (float)activeDispEnv.disp.w;
	const float psxScreenHInv = 1.0f / (float)activeDispEnv.disp.h;

	// first map to 0..1
	float clipRectX = (float)(rect->x - activeDispEnv.disp.x) * psxScreenWInv;
	float clipRectY = (float)(rect->y - activeDispEnv.disp.y) * psxScreenHInv;
	float clipRectW = (float)(rect->w) * psxScreenWInv;
	float clipRectH = (float)(rect->h) * psxScreenHInv;

	// then map to screen
	{
		clipRectX -= 0.5f;

		clipRectX *= emuScreenAspect;
		clipRectW *= emuScreenAspect;

		clipRectX += 0.5f;
	}

	// adjust scissor rectangle by the backbuffer size (window dimensions)
	const float flipOffset = g_windowHeight - clipRectH * (float)g_windowHeight;
	const float crx = clipRectX * (float)g_windowWidth;
	const float cry = clipRectY * (float)g_windowHeight;
	const float crw = clipRectW * (float)g_windowWidth;
	const float crh = clipRectH * (float)g_windowHeight;

	glScissor(crx, flipOffset - cry, crw, crh);
}

internal void NativeRenderer_SetShader(const ShaderID shader)
{
	if (s_previousShader != shader)
	{
		glUseProgram(shader);

		s_previousShader = shader;
	}
}


void NativeRenderer_SetTexture(TextureID texture, TexFormat texFormat)
{
	GLint texLoc = -1;
	GLint lutLoc = -1;

	switch (texFormat)
	{
	case TF_4_BIT:
		NativeRenderer_SetShader(s_gteShader4.shader);
		u_bilinearFilterLoc = s_gteShader4.bilinearFilterLoc;
		u_projectionLoc = s_gteShader4.projectionLoc;
		texLoc = s_gteShader4.texLoc;
		lutLoc = s_gteShader4.lutLoc;
		u_texelSizeLoc = -1;
		u_psxSemiTransPassLoc = s_gteShader4.psxSemiTransPassLoc;
		u_psxDrawMaskSetLoc = s_gteShader4.psxDrawMaskSetLoc;
		break;
	case TF_8_BIT:
		NativeRenderer_SetShader(s_gteShader8.shader);
		u_bilinearFilterLoc = s_gteShader8.bilinearFilterLoc;
		u_projectionLoc = s_gteShader8.projectionLoc;
		texLoc = s_gteShader8.texLoc;
		lutLoc = s_gteShader8.lutLoc;
		u_texelSizeLoc = -1;
		u_psxSemiTransPassLoc = s_gteShader8.psxSemiTransPassLoc;
		u_psxDrawMaskSetLoc = s_gteShader8.psxDrawMaskSetLoc;
		break;
	case TF_16_BIT:
		NativeRenderer_SetShader(s_gteShader16.shader);
		u_bilinearFilterLoc = s_gteShader16.bilinearFilterLoc;
		u_projectionLoc = s_gteShader16.projectionLoc;
		texLoc = s_gteShader16.texLoc;
		lutLoc = s_gteShader16.lutLoc;
		u_texelSizeLoc = -1;
		u_psxSemiTransPassLoc = s_gteShader16.psxSemiTransPassLoc;
		u_psxDrawMaskSetLoc = s_gteShader16.psxDrawMaskSetLoc;
		break;
	case TF_32_BIT_RGBA:
		NativeRenderer_SetShader(s_gteShader32Rgba.shader);
		u_bilinearFilterLoc = s_gteShader32Rgba.bilinearFilterLoc;
		u_projectionLoc = s_gteShader32Rgba.projectionLoc;
		texLoc = s_gteShader32Rgba.texLoc;
		lutLoc = s_gteShader32Rgba.lutLoc;
		u_texelSizeLoc = s_gteShader32Rgba.texelSizeLoc;
		u_psxSemiTransPassLoc = s_gteShader32Rgba.psxSemiTransPassLoc;
		u_psxDrawMaskSetLoc = s_gteShader32Rgba.psxDrawMaskSetLoc;
		break;
	}

	if (g_dbg_texturelessMode)
	{
		texture = s_whiteTexture;
	}

	if (s_lastBoundTexture == texture)
	{
		return;
	}

	if (texLoc >= 0)
		glUniform1i(texLoc, 0);
	if (lutLoc >= 0)
		glUniform1i(lutLoc, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, s_rgLutTexture);

	glActiveTexture(GL_TEXTURE0);

	if (u_bilinearFilterLoc >= 0)
		glUniform1i(u_bilinearFilterLoc, g_cfg_bilinearFiltering);
	NativeRenderer_SetPSXTextureSemiTransPass(0);

	s_lastBoundTexture = texture;
}

void NativeRenderer_SetOverrideTextureSize(int width, int height)
{
	if (u_texelSizeLoc == -1)
		return;

	float vec[] = {1.0f / (float)width, 1.0f / (float)height};
	glUniform2fv(u_texelSizeLoc, 1, vec);
}

void NativeRenderer_SetPSXTextureSemiTransPass(int pass)
{
	if (u_psxSemiTransPassLoc >= 0)
		glUniform1i(u_psxSemiTransPassLoc, pass);
}

void NativeRenderer_SetPSXDrawMaskSet(int maskSet)
{
	if (u_psxDrawMaskSetLoc >= 0)
		glUniform1i(u_psxDrawMaskSetLoc, maskSet);
}

internal void NativeRenderer_DestroyTexture(TextureID texture)
{
	if (texture == -1)
		return;

	glDeleteTextures(1, &texture);
}

void NativeRenderer_ClearVRAM(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	s_vramNeedsUpdate = 1;

	u16 *dst = vram + x + y * VRAM_WIDTH;

	if (x + w > VRAM_WIDTH)
		w = VRAM_WIDTH - x;

	if (y + h > VRAM_HEIGHT)
		h = VRAM_HEIGHT - y;

	// clear VRAM region with given color
	for (int i = 0; i < h; i++)
	{
		u16 *tmp = dst;

		for (int j = 0; j < w; j++)
			*tmp++ = r | (g << 5) | (b << 11);

		dst += VRAM_WIDTH;
	}
}

void NativeRenderer_Clear(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	s_framebufferNeedsUpdate = 1;

	glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void NativeRenderer_SaveVRAM(const char *outputFileName, int x, int y, int width, int height, int bReadFromFrameBuffer)
{
#define FLIP_Y (VRAM_HEIGHT - i - 1)

	FILE *fp = fopen(outputFileName, "wb");
	if (fp == NULL)
		return;

	u8 TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	u8 header[6];
	header[0] = (width % 256);
	header[1] = (width / 256);
	header[2] = (height % 256);
	header[3] = (height / 256);
	header[4] = 16;
	header[5] = 0;

	fwrite(TGAheader, sizeof(u8), 12, fp);
	fwrite(header, sizeof(u8), 6, fp);

	for (int i = 0; i < VRAM_HEIGHT; i++)
	{
		fwrite(vram + VRAM_WIDTH * FLIP_Y, sizeof(u16), VRAM_WIDTH, fp);
	}

	fclose(fp);

#undef FLIP_Y
}

internal void NativeRenderer_CopyRGBAFramebufferToVRAM(u32 *src, int x, int y, int w, int h, int update_vram, int flip_y)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(x + w <= VRAM_WIDTH);
	assert(y + h <= VRAM_HEIGHT);

	u16 *fb = (u16 *)malloc(w * h * sizeof(u16));
	u32 *data_src = (u32 *)src;
	u16 *data_dst = (u16 *)fb;

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			u32 c = *data_src++;

			u8 r = ((c >> 3) & 0x1F);
			u8 g = ((c >> 11) & 0x1F);
			u8 b = ((c >> 19) & 0x1F);
			// NOTE(aalhendi): framebuffer alpha carries the PS1 E6 mask/STP bit.
			u8 a = (c >> 31) & 1;

			*data_dst++ = r | (g << 5) | (b << 10) | (a << 15);
		}
	}

	u16 *ptr = (u16 *)vram + VRAM_WIDTH * y + x;

	for (int fy = 0; fy < h; fy++)
	{
		int py = flip_y ? (h - fy - 1) : fy;
		u16 *fb_ptr = fb + (h * py / h) * w;

		for (int fx = 0; fx < w; fx++)
			ptr[fx] = fb_ptr[w * fx / w];

		ptr += VRAM_WIDTH;
	}

	free(fb);

	if (update_vram)
		s_vramNeedsUpdate = 1;
}

void NativeRenderer_ReadFramebufferDataToVRAM(void)
{
	int x, y, w, h;
	if (!s_framebufferNeedsUpdate)
		return;

	s_framebufferNeedsUpdate = 0;

	x = s_previousFramebuffer.x;
	y = s_previousFramebuffer.y;
	w = s_previousFramebuffer.w;
	h = s_previousFramebuffer.h;

	// now we can read it back to VRAM texture

	// read the texture
	if (s_glFramebufferPBO.pixels)
	{
		glBindTexture(GL_TEXTURE_2D, s_framebufferTexture);
		PBO_Download(&s_glFramebufferPBO);
		glBindTexture(GL_TEXTURE_2D, 0);
		// NOTE(aalhendi): Keep the CPU-side VRAM mirror packed like PS1 VRAM.
		// Host texture bindings are invalid after this direct GL texture read.
		NativeRenderer_CopyRGBAFramebufferToVRAM((u32 *)s_glFramebufferPBO.pixels, x, y, w, h, 1, 0);
		s_lastBoundTexture = -1;
	}
}

internal void NativeRenderer_SetScissorState(int enable)
{
	if (s_previousScissorState == enable)
		return;

	if (s_previousScissorState)
		glDisable(GL_SCISSOR_TEST);
	else
		glEnable(GL_SCISSOR_TEST);
	s_previousScissorState = enable;
}

void NativeRenderer_SetOffscreenState(const RECT16 *offscreenRect, int enable)
{
	if (enable)
	{
		// setup render target viewport
		NativeRenderer_Ortho2D(0, offscreenRect->w, offscreenRect->h, 0, -1.0f, 1.0f);
	}
	else
	{
		// setup default viewport
		NativeRenderer_Ortho2D(0, activeDispEnv.disp.w, activeDispEnv.disp.h, 0, -1.0f, 1.0f);
	}

	if (s_previousOffscreenState == enable)
		return;

	s_previousOffscreenState = enable;

	if (enable)
	{
		// set storage size first
		if (s_previousOffscreen.w != offscreenRect->w && s_previousOffscreen.h != offscreenRect->h)
		{
			glBindTexture(GL_TEXTURE_2D, s_offscreenRenderTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, offscreenRect->w, offscreenRect->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		s_previousOffscreen = *offscreenRect;

		NativeRenderer_SetViewPort(0, 0, offscreenRect->w, offscreenRect->h);
		glBindFramebuffer(GL_FRAMEBUFFER, s_glOffscreenFramebuffer);

		// clear it out
		glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		NativeRenderer_SetViewPort(0, 0, g_windowWidth, g_windowHeight);

		// before drawing set source and target
		{
			glBindFramebuffer(GL_FRAMEBUFFER, s_glVramFramebuffer);

			// rebind texture
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_vramTexture, 0);

			// setup draw and read framebuffers
			glBindFramebuffer(GL_READ_FRAMEBUFFER, s_glOffscreenFramebuffer); // source is backbuffer
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_glVramFramebuffer);

			glBlitFramebuffer(0, 0, s_previousOffscreen.w, s_previousOffscreen.h, s_previousOffscreen.x, s_previousOffscreen.y + s_previousOffscreen.h,
			                  s_previousOffscreen.x + s_previousOffscreen.w, s_previousOffscreen.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			// done, unbind
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// copy rendering results to VRAM texture
		{
			// read the texture
			glBindTexture(GL_TEXTURE_2D, s_offscreenRenderTexture);
			// glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			PBO_Download(&s_glOffscreenPBO);
			glBindTexture(GL_TEXTURE_2D, s_lastBoundTexture);

			// Do not force a VRAM texture upload unless the blit path is off.
			NativeRenderer_CopyRGBAFramebufferToVRAM((u32 *)s_glOffscreenPBO.pixels, s_previousOffscreen.x, s_previousOffscreen.y, s_previousOffscreen.w,
			                                         s_previousOffscreen.h, 0, 1);
		}
	}
}

void NativeRenderer_StoreFrameBuffer(int x, int y, int w, int h)
{
	// set storage size first
	if (s_previousFramebuffer.w != w || s_previousFramebuffer.h != h)
	{
		glBindTexture(GL_TEXTURE_2D, s_framebufferTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	s_previousFramebuffer.x = x;
	s_previousFramebuffer.y = y;
	s_previousFramebuffer.w = w;
	s_previousFramebuffer.h = h;

	glBindFramebuffer(GL_FRAMEBUFFER, s_glBlitFramebuffer);

	// before drawing set source and target
	{
		// setup draw and read framebuffers
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // source is backbuffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_glBlitFramebuffer);

		glBlitFramebuffer(0, 0, g_windowWidth, g_windowHeight, x, y + h, x + w, y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// done, unbind
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}

	// after drawing
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glFlush();

	u32 *pixels = (u32 *)malloc((size_t)w * (size_t)h * sizeof(u32));
	if (pixels != NULL)
	{
		// NOTE(aalhendi): Screen-feedback effects sample PS1 VRAM as packed
		// 16-bit pixels. Do not leave the VRAM texture in host RGBA form here;
		// pack the copied framebuffer synchronously before the next primitive
		// can sample it.
		glBindTexture(GL_TEXTURE_2D, s_framebufferTexture);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture(GL_TEXTURE_2D, 0);

		NativeRenderer_CopyRGBAFramebufferToVRAM(pixels, x, y, w, h, 1, 0);
		NativeRenderer_UpdateVRAM();
		s_lastBoundTexture = -1;

		free(pixels);
	}
}

void NativeRenderer_CopyVRAM(u16 *src, int x, int y, int w, int h, int dst_x, int dst_y)
{
	s_vramNeedsUpdate = 1;

	int stride = w;

	if (!src)
	{
		// NOTE(aalhendi): MoveImage copies from PS1 VRAM. If native has a pending
		// host framebuffer readback, pull it into the CPU VRAM mirror before the
		// copy. Do not schedule a later readback here; boot TIM display moves are
		// immediately followed by DrawSync, and a deferred black backbuffer read can
		// wipe the copied splash page.
		NativeRenderer_ReadFramebufferDataToVRAM();
		src = vram;
		stride = VRAM_WIDTH;
	}

	src += x + y * stride;

	u16 *dst = vram + dst_x + dst_y * VRAM_WIDTH;

	for (int i = 0; i < h; i++)
	{
		SDL_memcpy(dst, src, w * sizeof(u16));
		dst += VRAM_WIDTH;
		src += stride;
	}
}

void NativeRenderer_ReadVRAM(u16 *dst, int x, int y, int dst_w, int dst_h)
{
	u16 *src = vram + x + VRAM_WIDTH * y;

	for (int i = 0; i < dst_h; i++)
	{
		SDL_memcpy(dst, src, dst_w * sizeof(u16));
		dst += dst_w;
		src += VRAM_WIDTH;
	}
}

int NativeRenderer_GetVRAMStateSize(void)
{
	return (int)sizeof(vram);
}

int NativeRenderer_CaptureVRAMState(void *dst, int dstSize)
{
	if ((dst == NULL) || (dstSize < (int)sizeof(vram)))
		return 0;

	// NOTE(aalhendi): Save-states own the CPU-side PSX VRAM mirror, not GL
	// textures or PBOs. Pull pending framebuffer copies into the mirror first.
	NativeRenderer_ReadFramebufferDataToVRAM();
	SDL_memcpy(dst, vram, sizeof(vram));
	return 1;
}

int NativeRenderer_RestoreVRAMState(const void *src, int srcSize)
{
	local_persist const RECT16 zeroRect = {0, 0, 0, 0};

	if ((src == NULL) || (srcSize < (int)sizeof(vram)))
		return 0;

	SDL_memcpy(vram, src, sizeof(vram));
	// NOTE(aalhendi): Restored VRAM is authoritative PSX state. Host GL caches
	// are rebuildable, so mark the texture dirty and drop stale bindings.
	s_vramNeedsUpdate = 1;
	s_framebufferNeedsUpdate = 0;
	s_previousFramebuffer = zeroRect;
	s_previousOffscreen = zeroRect;
	s_previousOffscreenState = 0;
	s_previousShader = -1;
	s_lastBoundTexture = -1;
	return 1;
}

void NativeRenderer_UpdateVRAM(void)
{
	if (!s_vramNeedsUpdate)
		return;

	s_vramNeedsUpdate = 0;

	s_vramTexture = s_vramTexturesDouble[s_vramTextureIndex];
	s_vramTextureIndex++;
	s_vramTextureIndex &= 1;

	glBindTexture(GL_TEXTURE_2D, s_vramTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, VRAM_INTERNAL_FORMAT, VRAM_WIDTH, VRAM_HEIGHT, 0, VRAM_FORMAT, GL_UNSIGNED_BYTE, vram);
}

internal u8 NativeRenderer_Expand5To8(u16 value)
{
	value &= 0x1f;
	return (u8)((value << 3) | (value >> 2));
}

internal void NativeRenderer_BuildDisplayChunkRGBA(u8 *dst, int srcX, int srcY, int w, int h)
{
	for (int y = 0; y < h; y++)
	{
		u16 *src = vram + srcX + (srcY + y) * VRAM_WIDTH;

		for (int x = 0; x < w; x++)
		{
			u16 pixel = src[x];

			*dst++ = NativeRenderer_Expand5To8(pixel);
			*dst++ = NativeRenderer_Expand5To8(pixel >> 5);
			*dst++ = NativeRenderer_Expand5To8(pixel >> 10);
			*dst++ = 0xff;
		}
	}
}

internal void NativeRenderer_FillDisplayChunkVerts(GrVertex *verts, int dstX, int dstY, int w, int h)
{
	const int maxU = w > 0 ? w - 1 : 0;
	const int maxV = h > 0 ? h - 1 : 0;

	memset(verts, 0, sizeof(GrVertex) * 6);

	verts[0].x = dstX;
	verts[0].y = dstY;
	verts[0].u = 0;
	verts[0].v = 0;

	verts[1].x = dstX;
	verts[1].y = dstY + h;
	verts[1].u = 0;
	verts[1].v = maxV;

	verts[2].x = dstX + w;
	verts[2].y = dstY + h;
	verts[2].u = maxU;
	verts[2].v = maxV;

	verts[3] = verts[0];
	verts[4] = verts[2];

	verts[5].x = dstX + w;
	verts[5].y = dstY;
	verts[5].u = maxU;
	verts[5].v = 0;

	for (int i = 0; i < 6; i++)
	{
		verts[i].bright = 1;
		verts[i].r = 0xff;
		verts[i].g = 0xff;
		verts[i].b = 0xff;
		verts[i].a = 0xff;
	}
}

void NativeRenderer_PresentVRAMDisplay(void)
{
	// NOTE(aalhendi): ctr-native local divergence. Retail presents this boot
	// splash path by displaying VRAM directly after DR_MOVE packets; the native
	// OpenGL backend otherwise swaps the current framebuffer and never shows
	// those VRAM-only copies.
	const int displayX = activeDispEnv.disp.x;
	const int displayY = activeDispEnv.disp.y;
	const int displayW = activeDispEnv.disp.w;
	const int displayH = activeDispEnv.disp.h;
	const int maxChunkW = 0x100;
	const int maxChunkH = 0x100;
	const int maxChunkBytes = maxChunkW * maxChunkH * 4;

	local_persist TextureID displayTexture = (TextureID)-1;
	local_persist u8 *rgba = NULL;

	if (displayW <= 0 || displayH <= 0)
		return;

	if (rgba == NULL)
		rgba = (u8 *)malloc(maxChunkBytes);

	if (rgba == NULL)
		return;

	if (displayTexture == (TextureID)-1)
		displayTexture = NativeRenderer_CreateRGBATexture(maxChunkW, maxChunkH, NULL);

	NativeRenderer_SetViewPort(0, 0, g_windowWidth, g_windowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	NativeRenderer_SetScissorState(0);
	NativeRenderer_EnableDepth(0);
	NativeRenderer_SetBlendMode(BM_NONE);
	NativeRenderer_SetTexture(displayTexture, TF_32_BIT_RGBA);
	NativeRenderer_SetPSXDrawMaskSet(0);
	NativeRenderer_Ortho2D(0, displayW, displayH, 0, -1.0f, 1.0f);

	for (int y = 0; y < displayH; y += maxChunkH)
	{
		const int chunkH = (displayH - y) > maxChunkH ? maxChunkH : displayH - y;

		for (int x = 0; x < displayW; x += maxChunkW)
		{
			GrVertex verts[6];
			const int chunkW = (displayW - x) > maxChunkW ? maxChunkW : displayW - x;

			NativeRenderer_BuildDisplayChunkRGBA(rgba, displayX + x, displayY + y, chunkW, chunkH);

			glBindTexture(GL_TEXTURE_2D, displayTexture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chunkW, chunkH, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

			NativeRenderer_SetOverrideTextureSize(maxChunkW, maxChunkH);
			NativeRenderer_FillDisplayChunkVerts(verts, x, y, chunkW, chunkH);
			NativeRenderer_UpdateVertexBuffer(verts, 6);
			NativeRenderer_DrawTriangles(0, 2);
		}
	}
}

void NativeRenderer_SwapWindow(void)
{
	SDL_GL_SwapWindow(g_window);

	// glFinish();
}

internal void NativeRenderer_EnableDepth(int enable)
{
	if (s_previousDepthMode == enable)
		return;

	s_previousDepthMode = enable;

	glDisable(GL_DEPTH_TEST);
}

void NativeRenderer_SetStencilMode(int drawPrim)
{
	if (s_previousStencilMode == drawPrim)
		return;

	s_previousStencilMode = drawPrim;

	if (drawPrim)
	{
		glStencilFunc(GL_ALWAYS, 1, 0x10);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	}
	else
	{
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
	}
}

void NativeRenderer_SetBlendMode(BlendMode blendMode)
{
	if (s_previousBlendMode == blendMode)
		return;

	if (blendMode == BM_NONE)
	{
		if (s_previousBlendMode != BM_NONE)
		{
			glBlendColor(1.f, 1.f, 1.f, 1.f);
			glDisable(GL_BLEND);
		}

		s_previousBlendMode = blendMode;
		NativeRenderer_EnableDepth(1);
		return;
	}
	else
	{
		if (s_previousBlendMode == BM_NONE)
		{
			glBlendColor(0.25f, 0.25f, 0.25f, 0.5f);
			glEnable(GL_BLEND);
		}

		s_previousBlendMode = blendMode;
		NativeRenderer_EnableDepth(0);
	}

	glBlendEquationSeparate(blendMode == BM_SUBTRACT ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_ADD, GL_FUNC_ADD);
	switch (blendMode)
	{
	case BM_AVERAGE:
		// NOTE(aalhendi): keep RGB blend weight constant so alpha can carry the PS1 mask bit.
		glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_ONE, GL_ZERO);
		break;
	case BM_ADD:
	case BM_SUBTRACT:
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
		break;
	case BM_ADD_QUATER_SOURCE:
		glBlendFuncSeparate(GL_CONSTANT_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
		break;
	}

	s_previousBlendMode = blendMode;
}

internal void NativeRenderer_SetViewPort(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

internal void NativeRenderer_SetWireframe(int enable)
{
	glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_LINE : GL_FILL);
}

internal void NativeRenderer_BindVertexBuffer(void)
{
	glBindVertexArray(s_glVertexArray[s_curVertexBuffer]);

	glEnableVertexAttribArray(a_position);
	glEnableVertexAttribArray(a_texcoord);
	glEnableVertexAttribArray(a_color);
	glEnableVertexAttribArray(a_extra);

	glVertexAttribPointer(a_position, 4, GL_SHORT, GL_FALSE, sizeof(GrVertex), &((GrVertex *)NULL)->x);
	glVertexAttribPointer(a_texcoord, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(GrVertex), &((GrVertex *)NULL)->u);
	glVertexAttribPointer(a_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GrVertex), &((GrVertex *)NULL)->r);
	glVertexAttribPointer(a_extra, 4, GL_BYTE, GL_FALSE, sizeof(GrVertex), &((GrVertex *)NULL)->tcx);

	s_curVertexBuffer++;
	s_curVertexBuffer &= 1;
}

void NativeRenderer_UpdateVertexBuffer(const GrVertex *vertices, int num_vertices)
{
	if (num_vertices >= MAX_VERTEX_BUFFER_SIZE)
	{
		NATIVE_RENDERER_ERROR("MAX_VERTEX_BUFFER_SIZE reached, expect rendering errors\n");
		num_vertices = MAX_VERTEX_BUFFER_SIZE;
	}

	// assert(num_vertices <= MAX_VERTEX_BUFFER_SIZE);
	NativeRenderer_BindVertexBuffer();

	glBufferSubData(GL_ARRAY_BUFFER, 0, num_vertices * sizeof(GrVertex), vertices);
}

void NativeRenderer_DrawTriangles(int start_vertex, int triangles)
{
	glDrawArrays(GL_TRIANGLES, start_vertex, triangles * 3);
}

void NativeRenderer_PushDebugLabel(const char *label)
{
	if (!GLAD_GL_KHR_debug)
		return;
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0x8000, strlen(label), label);
}

void NativeRenderer_PopDebugLabel(void)
{
	if (!GLAD_GL_KHR_debug)
		return;
	glPopDebugGroup();
}
