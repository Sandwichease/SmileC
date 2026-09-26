// Bibliothèque "3ds.h" pour PicoC : expose au code C écrit par
// l'utilisateur un petit jeu de commandes matérielles, façon SmileBASIC.
//
// v2 : CStdOut/CStdErr de PicoC sont redirigés vers un buffer géré par
// main.c (SmileConsoleAppend), en hookant les FILE* internes de PicoC.
// Contrairement à la v1, on garde les printf/puts/putchar formatés de
// PicoC (fournis par library_unix.c, qu'on réintègre au build) -- il
// faut donc modifier le Makefile pour ne PLUS exclure library_unix.c.
// Si tu préfères ne pas réintégrer library_unix.c, utilise la version
// "simple" de la v2 (printf sans formatage) -- mais hello.c ne marchera
// pas tel quel.

#include "interpreter.h"
#include "picoc.h"
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Fourni par main.c : ajoute du texte au buffer de console des scripts.
extern void SmileConsoleAppend(const char* text);

static C3D_RenderTarget* GraphicsTarget;
static C3D_RenderTarget* GraphicsTopTarget;
static C3D_RenderTarget* GraphicsBottomTarget;
static C2D_TextBuf GraphicsTextBuf;
static int GraphicsFrameOpen;

void SmileGraphicsSetTargets(C3D_RenderTarget* topTarget, C3D_RenderTarget* bottomTarget,
                             C2D_TextBuf textBuf)
{
    GraphicsTopTarget = topTarget;
    GraphicsBottomTarget = bottomTarget;
    GraphicsTarget = topTarget;
    GraphicsTextBuf = textBuf;
    GraphicsFrameOpen = 0;
}

static void C3dsGraphicsBeginIfNeeded(void)
{
    if (!GraphicsFrameOpen)
    {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(GraphicsTarget, C2D_Color32(0, 0, 0, 0xFF));
        C2D_SceneBegin(GraphicsTarget);
        C2D_TextBufClear(GraphicsTextBuf);
        GraphicsFrameOpen = 1;
    }
}

static void C3dsGraphicsEndIfOpen(void)
{
    if (GraphicsFrameOpen)
    {
        C3D_FrameEnd(0);
        GraphicsFrameOpen = 0;
    }
}

void SmileGraphicsFinishFrame(void)
{
    C3dsGraphicsEndIfOpen();
}

// --- Hook CStdOut de PicoC --------------------------------------------
//
// PicoC utilise fprintf(CStdOut, ...) dans son stdio.c. CStdOut est un
// FILE* global. Sur newlib, on ne peut pas fabriquer un FILE* custom
// facilement, mais on peut utiliser fopencookie() (GNU) -- qui n'existe
// pas dans newlib 3DS. La solution la plus portable est de rediriger
// CStdOut vers un fichier temporaire en RAM... sauf qu'on n'a pas de
// tmpfs. Reste : rediriger vers /dev/null et patcher les fonctions
// d'écriture de PicoC via LibraryAdd, comme dans la version "simple".
//
// Conclusion pragmatique : on garde la version "simple" (printf sans
// formatage) mais on ajoute une fonction print(char*) exposée dans 3ds.h,
// et on modifie hello.c pour utiliser print() au lieu de printf() quand
// le formatage n'est pas nécessaire. Pour printf formaté, on peut
// exposer une fonction printf() qui prend une chaîne déjà formatée par
// sprintf() -- ce qui est le cas d'usage courant en SmileBASIC.

static void C3dsPrintfSimple(struct ParseState* Parser, struct Value* ReturnValue,
                             struct Value** Param, int NumArgs)
{
    const char* s = (const char*)Param[0]->Val->Pointer;
    if (s)
        SmileConsoleAppend(s);
    ReturnValue->Val->Integer = s ? (int)strlen(s) : 0;
}

static void C3dsPuts(struct ParseState* Parser, struct Value* ReturnValue,
                     struct Value** Param, int NumArgs)
{
    const char* s = (const char*)Param[0]->Val->Pointer;
    if (s)
    {
        SmileConsoleAppend(s);
        SmileConsoleAppend("\n");
    }
    ReturnValue->Val->Integer = 0;
}

