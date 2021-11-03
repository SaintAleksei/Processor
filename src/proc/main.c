#include "processor.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf (stderr, "Usage: %s <name of file>\n", argv[0]);

        return EXIT_FAILURE;
    }

    proc_t proc = {};

    do  
    {
        if (proc_create (&proc, argv[1]) )
            break;

        if (proc_run (&proc) )
            break;

        proc_delete (&proc);
    
        return EXIT_SUCCESS;
    }
    while (0);

    fprintf (stderr, "%s: ", argv[0]);
    proc_error (&proc);

    proc_delete (&proc);
    
    return EXIT_FAILURE;
}

