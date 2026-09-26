#include "3ds.h"

int main()
{
    int i;
    char buf[64];

    printf("Hello from SmileClang!\n\n");

    for (i = 0; i < 5; i++)
    {
        sprintf(buf, "tick %d\n", i);
        printf(buf);
        wait(30); /* ~0.5s a 60fps */
    }

    printf("\nAppuie sur START pour revenir a l'editeur.\n");

    return 0;
}