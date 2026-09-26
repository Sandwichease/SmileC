# SmileClang

Un petit éditeur C pour Nintendo 3DS, inspiré de SmileBASIC.

L'idée est simple : écrire du C directement sur la 3DS avec le clavier tactile, puis lancer le programme sans avoir besoin d'un PC.

Le projet utilise **PicoC** comme moteur d'exécution. Ce n'est donc pas un compilateur C natif pour ARM : le code est interprété directement sur la console.

## Fonctionnalités

* Éditeur de code sur l'écran du haut
* Clavier tactile personnalisé sur l'écran du bas
* Clavier avec une page lettres et une page symboles/chiffres
* `RUN` pour sauvegarder et lancer le programme
* `SAVE` pour sauvegarder le code
* Affichage de `printf()` directement sur l'écran du haut
* Quelques fonctions 3DS accessibles depuis le code C
* Sauvegarde automatique dans `sdmc:/3ds/smileclang/program.c`

## Exemple

Un programme peut simplement faire :

```c
#include <stdio.h>
#include <3ds.h>

int main()
{
    printf("Hello from 3DS!\n");

    while (1)
    {
        if (btnPressed() & BTN_A)
            break;

        wait(1);
    }

    return 0;
}
```

Un exemple complet est disponible dans `examples/hello.c`.

## Structure

```text
source/
├── main.c          boucle principale et gestion des modes
├── editor.c        éditeur et sauvegarde
├── keyboard.c      clavier tactile
└── library_3ds.c   fonctions 3DS accessibles à PicoC

3rdparty/
└── picoc/           moteur PicoC

examples/
└── hello.c          exemple de programme
```

## Installer PicoC

PicoC n'est pas inclus directement dans le dépôt.

Avec un submodule Git :

```bash
git submodule add https://github.com/FlagBrew/picoc.git 3rdparty/picoc
```

Ou simplement :

```bash
git clone https://github.com/FlagBrew/picoc.git 3rdparty/picoc
```

Le Makefile s'occupe ensuite de récupérer les fichiers nécessaires à la compilation.

## Dépendances

Le projet utilise l'environnement **devkitPro** avec :

* devkitARM
* libctru
* citro2d
* citro3d

Sur une installation devkitPro classique :

```bash
(dkp-)pacman -S 3ds-dev citro2d citro3d
```

## Compiler

Configure d'abord `DEVKITARM` si nécessaire :

```bash
export DEVKITARM=/opt/devkitpro/devkitARM
```

Puis :

```bash
make
```

Le build produit notamment :

```text
SmileClang.3dsx
SmileClang.smdh
```

Place ensuite le `.3dsx` dans :

```text
/3ds/SmileClang/
```

et lance-le depuis le Homebrew Launcher.

## Comment fonctionne l'affichage

SmileClang utilise PicoC pour exécuter le code.

Quand un programme PicoC fait :

```c
printf("Hello\n");
```

PicoC utilise le `stdout` standard. Sur 3DS, celui-ci peut être affiché avec la console de libctru.

Il n'y a donc pas de système spécial dans SmileClang pour récupérer chaque `printf()` du programme utilisateur.

## Limites

Le projet est encore une v1, donc il y a quelques limites :

* PicoC interprète le code au lieu de produire du code ARM natif.
* Le C supporté dépend de PicoC et ne correspond pas à un C99/C11 complet.
* Le clavier ne contient pas encore tous les symboles.
* Il n'y a pas encore de coloration syntaxique.
* Pas d'undo ni de sélection de texte avancée.
* Le retour de l'écran de console vers l'éditeur après `RUN` doit encore être testé sur hardware.

Le clavier et l'API 3DS sont pensés pour être assez faciles à étendre.

## API 3DS

Les programmes PicoC peuvent utiliser :

```c
#include <3ds.h>
```

L'API actuelle contient notamment :

```c
btn();
btnPressed();
touchx();
touchy();
wait();
```

ainsi que des constantes comme :

```c
BTN_A
BTN_B
```

L'API est encore petite pour le moment, mais elle pourra être étendue par la suite.

## Pourquoi PicoC ?

Le but de SmileClang n'est pas de créer un compilateur C complet sur 3DS.

L'objectif est plutôt d'avoir quelque chose de proche de l'expérience SmileBASIC

PicoC permet de faire ça sans avoir à construire tout un compilateur natif pour la console.

## État du projet

Projet en développement.

La v1 se concentre surtout sur l'éditeur, le clavier tactile et l'exécution de petits programmes C directement sur 3DS.

