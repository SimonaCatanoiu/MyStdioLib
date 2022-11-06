#include <stdio.h>
#include "so_stdio.h"

int main()
{
    SO_FILE *f;
    char line[11];
    f = so_popen("ls -l", "r");
    int total = 0;
    while (so_feof(f) == 0)
    {
        size_t ret = so_fread(&line[total], 1, 10, f);
        if (ret > 0)
        {
            line[ret] = '\0';
            printf("%s", line);
        }
    }
    so_pclose(f);


    SO_FILE *f2;
    f2 = so_popen("cat","w");
    for(int i=0;i<5;i++)
    {
        char buffer[6]="12345\n";
        so_fwrite(buffer,1,6,f2);
    }
    so_pclose(f2);

    return 0;
}