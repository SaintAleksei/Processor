#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main ()
{
    char buff[0x100] = "";
    char str[0x100]  = "";
    char symbol      = 0;
    uint64_t val     = 0;
    int  count       = 0;
    int ret          = 0;
    

    fgets (buff, 0x100, stdin);
    ret = sscanf (buff, "%[a-zA-Z]%c%lu%n", str, &symbol, &val, &count);

    printf ("len    = %lu;\n"
            "ret    = %d;\n"
            "str    = \"%s\";\n"
            "symbol = %c;\n"
            "val    = %lu;\n"
            "count  = %d;\n", strlen (buff), ret, str, symbol, val, count);
}
