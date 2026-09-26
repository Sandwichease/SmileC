#include "keyboard.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

#define CELL_W (320.0f / KB_COLS)
#define CELL_H ((240.0f - KB_TOOLBAR_H) / KB_ROWS)

// --- Page lettres -----------------------------------------------------
static const KeyDef kLettersLayout[] = {
    {"q", KA_CHAR, 'q', 0, 0, 1}, {"w", KA_CHAR, 'w', 1, 0, 1},
    {"e", KA_CHAR, 'e', 2, 0, 1}, {"r", KA_CHAR, 'r', 3, 0, 1},
    {"t", KA_CHAR, 't', 4, 0, 1}, {"y", KA_CHAR, 'y', 5, 0, 1},
    {"u", KA_CHAR, 'u', 6, 0, 1}, {"i", KA_CHAR, 'i', 7, 0, 1},
    {"o", KA_CHAR, 'o', 8, 0, 1}, {"p", KA_CHAR, 'p', 9, 0, 1},

    {"a", KA_CHAR, 'a', 0, 1, 1}, {"s", KA_CHAR, 's', 1, 1, 1},
    {"d", KA_CHAR, 'd', 2, 1, 1}, {"f", KA_CHAR, 'f', 3, 1, 1},
    {"g", KA_CHAR, 'g', 4, 1, 1}, {"h", KA_CHAR, 'h', 5, 1, 1},
    {"j", KA_CHAR, 'j', 6, 1, 1}, {"k", KA_CHAR, 'k', 7, 1, 1},
    {"l", KA_CHAR, 'l', 8, 1, 1}, {"<-", KA_BACKSPACE, 0, 9, 1, 1},

    {"SHIFT", KA_SHIFT, 0, 0, 2, 2},
    {"z", KA_CHAR, 'z', 2, 2, 1}, {"x", KA_CHAR, 'x', 3, 2, 1},
    {"c", KA_CHAR, 'c', 4, 2, 1}, {"v", KA_CHAR, 'v', 5, 2, 1},
    {"b", KA_CHAR, 'b', 6, 2, 1}, {"n", KA_CHAR, 'n', 7, 2, 1},
    {"m", KA_CHAR, 'm', 8, 2, 1}, {",", KA_CHAR, ',', 9, 2, 1},

    {"123", KA_TOGGLE_SYMBOLS, 0, 0, 3, 2},
    {"SPACE", KA_SPACE, ' ', 2, 3, 5},
    {".", KA_CHAR, '.', 7, 3, 1},
    {"ENTER", KA_ENTER, 0, 8, 3, 2},

    // row 4 : TAB  RUN(2)  SAVE(3)  <  >  ^  v
    {"TAB", KA_TAB, 0, 0, 4, 1},
    {"RUN", KA_RUN, 0, 1, 4, 2},
    {"SAVE", KA_SAVE, 0, 3, 4, 3},
    {"<", KA_LEFT, 0, 6, 4, 1},
    {">", KA_RIGHT, 0, 7, 4, 1},
    {"^", KA_UP, 0, 8, 4, 1},
    {"v", KA_DOWN, 0, 9, 4, 1},
};
#define N_LETTERS (sizeof(kLettersLayout) / sizeof(kLettersLayout[0]))

