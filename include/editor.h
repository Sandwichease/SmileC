#pragma once
#include <citro2d.h>
#include <stddef.h>

// Taille max du programme C édité. 3DS a de la RAM (128 Mo sur 2011,
// bien plus sur New 3DS), donc 256 Ko de texte est très large pour un
// petit programme "style SmileBASIC".
#define EDITOR_BUFFER_CAPACITY (256 * 1024)

// Nombre max de propositions d'autocomplétion renvoyées d'un coup.
#define EDITOR_MAX_SUGGESTIONS 3

typedef struct
{
    char buffer[EDITOR_BUFFER_CAPACITY];
    int  length;      // nombre d'octets utilisés dans buffer
    int  cursor;      // position du curseur (index dans buffer, 0..length)
    int  scrollLine;  // première ligne visible (pour le défilement vertical)
} Editor;

void Editor_Init(Editor* ed);

// Édition
void Editor_InsertChar(Editor* ed, char c);
void Editor_InsertNewline(Editor* ed);
void Editor_Backspace(Editor* ed);
void Editor_MoveCursorLeft(Editor* ed);
void Editor_MoveCursorRight(Editor* ed);
void Editor_MoveCursorUp(Editor* ed);
void Editor_MoveCursorDown(Editor* ed);
void Editor_Clear(Editor* ed);
void Editor_InsertTab(Editor* ed);

// Autocomplétion : remplit out[] avec jusqu'à maxOut mots commençant par
// le mot en cours de frappe et renvoie le nombre effectivement écrit.
// Les pointeurs renvoyés visent des chaînes statiques : ils restent
// valides tant qu'on ne rappelle pas la fonction... et même après, car
// ce sont des littéraux. On peut donc les stocker (cf. clavier).
int Editor_GetSuggestions(const Editor* ed, const char** out, int maxOut);

// Insère la fin du mot suggéré n°index (0 = première proposition).
// Ne fait rien si l'index ne correspond à aucune suggestion.
void Editor_AcceptSuggestion(Editor* ed, int index);

// Persistance carte SD (chemin "sdmc:/3ds/smileclang/...")
// Renvoie 0 si OK, -1 en cas d'erreur.
int Editor_SaveToFile(const Editor* ed, const char* path);
int Editor_LoadFromFile(Editor* ed, const char* path);

// Rendu du texte sur l'écran (citro2d). textBuf doit être vidé
// (C2D_TextBufClear) par l'appelant avant l'appel, car on y parse
// le texte visible à chaque frame (contenu dynamique).
// Note : met aussi à jour ed->scrollLine pour garder le curseur visible,
// donc prend un pointeur non-const (ce n'est pas une fonction "pure").
void Editor_Render(Editor* ed, C2D_TextBuf textBuf,
                    float x, float y, float lineHeight, int visibleLines);