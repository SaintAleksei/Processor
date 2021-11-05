#include "assembler.h"
#include <stdio.h>

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf (stderr, "Usage: %s <name of file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    assm_t assm = {};

    do
    {
        if (assm_create (&assm, argv[1]) )
            break;

        if (assm_translate (&assm) )
            break;

        if (assm_write (&assm) ) 
            break;

        assm_delete (&assm);

        return EXIT_SUCCESS;
    }
    while (0);

    assm_error (&assm);

    assm_delete (&assm);

    return EXIT_FAILURE;
}
