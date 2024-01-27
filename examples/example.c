#include <stdio.h>

#define CARGPARSE_IMPLEMENTATION
#define DEFINE_CARG
#include "../cargparse.h"

int main(int argc, char *argv[])
{
    cargparse_setup_args();
    cargparse_parse_args(&argc, &argv);

    printf("--> arg1 = %d!\n", arg1);
    printf("--> arg2 = %s!\n", arg2);
    printf("--> boolean = %i!\n", arg3);

    printf("Remaining arguments: %s", argv[0]);
    for (int i = 1; i < argc; i++)
        printf(", %s", argv[i]);
    printf("\n");
}
