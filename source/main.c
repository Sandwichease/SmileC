#include <3ds.h>
#include <citro2d.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "editor.h"
#include "keyboard.h"
#include "gfx3d.h"
#include "picoc.h"

#define PICOC_STACK_SIZE (512 * 1024)
#define PROGRAM_PARENT "sdmc:/3ds"
#define PROGRAM_DIR  "sdmc:/3ds/smileclang"
#define PROGRAM_PATH "sdmc:/3ds/smileclang/program.c"

#define SCRIPT_CONSOLE_CAPACITY (16 * 1024)

extern void SmileGraphicsFinishFrame(void);
extern void SmileGraphicsSetTargets(C3D_RenderTarget* topTarget,
                                    C3D_RenderTarget* bottomTarget,
                                    C2D_TextBuf textBuf);
extern void SmileConsoleAppend(const char* text);

static Editor editor;
static const char* editorStatus = "";
static const char defaultProgram[] =
    "#include \"3ds.h\"\n"
    "\n"
    "int main()\n"
    "{\n"
    "    int x = 20;\n"
    "    int dx = 3;\n"
    "    while (!btn(BTN_START))\n"
    "    {\n"
    "        gfxClear(10, 10, 25);\n"
    "        gfxText(16, 16, \"START: retour editeur\", 255, 255, 255);\n"
    "        gfxCircle(x, 120, 20, 40, 220, 120);\n"
    "        gfxPresent();\n"
    "        x += dx;\n"
    "        if (x >= 380) dx = -3;\n"
    "        if (x <= 20) dx = 3;\n"
    "        wait(1);\n"
    "    }\n"
    "    return 0;\n"
    "}\n";

static char  s_consoleBuffer[SCRIPT_CONSOLE_CAPACITY];
static int   s_consoleLength = 0;

void SmileConsoleAppend(const char* text)
{
    if (!text) return;
    size_t n = strlen(text);
    if (n == 0) return;
    if (s_consoleLength + (int)n >= SCRIPT_CONSOLE_CAPACITY)
    {
        int keep = SCRIPT_CONSOLE_CAPACITY / 2;
        memmove(s_consoleBuffer, s_consoleBuffer + s_consoleLength - keep,
                (size_t)keep);
        s_consoleLength = keep;
    }
    if (s_consoleLength + (int)n >= SCRIPT_CONSOLE_CAPACITY)
        n = (size_t)(SCRIPT_CONSOLE_CAPACITY - 1 - s_consoleLength);
    memcpy(s_consoleBuffer + s_consoleLength, text, n);
    s_consoleLength += (int)n;
    s_consoleBuffer[s_consoleLength] = '\0';
}

static void SmileConsoleClear(void)
{
    s_consoleLength = 0;
    s_consoleBuffer[0] = '\0';
}

static void RenderScriptConsole(C2D_TextBuf textBuf)
{
    if (s_consoleLength == 0)
        return;

    enum { MAX_VISIBLE_LINES = 30 };
    const char* lines[MAX_VISIBLE_LINES];
    int lineCount = 0;

    const char* p = s_consoleBuffer;
    while (*p)
    {
        if (lineCount < MAX_VISIBLE_LINES)
        {
            lines[lineCount++] = p;
        }
        else
        {
            memmove(&lines[0], &lines[1],
                    sizeof(lines[0]) * (MAX_VISIBLE_LINES - 1));
            lines[MAX_VISIBLE_LINES - 1] = p;
        }
        const char* nl = strchr(p, '\n');
        if (!nl)
            break;
        p = nl + 1;
    }

    float y = 4.0f;
    for (int i = 0; i < lineCount; i++)
    {
        const char* start = lines[i];
        const char* end = strchr(start, '\n');
        char lineBuf[256];
        int len = end ? (int)(end - start) : (int)strlen(start);
        if (len > (int)sizeof(lineBuf) - 1)
            len = (int)sizeof(lineBuf) - 1;
        memcpy(lineBuf, start, (size_t)len);
        lineBuf[len] = '\0';

        C2D_Text text;
        C2D_TextParse(&text, textBuf, lineBuf);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 4.0f, y, 0.5f, 0.42f, 0.42f,
                     C2D_Color32(0xE0, 0xE0, 0xE0, 0xFF));
        y += 14.0f;
        if (y > 230.0f)
            break;
    }
}