// --- Page symboles / chiffres -----------------------------------------
static const KeyDef kSymbolsLayout[] = {
    {"1", KA_CHAR, '1', 0, 0, 1}, {"2", KA_CHAR, '2', 1, 0, 1},
    {"3", KA_CHAR, '3', 2, 0, 1}, {"4", KA_CHAR, '4', 3, 0, 1},
    {"5", KA_CHAR, '5', 4, 0, 1}, {"6", KA_CHAR, '6', 5, 0, 1},
    {"7", KA_CHAR, '7', 6, 0, 1}, {"8", KA_CHAR, '8', 7, 0, 1},
    {"9", KA_CHAR, '9', 8, 0, 1}, {"0", KA_CHAR, '0', 9, 0, 1},

    {"{", KA_CHAR, '{', 0, 1, 1}, {"}", KA_CHAR, '}', 1, 1, 1},
    {"(", KA_CHAR, '(', 2, 1, 1}, {")", KA_CHAR, ')', 3, 1, 1},
    {"[", KA_CHAR, '[', 4, 1, 1}, {"]", KA_CHAR, ']', 5, 1, 1},
    {"<", KA_CHAR, '<', 6, 1, 1}, {">", KA_CHAR, '>', 7, 1, 1},
    {"=", KA_CHAR, '=', 8, 1, 1}, {";", KA_CHAR, ';', 9, 1, 1},

    {"\"", KA_CHAR, '"', 0, 2, 1}, {"'", KA_CHAR, '\'', 1, 2, 1},
    {"#", KA_CHAR, '#', 2, 2, 1}, {"_", KA_CHAR, '_', 3, 2, 1},
    {"+", KA_CHAR, '+', 4, 2, 1}, {"-", KA_CHAR, '-', 5, 2, 1},
    {"*", KA_CHAR, '*', 6, 2, 1}, {"/", KA_CHAR, '/', 7, 2, 1},
    {"<-", KA_BACKSPACE, 0, 8, 2, 2},

    {"EXT", KA_TOGGLE_SYMBOLS, 0, 0, 3, 2},
    {"SPACE", KA_SPACE, ' ', 2, 3, 5},
    {",", KA_CHAR, ',', 7, 3, 1},
    {"ENTER", KA_ENTER, 0, 8, 3, 2},

    {"TAB", KA_TAB, 0, 0, 4, 1},
    {"RUN", KA_RUN, 0, 1, 4, 2},
    {"SAVE", KA_SAVE, 0, 3, 4, 3},
    {"<", KA_LEFT, 0, 6, 4, 1},
    {">", KA_RIGHT, 0, 7, 4, 1},
    {"^", KA_UP, 0, 8, 4, 1},
    {"v", KA_DOWN, 0, 9, 4, 1},
};
#define N_SYMBOLS (sizeof(kSymbolsLayout) / sizeof(kSymbolsLayout[0]))

// --- Page symboles rares ----------------------------------------------
static const KeyDef kExtraLayout[] = {
    {"%", KA_CHAR, '%', 0, 0, 1}, {"&", KA_CHAR, '&', 1, 0, 1},
    {"|", KA_CHAR, '|', 2, 0, 1}, {":", KA_CHAR, ':', 3, 0, 1},
    {"~", KA_CHAR, '~', 4, 0, 1}, {"^", KA_CHAR, '^', 5, 0, 1},
    {"\\", KA_CHAR, '\\', 6, 0, 1}, {"?", KA_CHAR, '?', 7, 0, 1},
    {"!", KA_CHAR, '!', 8, 0, 1}, {"@", KA_CHAR, '@', 9, 0, 1},

    {"$", KA_CHAR, '$', 0, 1, 1}, {"`", KA_CHAR, '`', 1, 1, 1},
    {"<-", KA_BACKSPACE, 0, 2, 1, 2},
    {"SPACE", KA_SPACE, ' ', 4, 1, 5},
    {".", KA_CHAR, '.', 9, 1, 1},

    {"ABC", KA_TOGGLE_SYMBOLS, 0, 0, 2, 2},
    {"SPACE", KA_SPACE, ' ', 2, 2, 5},
    {",", KA_CHAR, ',', 7, 2, 1},
    {"ENTER", KA_ENTER, 0, 8, 2, 2},

    {"TAB", KA_TAB, 0, 0, 4, 1},
    {"RUN", KA_RUN, 0, 1, 4, 2},
    {"SAVE", KA_SAVE, 0, 3, 4, 3},
    {"<", KA_LEFT, 0, 6, 4, 1},
    {">", KA_RIGHT, 0, 7, 4, 1},
    {"^", KA_UP, 0, 8, 4, 1},
    {"v", KA_DOWN, 0, 9, 4, 1},
};
#define N_EXTRA (sizeof(kExtraLayout) / sizeof(kExtraLayout[0]))

static const KeyDef* CurrentLayout(const Keyboard* kb, size_t* outCount)
{
    switch (kb->page)
    {
        case KB_PAGE_LETTERS: *outCount = N_LETTERS; return kLettersLayout;
        case KB_PAGE_SYMBOLS: *outCount = N_SYMBOLS; return kSymbolsLayout;
        case KB_PAGE_EXTRA:   *outCount = N_EXTRA;   return kExtraLayout;
        default:              *outCount = N_LETTERS; return kLettersLayout;
    }
}

