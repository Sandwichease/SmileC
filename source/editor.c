#include "editor.h"
#include <string.h>
#include <stdio.h>

void Editor_Init(Editor* ed)
{
    ed->length = 0;
    ed->cursor = 0;
    ed->scrollLine = 0;
    ed->buffer[0] = '\0';
}

void Editor_Clear(Editor* ed)
{
    Editor_Init(ed);
}

void Editor_InsertChar(Editor* ed, char c)
{
    if (ed->length >= EDITOR_BUFFER_CAPACITY - 1)
        return; // buffer plein, on ignore silencieusement

    memmove(&ed->buffer[ed->cursor + 1], &ed->buffer[ed->cursor],
            ed->length - ed->cursor);
    ed->buffer[ed->cursor] = c;
    ed->length++;
    ed->cursor++;
    ed->buffer[ed->length] = '\0';
}

void Editor_InsertNewline(Editor* ed)
{
    Editor_InsertChar(ed, '\n');
}

void Editor_InsertTab(Editor* ed)
{
    for (int i = 0; i < 4; i++)
        Editor_InsertChar(ed, ' ');
}

// Retourne le début du "mot" en cours de frappe. Inclut '#' pour que
// "#inc" soit complété en "#include" (sinon le préfixe serait "inc" et
// ne matcherait jamais "#include").
static int CurrentWordStart(const Editor* ed)
{
    int start = ed->cursor;
    while (start > 0)
    {
        char c = ed->buffer[start - 1];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '#'))
            break;
        start--;
    }
    return start;
}

int Editor_GetSuggestions(const Editor* ed, const char** out, int maxOut)
{
    static const char* words[] = {
        "#include", "int", "void", "char", "return", "if", "else",
        "while", "for", "printf", "sprintf", "btn", "btnPressed",
        "touchx", "touchy", "wait", "gfxClear", "gfxText", "gfxRect",
        "gfxLine", "gfxCircle", "gfxEllipse", "gfxTriangle", "gfxPresent",
        "gfxTarget",
        "BTN_A", "BTN_B", "BTN_X", "BTN_Y", "BTN_START", "BTN_SELECT",
        "BTN_UP", "BTN_DOWN", "BTN_LEFT", "BTN_RIGHT", "BTN_L", "BTN_R"
    };
    char prefix[64];
    int count = 0;

    if (out == NULL || maxOut <= 0)
        return 0;

    int start = CurrentWordStart(ed);
    int length = ed->cursor - start;
    if (length <= 0 || length >= (int)sizeof(prefix))
        return 0;

    memcpy(prefix, &ed->buffer[start], (size_t)length);
    prefix[length] = '\0';

    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++)
    {
        if (strncmp(words[i], prefix, (size_t)length) == 0 &&
            strcmp(words[i], prefix) != 0)
        {
            out[count++] = words[i];
            if (count >= maxOut)
                break;
        }
    }
    return count;
}

void Editor_AcceptSuggestion(Editor* ed, int index)
{
    const char* suggestions[EDITOR_MAX_SUGGESTIONS];
    int count = Editor_GetSuggestions(ed, suggestions, EDITOR_MAX_SUGGESTIONS);
    if (index < 0 || index >= count)
        return;

    const char* suggestion = suggestions[index];
    int start = CurrentWordStart(ed);
    int typedLength = ed->cursor - start;
    while (suggestion[typedLength] != '\0')
        Editor_InsertChar(ed, suggestion[typedLength++]);
}

void Editor_Backspace(Editor* ed)
{
    if (ed->cursor <= 0)
        return;

    memmove(&ed->buffer[ed->cursor - 1], &ed->buffer[ed->cursor],
            ed->length - ed->cursor);
    ed->length--;
    ed->cursor--;
    ed->buffer[ed->length] = '\0';
}

void Editor_MoveCursorLeft(Editor* ed)
{
    if (ed->cursor > 0)
        ed->cursor--;
}

void Editor_MoveCursorRight(Editor* ed)
{
    if (ed->cursor < ed->length)
        ed->cursor++;
}

static int LineStart(const Editor* ed, int pos)
{
    while (pos > 0 && ed->buffer[pos - 1] != '\n')
        pos--;
    return pos;
}

