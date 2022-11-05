#include "utils.h"
#include <string.h>
#include <fcntl.h>

int get_flags(const char *mode)
{
    int flags=0;
    // deschide fisierul pentru citire. Esueaza daca nu exista
    if (strcmp(mode, "r") == 0)
    {
        flags=O_RDONLY;
    }
    //deschide fisierul pentru scriere. Daca nu exista, creeaza. Daca exista, truncheaza la 0
    if (strcmp(mode, "w") == 0)
    {
        flags=O_WRONLY|O_CREAT|O_TRUNC;
    }
    //deschide in append(scriere la final).Daca nu exista,creeaza
    if (strcmp(mode, "a") == 0)
    {
        flags=O_WRONLY|O_CREAT|O_APPEND;
    }
    //deschide pentru citire si scriere. Esueaza daca nu exista
    if (strcmp(mode, "r+") == 0)
    {
        flags=O_RDWR;
    }
    //deschide pentru citire si scriere. Daca nu exista,creeaza. Daca exista, truncheaza la 0
    if (strcmp(mode, "w+") == 0)
    {
        flags=O_RDWR|O_CREAT|O_TRUNC;
    }
    //deschide in append + read. Daca nu exista,creeaza
    if (strcmp(mode, "a+") == 0)
    {
        flags=O_RDWR|O_CREAT|O_APPEND;
    }
    return flags;
}

int get_filemode(const char* mode)
{
    int filemode=-1;
    if((strcmp(mode, "w") == 0)||(strcmp(mode, "a") == 0)||(strcmp(mode, "w+") == 0)||(strcmp(mode, "a+") == 0))
    {
        filemode=0644; //permisiunile implicite
    }
    return filemode;
}

OPENMODE get_open_mode(const char* mode)
{
    if (strcmp(mode, "r") == 0)
    {
        return r;
    }
    if (strcmp(mode, "w") == 0)
    {
        return w;
    }
    if (strcmp(mode, "a") == 0)
    {
        return a;
    }
    if (strcmp(mode, "r+") == 0)
    {
        return rplus;
    }
    if (strcmp(mode, "w+") == 0)
    {
        return wplus;
    }
    if (strcmp(mode, "a+") == 0)
    {
        return aplus;
    }
    return unset;
}

int is_read_flag_on(OPENMODE mode)
{
    if(mode==r || mode==rplus || mode==aplus||mode==wplus)
    {
        return 1;
    }
    return 0;
}

int is_write_flag_on(OPENMODE mode)
{
    if(mode==w || mode==wplus || mode==aplus||mode==a||mode==rplus)
    {
        return 1;
    }
    return 0;
}