void Keyboard_Init(Keyboard* kb)
{
    kb->page = KB_PAGE_LETTERS;
    kb->shift = 0;
    kb->suggestionCount = 0;
    kb->suggestionIndex = 0;
    for (int i = 0; i < KB_MAX_SUGGESTIONS; i++)
        kb->suggestions[i] = NULL;
}

void Keyboard_SetSuggestions(Keyboard* kb, const char** suggestions, int count)
{
    if (count > KB_MAX_SUGGESTIONS)
        count = KB_MAX_SUGGESTIONS;
    if (count < 0 || suggestions == NULL)
        count = 0;

    kb->suggestionCount = count;
    for (int i = 0; i < KB_MAX_SUGGESTIONS; i++)
        kb->suggestions[i] = (i < count) ? suggestions[i] : NULL;

    // Si la sélection pointait au-delà du nouveau nombre, on la ramène
    // dans les bornes (ou à 0).
    if (kb->suggestionCount == 0)
        kb->suggestionIndex = 0;
    else if (kb->suggestionIndex >= kb->suggestionCount)
        kb->suggestionIndex = 0;
}

void Keyboard_CycleSuggestion(Keyboard* kb)
{
    if (kb->suggestionCount <= 0)
    {
        kb->suggestionIndex = 0;
        return;
    }
    kb->suggestionIndex = (kb->suggestionIndex + 1) % kb->suggestionCount;
}

const char* Keyboard_GetSelectedSuggestion(const Keyboard* kb)
{
    if (kb->suggestionCount <= 0)
        return NULL;
    if (kb->suggestionIndex < 0 || kb->suggestionIndex >= kb->suggestionCount)
        return NULL;
    return kb->suggestions[kb->suggestionIndex];
}

static u32 ColorForAction(KeyAction action)
{
    switch (action)
    {
        case KA_RUN:  return C2D_Color32(0x20, 0x90, 0x30, 0xFF);
        case KA_SAVE: return C2D_Color32(0x20, 0x60, 0x90, 0xFF);
        case KA_DEFAULT: return C2D_Color32(0x90, 0x50, 0x20, 0xFF);
        case KA_TAB: return C2D_Color32(0x50, 0x70, 0x90, 0xFF);
        case KA_COMPLETE: return C2D_Color32(0x70, 0x50, 0x90, 0xFF);
        case KA_SHIFT:
        case KA_TOGGLE_SYMBOLS:
            return C2D_Color32(0x60, 0x60, 0x60, 0xFF);
        case KA_BACKSPACE:
        case KA_ENTER:
            return C2D_Color32(0x50, 0x50, 0x50, 0xFF);
        default:
            return C2D_Color32(0x38, 0x38, 0x38, 0xFF);
    }
}

