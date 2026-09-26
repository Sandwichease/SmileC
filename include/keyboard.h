#pragma once
#include <citro2d.h>

typedef enum
{
    KA_NONE = 0,
    KA_CHAR,
    KA_BACKSPACE,
    KA_ENTER,
    KA_SPACE,
    KA_SHIFT,
    KA_TOGGLE_SYMBOLS,
    KA_LEFT,
    KA_RIGHT,
    KA_UP,
    KA_DOWN,
    KA_RUN,
    KA_SAVE,
    KA_DEFAULT,
    KA_TAB,
    KA_COMPLETE,
} KeyAction;

typedef enum
{
    KB_PAGE_LETTERS = 0,
    KB_PAGE_SYMBOLS = 1,
    KB_PAGE_EXTRA   = 2,
} KeyboardPage;

// Grille logique : 10 colonnes x 5 lignes sur l'écran tactile (320x240).
#define KB_COLS 10
#define KB_ROWS 5

// Bandeau du haut = barre d'autocomplétion (3 propositions) + DEFAULT.
#define KB_TOOLBAR_H 24
#define KB_MAX_SUGGESTIONS 3
#define KB_SUGGEST_W 85.0f                               // largeur d'un slot
#define KB_DEFAULT_X (KB_MAX_SUGGESTIONS * KB_SUGGEST_W) // 255 -> 320

typedef struct
{
    const char* label;
    KeyAction   action;
    char        ch;
    int         col;
    int         row;
    int         colspan;
} KeyDef;

typedef struct
{
    KeyboardPage page;
    int          shift;

    const char*  suggestions[KB_MAX_SUGGESTIONS];
    int          suggestionCount;
    int          suggestionIndex; // slot sélectionné (cycle avec L physique)
} Keyboard;

void Keyboard_Init(Keyboard* kb);

void Keyboard_SetSuggestions(Keyboard* kb, const char** suggestions, int count);

void Keyboard_Render(const Keyboard* kb, C2D_TextBuf textBuf);

KeyAction Keyboard_HitTest(Keyboard* kb, int touchX, int touchY, char* outCh);

// Fait avancer la sélection de suggestion (appelé par le bouton L physique).
void Keyboard_CycleSuggestion(Keyboard* kb);

// Retourne la suggestion actuellement sélectionnée, ou NULL si aucune.
const char* Keyboard_GetSelectedSuggestion(const Keyboard* kb);