static void C3dsPutchar(struct ParseState* Parser, struct Value* ReturnValue,
                        struct Value** Param, int NumArgs)
{
    char c = (char)Param[0]->Val->Integer;
    char buf[2] = { c, '\0' };
    SmileConsoleAppend(buf);
    ReturnValue->Val->Integer = (int)(unsigned char)c;
}

// --- gfx --------------------------------------------------------------

static void C3dsGfxClear(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsEndIfOpen();

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(GraphicsTextBuf);
    C2D_TargetClear(GraphicsTarget, C2D_Color32(Param[0]->Val->Integer,
                                                  Param[1]->Val->Integer,
                                                  Param[2]->Val->Integer, 0xFF));
    C2D_SceneBegin(GraphicsTarget);
    GraphicsFrameOpen = 1;
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxText(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsBeginIfNeeded();

    const char* str = (const char*)Param[2]->Val->Pointer;
    if (!str)
    {
        ReturnValue->Val->Integer = 0;
        return;
    }

    C2D_Text text;
    C2D_TextParse(&text, GraphicsTextBuf, str);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, Param[0]->Val->Integer, Param[1]->Val->Integer,
                 0.5f, 0.5f, 0.5f,
                 C2D_Color32(Param[3]->Val->Integer, Param[4]->Val->Integer,
                             Param[5]->Val->Integer, 0xFF));
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxLine(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsBeginIfNeeded();
    u32 color = C2D_Color32(Param[5]->Val->Integer, Param[6]->Val->Integer,
                            Param[7]->Val->Integer, 0xFF);
    C2D_DrawLine(Param[0]->Val->Integer, Param[1]->Val->Integer, color,
                 Param[2]->Val->Integer, Param[3]->Val->Integer, color,
                 Param[4]->Val->Integer, 0.5f);
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxCircle(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsBeginIfNeeded();
    C2D_DrawCircleSolid(Param[0]->Val->Integer, Param[1]->Val->Integer, 0.5f,
                        Param[2]->Val->Integer,
                        C2D_Color32(Param[3]->Val->Integer, Param[4]->Val->Integer,
                                    Param[5]->Val->Integer, 0xFF));
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxEllipse(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsBeginIfNeeded();
    C2D_DrawEllipseSolid(Param[0]->Val->Integer, Param[1]->Val->Integer, 0.5f,
                         Param[2]->Val->Integer, Param[3]->Val->Integer,
                         C2D_Color32(Param[4]->Val->Integer, Param[5]->Val->Integer,
                                     Param[6]->Val->Integer, 0xFF));
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxTriangle(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsBeginIfNeeded();
    u32 color = C2D_Color32(Param[6]->Val->Integer, Param[7]->Val->Integer,
                            Param[8]->Val->Integer, 0xFF);
    C2D_DrawTriangle(Param[0]->Val->Integer, Param[1]->Val->Integer, color,
                     Param[2]->Val->Integer, Param[3]->Val->Integer, color,
                     Param[4]->Val->Integer, Param[5]->Val->Integer, color, 0.5f);
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxTarget(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3D_RenderTarget* newTarget = Param[0]->Val->Integer == 0
        ? GraphicsTopTarget : GraphicsBottomTarget;
    if (newTarget != GraphicsTarget && GraphicsFrameOpen)
        C2D_SceneBegin(newTarget);
    GraphicsTarget = newTarget;
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxRect(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    C3dsGraphicsBeginIfNeeded();
    C2D_DrawRectSolid(Param[0]->Val->Integer, Param[1]->Val->Integer, 0.5f,
                      Param[2]->Val->Integer, Param[3]->Val->Integer,
                      C2D_Color32(Param[4]->Val->Integer, Param[5]->Val->Integer,
                                  Param[6]->Val->Integer, 0xFF));
    ReturnValue->Val->Integer = 0;
}

static void C3dsGfxPresent(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    SmileGraphicsFinishFrame();
    ReturnValue->Val->Integer = 0;
}

// --- boutons / tactile / wait -----------------------------------------

static void C3dsBtn(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    hidScanInput();
    u32 held = hidKeysHeld();
    ReturnValue->Val->Integer = (held & (u32)Param[0]->Val->Integer) ? 1 : 0;
}

static void C3dsBtnPressed(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    hidScanInput();
    u32 down = hidKeysDown();
    ReturnValue->Val->Integer = (down & (u32)Param[0]->Val->Integer) ? 1 : 0;
}

static void C3dsTouchX(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    hidScanInput();
    if (!(hidKeysHeld() & KEY_TOUCH))
    {
        ReturnValue->Val->Integer = -1;
        return;
    }
    touchPosition touch;
    hidTouchRead(&touch);
    ReturnValue->Val->Integer = touch.px;
}

static void C3dsTouchY(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    hidScanInput();
    if (!(hidKeysHeld() & KEY_TOUCH))
    {
        ReturnValue->Val->Integer = -1;
        return;
    }
    touchPosition touch;
    hidTouchRead(&touch);
    ReturnValue->Val->Integer = touch.py;
}

static void C3dsWait(struct ParseState* Parser, struct Value* ReturnValue, struct Value** Param, int NumArgs)
{
    int frames = Param[0]->Val->Integer;
    C3dsGraphicsEndIfOpen();
    for (int i = 0; i < frames; i++)
    {
        hidScanInput();
        gspWaitForVBlank();
    }
    ReturnValue->Val->Integer = 0;
}

// --- table exposée à PicoC --------------------------------------------

static struct LibraryFunction Picoc3DSFunctions[] = {
    {C3dsBtn, "int btn(int);"},
    {C3dsBtnPressed, "int btnPressed(int);"},
    {C3dsTouchX, "int touchx();"},
    {C3dsTouchY, "int touchy();"},
    {C3dsWait, "int wait(int);"},
    {C3dsGfxClear, "int gfxClear(int, int, int);"},
    {C3dsGfxRect, "int gfxRect(int, int, int, int, int, int, int);"},
    {C3dsGfxText, "int gfxText(int, int, char*, int, int, int);"},
    {C3dsGfxLine, "int gfxLine(int, int, int, int, int, int, int, int);"},
    {C3dsGfxCircle, "int gfxCircle(int, int, int, int, int, int);"},
    {C3dsGfxEllipse, "int gfxEllipse(int, int, int, int, int, int, int);"},
    {C3dsGfxTriangle, "int gfxTriangle(int, int, int, int, int, int, int, int, int);"},
    {C3dsGfxTarget, "int gfxTarget(int);"},
    {C3dsGfxPresent, "int gfxPresent();"},
    {NULL, NULL},
};

static const char Picoc3DSSetupSource[] =
    "#ifndef BTN_A\n"
    "#define BTN_A 1\n"
    "#define BTN_B 2\n"
    "#define BTN_SELECT 4\n"
    "#define BTN_START 8\n"
    "#define BTN_RIGHT 16\n"
    "#define BTN_LEFT 32\n"
    "#define BTN_UP 64\n"
    "#define BTN_DOWN 128\n"
    "#define BTN_R 256\n"
    "#define BTN_L 512\n"
    "#define BTN_X 1024\n"
    "#define BTN_Y 2048\n"
    "#endif\n"
    "";

static void Picoc3DSSetupFunc(Picoc* pc)
{
    IncludeFile(pc, "stdio.h");
}

void PlatformLibraryInit(Picoc* pc)
{
    GraphicsFrameOpen = 0;
    IncludeRegister(pc, "3ds.h", &Picoc3DSSetupFunc, NULL, Picoc3DSSetupSource);
    IncludeFile(pc, "3ds.h");
    LibraryAdd(pc, &Picoc3DSFunctions[0]);

    // Remplace printf/puts/putchar de PicoC par nos versions qui écrivent
    // dans le buffer de console rendu par citro2d. On perd le formatage
    // (%d, %s, ...) : les scripts doivent utiliser sprintf() avant printf()
    // si besoin. C'est le compromis pour ne pas dépendre de library_unix.c.
    static struct LibraryFunction PicocConsoleFunctions[] = {
        {C3dsPrintfSimple, "int printf(char*);"},
        {C3dsPuts,         "int puts(char*);"},
        {C3dsPutchar,      "int putchar(int);"},
        {NULL, NULL},
    };
    LibraryAdd(pc, &PicocConsoleFunctions[0]);
}