static void DrawCenteredLabel(C2D_TextBuf textBuf, const char* str,
                              float x, float y, float w, float h, float scale)
{
    C2D_Text text;
    C2D_TextParse(&text, textBuf, str);
    C2D_TextOptimize(&text);

    float tw, th;
    C2D_TextGetDimensions(&text, scale, scale, &tw, &th);
    if (tw > w - 4.0f && tw > 0.0f)
    {
        scale *= (w - 4.0f) / tw;
        C2D_TextGetDimensions(&text, scale, scale, &tw, &th);
    }

    C2D_DrawText(&text, C2D_WithColor, x + (w - tw) / 2.0f, y + (h - th) / 2.0f,
                 0.5f, scale, scale, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
}

void Keyboard_Render(const Keyboard* kb, C2D_TextBuf textBuf)
{
    // --- bandeau du haut : 3 propositions d'autocomplétion -------------
    for (int i = 0; i < KB_MAX_SUGGESTIONS; i++)
    {
        float x = i * KB_SUGGEST_W;
        int filled = (i < kb->suggestionCount) && (kb->suggestions[i] != NULL);
        int selected = filled && (i == kb->suggestionIndex);

        // Slot sélectionné (avec L) : surbrillance jaune.
        u32 bg;
        if (!filled)
            bg = C2D_Color32(0x22, 0x22, 0x22, 0xFF);
        else if (selected)
            bg = C2D_Color32(0xC0, 0x90, 0x30, 0xFF);
        else
            bg = ColorForAction(KA_COMPLETE);

        C2D_DrawRectSolid(x + 1.0f, 1.0f, 0.4f, KB_SUGGEST_W - 2.0f,
                          KB_TOOLBAR_H - 2.0f, bg);
        if (filled)
            DrawCenteredLabel(textBuf, kb->suggestions[i], x + 1.0f, 1.0f,
                              KB_SUGGEST_W - 2.0f, KB_TOOLBAR_H - 2.0f, 0.36f);
    }

    // --- bandeau du haut : bouton DEFAULT à droite ---------------------
    float defaultW = 320.0f - KB_DEFAULT_X;
    C2D_DrawRectSolid(KB_DEFAULT_X + 1.0f, 1.0f, 0.4f, defaultW - 2.0f,
                      KB_TOOLBAR_H - 2.0f, ColorForAction(KA_DEFAULT));
    DrawCenteredLabel(textBuf, "DEFAULT", KB_DEFAULT_X + 1.0f, 1.0f,
                      defaultW - 2.0f, KB_TOOLBAR_H - 2.0f, 0.32f);

    // --- grille du clavier ---------------------------------------------
    size_t count;
    const KeyDef* layout = CurrentLayout(kb, &count);

    for (size_t i = 0; i < count; i++)
    {
        const KeyDef* k = &layout[i];
        float x = k->col * CELL_W;
        float y = KB_TOOLBAR_H + k->row * CELL_H;
        float w = k->colspan * CELL_W - 2.0f;
        float h = CELL_H - 2.0f;

        u32 keyColor = ColorForAction(k->action);
        if (k->action == KA_SHIFT && kb->shift)
            keyColor = C2D_Color32(0x30, 0xA0, 0xD0, 0xFF);

        C2D_DrawRectSolid(x + 1, y + 1, 0.4f, w, h, keyColor);

        char label[8];
        if (k->action == KA_CHAR && k->ch != ' ')
        {
            label[0] = kb->shift ? (char)toupper((unsigned char)k->ch) : k->ch;
            label[1] = '\0';
        }
        else
        {
            int n = 0;
            while (k->label[n] && n < 7) { label[n] = k->label[n]; n++; }
            label[n] = '\0';
        }

        DrawCenteredLabel(textBuf, label, x + 1.0f, y + 1.0f, w, h, 0.45f);
    }
}

KeyAction Keyboard_HitTest(Keyboard* kb, int touchX, int touchY, char* outCh)
{
    if (touchY >= 0 && touchY < KB_TOOLBAR_H)
    {
        if ((float)touchX >= KB_DEFAULT_X)
            return KA_DEFAULT;

        int index = (int)((float)touchX / KB_SUGGEST_W);
        if (index >= 0 && index < kb->suggestionCount &&
            kb->suggestions[index] != NULL)
        {
            kb->suggestionIndex = index;
            return KA_COMPLETE;
        }
        return KA_NONE;
    }

    size_t count;
    const KeyDef* layout = CurrentLayout(kb, &count);

    int col = (int)(touchX / CELL_W);
    int row = (int)((touchY - KB_TOOLBAR_H) / CELL_H);

    for (size_t i = 0; i < count; i++)
    {
        const KeyDef* k = &layout[i];
        if (row == k->row && col >= k->col && col < k->col + k->colspan)
        {
            switch (k->action)
            {
                case KA_SHIFT:
                    kb->shift = !kb->shift;
                    return KA_SHIFT;

                case KA_TOGGLE_SYMBOLS:
                    if (kb->page == KB_PAGE_LETTERS)
                        kb->page = KB_PAGE_SYMBOLS;
                    else if (kb->page == KB_PAGE_SYMBOLS)
                        kb->page = KB_PAGE_EXTRA;
                    else
                        kb->page = KB_PAGE_LETTERS;
                    return KA_TOGGLE_SYMBOLS;

                case KA_CHAR:
                    if (outCh)
                        *outCh = kb->shift ? (char)toupper((unsigned char)k->ch) : k->ch;
                    kb->shift = 0;
                    return KA_CHAR;

                case KA_SPACE:
                    if (outCh) *outCh = ' ';
                    return KA_SPACE;

                default:
                    return k->action;
            }
        }
    }
    return KA_NONE;
}