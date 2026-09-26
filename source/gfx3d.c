// Implémentation du moteur 3D immédiat déclaré dans gfx3d.h.

#include "gfx3d.h"
#include <string.h>
#include <stdbool.h>

typedef struct
{
    float position[3];
    float color[4];
} Gfx3DVertex;

// Fourni par le build : shader vertex compilé en .shbin puis converti en
// objet via bin2s (voir Makefile). Le nom du symbole suit le nom du fichier
// .shbin : vshader3d.v.pica.shbin -> vshader3d_v_pica_shbin[] /
// vshader3d_v_pica_shbin_size.
extern const u8  vshader3d_v_pica_shbin[];
extern const u32 vshader3d_v_pica_shbin_size;

static DVLB_s*         s_shaderDvlb;
static shaderProgram_s s_program;
static int             s_uLocProjection = -1;
static int             s_uLocModelView  = -1;

static C3D_Mtx s_projection;
static C3D_Mtx s_modelView;

static Gfx3DVertex*   s_vbo;
static int             s_vboCount;
static GPU_Primitive_t s_primType;
static int             s_recording;

int Gfx3D_Init(void)
{
    s_vbo = (Gfx3DVertex*)linearAlloc(sizeof(Gfx3DVertex) * GFX3D_MAX_VERTICES);
    if (!s_vbo)
        return -1;

    s_shaderDvlb = DVLB_ParseFile((u32*)vshader3d_v_pica_shbin,
                                  vshader3d_v_pica_shbin_size);
    if (!s_shaderDvlb)
    {
        linearFree(s_vbo);
        s_vbo = NULL;
        return -1;
    }

    shaderProgramInit(&s_program);
    shaderProgramSetVsh(&s_program, &s_shaderDvlb->DVLE[0]);

    s_uLocProjection = shaderInstanceGetUniformLocation(s_program.vertexShader, "projection");
    s_uLocModelView  = shaderInstanceGetUniformLocation(s_program.vertexShader, "modelView");

    Mtx_Identity(&s_modelView);
    Gfx3D_SetFov(60.0f, 400.0f / 240.0f);

    s_vboCount   = 0;
    s_recording  = 0;
    s_primType   = GPU_TRIANGLES;
    return 0;
}

void Gfx3D_Exit(void)
{
    if (s_shaderDvlb)
    {
        shaderProgramFree(&s_program);
        DVLB_Free(s_shaderDvlb);
        s_shaderDvlb = NULL;
    }
    if (s_vbo)
    {
        linearFree(s_vbo);
        s_vbo = NULL;
    }
}

void Gfx3D_SetCamera(float eyeX, float eyeY, float eyeZ,
                     float targetX, float targetY, float targetZ)
{
    C3D_FVec eye    = FVec3_New(eyeX, eyeY, eyeZ);
    C3D_FVec target = FVec3_New(targetX, targetY, targetZ);
    C3D_FVec up     = FVec3_New(0.0f, 1.0f, 0.0f);
    Mtx_LookAt(&s_modelView, eye, target, up, false);
}

void Gfx3D_SetFov(float fovDegrees, float aspectRatio)
{
    Mtx_PerspTilt(&s_projection, C3D_AngleFromDegrees(fovDegrees),
                  aspectRatio, 0.1f, 100.0f, false);
}

void Gfx3D_Begin(GPU_Primitive_t primType)
{
    s_primType  = primType;
    s_vboCount  = 0;
    s_recording = 1;
}

void Gfx3D_Vertex(float x, float y, float z, int r, int g, int b)
{
    if (!s_recording || s_vboCount >= GFX3D_MAX_VERTICES)
        return;

    Gfx3DVertex* v = &s_vbo[s_vboCount++];
    v->position[0] = x;
    v->position[1] = y;
    v->position[2] = z;
    v->color[0] = (float)r / 255.0f;
    v->color[1] = (float)g / 255.0f;
    v->color[2] = (float)b / 255.0f;
    v->color[3] = 1.0f;
}

void Gfx3D_End(void)
{
    if (!s_recording || s_vboCount == 0)
    {
        s_recording = 0;
        s_vboCount  = 0;
        return;
    }

    C3D_BindProgram(&s_program);

    C3D_TexEnv* texEnv = C3D_GetTexEnv(0);
    C3D_TexEnvInit(texEnv);
    C3D_TexEnvSrc(texEnv, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(texEnv, C3D_Both, GPU_REPLACE);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4);

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, s_vbo, sizeof(Gfx3DVertex), 2, 0x10);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s_uLocProjection, &s_projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, s_uLocModelView, &s_modelView);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    C3D_DrawArrays(s_primType, 0, s_vboCount);

    s_recording = 0;
    s_vboCount  = 0;
}