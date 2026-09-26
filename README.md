# SmileClang — éditeur C on-device pour 3DS (moteur PicoC)

Un homebrew 3DS façon SmileBASIC : écran du haut = code + sortie du
programme, écran du bas = clavier tactile custom pour écrire du C
directement sur la console, sans PC. Le moteur d'exécution est
**PicoC** (fork FlagBrew, déjà utilisé en production dans Checkpoint sur
3DS), pas un compilateur natif ARM — voir la note "Limites" plus bas.

⚠️ **Important** : ce code a été écrit et vérifié ligne à ligne contre le
vrai code source de PicoC (cloné et étudié pour cette intégration), mais
il n'a **pas été compilé ni testé sur un vrai devkitARM** — mon
environnement ne l'a pas. Attends-toi à devoir corriger quelques erreurs
de compilation mineures (signatures légèrement différentes selon la
version de libctru/citro2d que tu as installée, etc.).

## Ce que ça fait déjà (v1)

- Éditeur de texte plein écran (haut) avec défilement automatique.
- Clavier tactile custom (bas) avec 2 pages : lettres et
  symboles/chiffres, bascule via la touche `123`/`ABC`.
- Boutons `RUN` (sauvegarde + exécute le programme) et `SAVE`.
- `printf()`/`scanf()` du programme utilisateur s'affichent directement
  sur l'écran du haut (voir "Comment ça marche" ci-dessous — aucun code
  personnalisé requis pour ça, c'est une propriété de PicoC + libctru).
- Une petite lib `3ds.h` exposée aux scripts : `btn()`, `btnPressed()`,
  `touchx()`, `touchy()`, `wait()`, constantes `BTN_A`, `BTN_B`, etc.
- Sauvegarde/chargement automatique sur `sdmc:/3ds/smileclang/program.c`.

## Comment ça marche (architecture)

```
source/main.c        -> init citro2d, boucle principale, bascule EDIT/RUN
source/editor.c       -> buffer de texte, curseur, rendu, sauvegarde SD
source/keyboard.c     -> layout du clavier tactile, hit-test, rendu
source/library_3ds.c  -> API "#include <3ds.h>" exposée aux scripts PicoC
3rdparty/picoc/       -> coeur de PicoC (à récupérer, voir ci-dessous)
```

Point clé compris en étudiant le code source de PicoC : `printf()` d'un
script PicoC écrit directement sur le vrai `stdout` du système (`CStdOut
= stdout` dans `picoc/source/interpreter/cstdlib/stdio.c`). Sur 3DS,
`consoleInit(GFX_TOP, NULL)` (libctru) redirige justement ce `stdout`
vers un rendu texte à l'écran. Résultat : la sortie des programmes
écrits par l'utilisateur s'affiche automatiquement, sans qu'on ait eu à
écrire le moindre code de redirection nous-mêmes.

## Étape 1 : récupérer PicoC

```bash
cd smileclang-3ds
git submodule add https://github.com/FlagBrew/picoc.git 3rdparty/picoc
# ou, si tu ne veux pas de submodule git :
# git clone https://github.com/FlagBrew/picoc.git 3rdparty/picoc
```

Le Makefile fourni compile déjà les bons dossiers
(`3rdparty/picoc/source`, `.../interpreter`, `.../interpreter/cstdlib`,
`.../platform`) et exclut automatiquement `library_unix.c`,
`platform_msvc.c` et `library_msvc.c` (voir commentaires dans le
Makefile pour le détail — en résumé : `library_unix.c` entre en conflit
avec notre `source/library_3ds.c`, et les `_msvc.c` sont pour Windows).

## Étape 2 : dépendances devkitPro

Tu as déjà ton environnement prêt, mais pour mémoire, il faut :

```bash
(dkp-)pacman -S 3ds-dev citro2d citro3d
```

## Étape 3 : compiler

```bash
export DEVKITARM=/opt/devkitpro/devkitARM   # adapte à ton install
make
```

Ça doit produire `SmileClang.3dsx` (+ `.smdh`). Copie-le dans
`/3ds/SmileClang/` sur ta carte SD et lance-le depuis le Homebrew
Launcher / hbmenu.

## Limites connues et pistes d'amélioration

- **PicoC est un interpréteur, pas un compilateur natif ARM.** Les
  programmes tournent plus lentement qu'un vrai binaire compilé — très
  bien pour des petits programmes façon SmileBASIC, pas pour un jeu 3D
  gourmand. Voir la discussion précédente sur pourquoi un vrai
  compilateur C auto-hébergé sur 3DS n'existe pas aujourd'hui.
- **Sous-ensemble du C.** PicoC ne supporte pas 100% du C99/C11 (voir
  son README). Suffisant pour la plupart des petits programmes.
- **Clavier minimal.** Il manque volontairement quelques symboles rares
  (`%`, `\`, `|`, `:`, `~`, `^`, `&`) pour tenir dans la grille 10×5 —
  la table `KeyDef` dans `source/keyboard.c` est faite pour être
  étendue facilement (ajoute une 3e page si besoin).
- **Constantes `BTN_*` non vérifiées ici** (voir avertissement en tête
  de `source/library_3ds.c`) — à comparer avec ton header libctru
  installé avant de t'y fier.
- **Retour EDIT après RUN non testé.** L'alternance
  `consoleInit()` (texte simple) → citro2d (GPU) → citro2d à nouveau est
  un pattern standard en homebrew 3DS, mais je ne l'ai pas fait tourner
  ici pour le confirmer. Si l'écran du haut reste bloqué en mode texte
  après un RUN, cherche du côté de `gfxSetDoubleBuffering` /
  ré-initialisation du render target citro2d.
- **Pas de coloration syntaxique, pas d'undo, pas de sélection multi-
  caractères.** Volontairement hors scope v1.

## Exemple de programme à tester

Voir `examples/hello.c` : un petit programme qui utilise `printf`,
`3ds.h` et `wait()`.