static int EnsureSaveDirectory(void)
{
    if (mkdir(PROGRAM_PARENT, 0777) != 0 && errno != EEXIST)
        return -1;
    if (mkdir(PROGRAM_DIR, 0777) != 0 && errno != EEXIST)
        return -1;
    return 0;
}

static void LoadDefaultProgram(Editor* ed)
{
    ed->length = (int)(sizeof(defaultProgram) - 1);
    memcpy(ed->buffer, defaultProgram, sizeof(defaultProgram));
    ed->cursor = ed->length;
    ed->scrollLine = 0;
}

static void InitGraphics(C3D_RenderTarget** topTarget, C3D_RenderTarget** botTarget,
                         C2D_TextBuf* textBuf)
{
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    *topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    *botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    *textBuf = C2D_TextBufNew(4096);
}

static void ShutdownGraphics(C2D_TextBuf* textBuf)
{
    C2D_TextBufDelete(*textBuf);
    *textBuf = NULL;
    C2D_Fini();
    C3D_Fini();
}

static void RunProgram(C3D_RenderTarget** topTarget, C3D_RenderTarget** botTarget,
                       C2D_TextBuf* textBuf)
{
    (void)topTarget;
    (void)botTarget;
    int exitCode = 0;
    int programFailed = 0;

    SmileConsoleClear();

    while (aptMainLoop() && !programFailed)
    {
        Picoc pc;
        hidScanInput();
        if (hidKeysHeld() & KEY_START)
            break;

        PicocInitialize(&pc, PICOC_STACK_SIZE);
        if (!PicocPlatformSetExitPoint(&pc))
        {
            PicocPlatformScanFile(&pc, PROGRAM_PATH);
            PicocCallMain(&pc, 0, NULL);
        }

        SmileGraphicsFinishFrame();
        exitCode = pc.PicocExitValue;
        programFailed = exitCode != 0;
        PicocCleanup(&pc);
        gspWaitForVBlank();
    }

    editorStatus = programFailed ? "Program error (voir console)"
                                 : "Program stopped (START)";
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    gfxInitDefault();
    C3D_RenderTarget* topTarget;
    C3D_RenderTarget* botTarget;

    C2D_TextBuf textBuf;
    InitGraphics(&topTarget, &botTarget, &textBuf);
    SmileGraphicsSetTargets(topTarget, botTarget, textBuf);

    if (Gfx3D_Init() != 0)
        editorStatus = "3D init failed";

    if (EnsureSaveDirectory() != 0)
        editorStatus = "Save folder failed";

    Editor_Init(&editor);
    if (Editor_LoadFromFile(&editor, PROGRAM_PATH) != 0)
    {
        LoadDefaultProgram(&editor);
        editorStatus = "Default test program";
    }

    Keyboard keyboard;
    Keyboard_Init(&keyboard);

    while (aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START)
            break; // quitte l'appli

        // --- Boutons physiques ---------------------------------------
        // Croix directionnelle : déplacement du curseur dans l'éditeur.
        // On utilise kDown (front montant) pour un déplacement propre.
        if (kDown & KEY_DLEFT)  Editor_MoveCursorLeft(&editor);
        if (kDown & KEY_DRIGHT) Editor_MoveCursorRight(&editor);
        if (kDown & KEY_DUP)    Editor_MoveCursorUp(&editor);
        if (kDown & KEY_DDOWN)  Editor_MoveCursorDown(&editor);

        // L : cycle vers la suggestion suivante
        if (kDown & KEY_L)
            Keyboard_CycleSuggestion(&keyboard);

        // R : valide la suggestion sélectionnée
        if (kDown & KEY_R)
        {
            const char* sel = Keyboard_GetSelectedSuggestion(&keyboard);
            if (sel)
            {
                Editor_AcceptSuggestion(&editor, keyboard.suggestionIndex);
                editorStatus = "Suggestion acceptee";
            }
        }

        // --- Écran tactile -------------------------------------------
        if (kDown & KEY_TOUCH)
        {
            touchPosition touch;
            hidTouchRead(&touch);

            char ch = 0;
            KeyAction action = Keyboard_HitTest(&keyboard, touch.px, touch.py, &ch);
            switch (action)
            {
                case KA_CHAR:      Editor_InsertChar(&editor, ch); break;
                case KA_SPACE:     Editor_InsertChar(&editor, ' '); break;
                case KA_ENTER:     Editor_InsertNewline(&editor); break;
                case KA_BACKSPACE: Editor_Backspace(&editor); break;
                case KA_LEFT:      Editor_MoveCursorLeft(&editor); break;
                case KA_RIGHT:     Editor_MoveCursorRight(&editor); break;
                case KA_UP:        Editor_MoveCursorUp(&editor); break;
                case KA_DOWN:      Editor_MoveCursorDown(&editor); break;
                case KA_SAVE:
                    if (EnsureSaveDirectory() != 0)
                        editorStatus = "Save folder failed";
                    else
                        editorStatus = Editor_SaveToFile(&editor, PROGRAM_PATH) == 0
                                           ? "Saved"
                                           : "Save failed";
                    break;
                case KA_DEFAULT:
                    LoadDefaultProgram(&editor);
                    editorStatus = "Default loaded";
                    break;
                case KA_TAB:
                    Editor_InsertTab(&editor);
                    break;
                case KA_COMPLETE:
                    Editor_AcceptSuggestion(&editor, keyboard.suggestionIndex);
                    break;
                case KA_RUN:
                    if (EnsureSaveDirectory() != 0)
                        editorStatus = "Save folder failed";
                    else if (Editor_SaveToFile(&editor, PROGRAM_PATH) == 0)
                        RunProgram(&topTarget, &botTarget, &textBuf);
                    else
                        editorStatus = "Save failed";
                    break;
                default:
                    break;
            }
        }

        // --- Suggestions --------------------------------------------
        const char* suggestions[KB_MAX_SUGGESTIONS];
        int suggestionCount = Editor_GetSuggestions(&editor, suggestions,
                                                    KB_MAX_SUGGESTIONS);
        Keyboard_SetSuggestions(&keyboard, suggestions, suggestionCount);

        // --- Rendu ---------------------------------------------------
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TextBufClear(textBuf);

        C2D_TargetClear(topTarget, C2D_Color32(0x10, 0x10, 0x18, 0xFF));
        C2D_SceneBegin(topTarget);

        if (s_consoleLength > 0)
            RenderScriptConsole(textBuf);
        else
            Editor_Render(&editor, textBuf, 4.0f, 4.0f, 14.0f, 16);

        C2D_Text statusText;
        C2D_TextParse(&statusText, textBuf, editorStatus);
        C2D_TextOptimize(&statusText);
        C2D_DrawText(&statusText, C2D_WithColor, 4.0f, 230.0f, 0.5f, 0.35f, 0.35f,
                 C2D_Color32(0xFF, 0xC0, 0x40, 0xFF));

        C2D_TargetClear(botTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
        C2D_SceneBegin(botTarget);
        Keyboard_Render(&keyboard, textBuf);

        C3D_FrameEnd(0);

        (void)kHeld; // pas utilisé pour l'instant, mais dispo pour du repeat
    }

    Gfx3D_Exit();
    ShutdownGraphics(&textBuf);
    gfxExit();
    return 0;
}