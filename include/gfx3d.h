#pragma once
#include <citro3d.h>

// Petit moteur 3D "immédiat" façon SmileBASIC, construit directement sur
// citro3d (indépendant de citro2d, mais dessine sur le même
// C3D_RenderTarget qu'utilise déjà le rendu 2D -- voir main.c/library_3ds.c
// pour le partage de GraphicsTarget). Pensé pour être piloté depuis un
// script PicoC : gfx3dBegin() / vertex3d() / gfx3dEnd(), un triangle à la
// fois, sans se soucier de VBO/attributs/shaders.
//
// ⚠️ Partie la plus spéculative du projet : shader PICA200 et interactions
// AttrInfo/BufInfo avec citro2d écrits de mémoire, jamais compilés ni
// testés sur un vrai devkitARM. Voir le README pour les points à vérifier
// en premier si ça ne fonctionne pas tel quel.

// Nombre max de sommets accumulés entre un gfx3dBegin() et un gfx3dEnd().
// Largement suffisant pour des formes "façon SmileBASIC" (quelques
// centaines de triangles) ; augmente si besoin (alloué en mémoire linéaire).
#define GFX3D_MAX_VERTICES 4096

// Initialise le shader et le buffer de sommets. À appeler une fois après
// C3D_Init()/C2D_Init() (voir main.c). Renvoie 0 si OK, -1 sinon.
int Gfx3D_Init(void);

// Libère le shader et le buffer de sommets. À appeler avant C3D_Fini().
void Gfx3D_Exit(void);

// Positionne la caméra ("regarde de eye vers target, up = +Y monde").
// Coordonnées en unités "monde" (float), pas en pixels écran.
void Gfx3D_SetCamera(float eyeX, float eyeY, float eyeZ,
                     float targetX, float targetY, float targetZ);

// Ajuste le champ de vision (en degrés) et le ratio largeur/hauteur de
// la projection en perspective. À rappeler si l'aspect change (écran du
// haut 400x240 vs écran du bas 320x240, cf. gfx3dTarget côté PicoC).
void Gfx3D_SetFov(float fovDegrees, float aspectRatio);

// Démarre l'accumulation de sommets pour une primitive. primType doit être
// une valeur de GPU_Primitive_t (GPU_TRIANGLES, GPU_TRIANGLE_STRIP,
// GPU_TRIANGLE_FAN).
void Gfx3D_Begin(GPU_Primitive_t primType);

// Ajoute un sommet (position monde + couleur 0..255 par canal) à la
// primitive en cours. Sans effet si Gfx3D_Begin() n'a pas été appelé, ou
// si GFX3D_MAX_VERTICES est dépassé (sommets excédentaires ignorés).
void Gfx3D_Vertex(float x, float y, float z, int r, int g, int b);

// Envoie les sommets accumulés au GPU et dessine la primitive sur le
// C3D_RenderTarget actuellement lié (voir C3D_FrameDrawOn / GraphicsTarget
// dans library_3ds.c). Sans effet si aucun sommet n'a été ajouté.
void Gfx3D_End(void);