static int LineEnd(const Editor* ed, int start)
{
    int pos = start;
    while (pos < ed->length && ed->buffer[pos] != '\n')
        pos++;
    return pos;
}

void Editor_MoveCursorUp(Editor* ed)
{
    int curLineStart = LineStart(ed, ed->cursor);
    if (curLineStart == 0)
        return;

    int column = ed->cursor - curLineStart;
    int prevLineEnd = curLineStart - 1;
    int prevLineStart = LineStart(ed, prevLineEnd);
    int prevLineLen = prevLineEnd - prevLineStart;

    ed->cursor = prevLineStart + (column < prevLineLen ? column : prevLineLen);
}

void Editor_MoveCursorDown(Editor* ed)
{
    int curLineStart = LineStart(ed, ed->cursor);
    int curLineEnd = LineEnd(ed, curLineStart);
    if (curLineEnd >= ed->length)
        return;

    int column = ed->cursor - curLineStart;
    int nextLineStart = curLineEnd + 1;
    int nextLineEnd = LineEnd(ed, nextLineStart);
    int nextLineLen = nextLineEnd - nextLineStart;

    ed->cursor = nextLineStart + (column < nextLineLen ? column : nextLineLen);
}

int Editor_SaveToFile(const Editor* ed, const char* path)
{
    FILE* f = fopen(path, "w");
    if (!f)
        return -1;
    size_t written = fwrite(ed->buffer, 1, (size_t)ed->length, f);
    fclose(f);
    return written == (size_t)ed->length ? 0 : -1;
}

int Editor_LoadFromFile(Editor* ed, const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f)
        return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0 || size >= EDITOR_BUFFER_CAPACITY)
    {
        fclose(f);
        return -1;
    }

    size_t readBytes = fread(ed->buffer, 1, (size_t)size, f);
    fclose(f);

    ed->length = (int)readBytes;
    ed->buffer[ed->length] = '\0';
    ed->cursor = ed->length;
    ed->scrollLine = 0;
    return 0;
}

static int LineNumberOf(const Editor* ed, int pos)
{
    int line = 0;
    for (int i = 0; i < pos; i++)
        if (ed->buffer[i] == '\n')
            line++;
    return line;
}

void Editor_Render(Editor* ed, C2D_TextBuf textBuf,
                    float x, float y, float lineHeight, int visibleLines)
{
    int cursorLine = LineNumberOf(ed, ed->cursor);
    if (cursorLine < ed->scrollLine)
        ed->scrollLine = cursorLine;
    if (cursorLine >= ed->scrollLine + visibleLines)
        ed->scrollLine = cursorLine - visibleLines + 1;
    if (ed->scrollLine < 0)
        ed->scrollLine = 0;

    int pos = 0;
    int line = 0;

    while (line < ed->scrollLine && pos < ed->length)
    {
        if (ed->buffer[pos] == '\n')
            line++;
        pos++;
    }

    char lineBuf[256];
    for (int row = 0; row < visibleLines && pos <= ed->length; row++)
    {
        int start = pos;
        int len = 0;
        while (pos < ed->length && ed->buffer[pos] != '\n' && len < (int)sizeof(lineBuf) - 1)
        {
            lineBuf[len++] = ed->buffer[pos++];
        }
        lineBuf[len] = '\0';
        while (pos < ed->length && ed->buffer[pos] != '\n')
            pos++;

        float rowY = y + row * lineHeight;

        C2D_Text text;
        C2D_TextParse(&text, textBuf, lineBuf);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, x, rowY, 0.5f, 0.42f, 0.42f,
                     C2D_Color32(0xE0, 0xE0, 0xE0, 0xFF));

        if (line == cursorLine)
        {
            int col = ed->cursor - start;
            if (col >= 0 && col <= len)
            {
                char before[256];
                memcpy(before, lineBuf, col);
                before[col] = '\0';

                C2D_Text beforeText;
                C2D_TextParse(&beforeText, textBuf, before);
                C2D_TextOptimize(&beforeText);
                float w, h;
                C2D_TextGetDimensions(&beforeText, 0.42f, 0.42f, &w, &h);

                C2D_DrawRectSolid(x + w, rowY, 0.5f, 1.5f, lineHeight - 2,
                                   C2D_Color32(0x30, 0xC0, 0x30, 0xFF));
            }
        }

        line++;
        pos++;
    }
}