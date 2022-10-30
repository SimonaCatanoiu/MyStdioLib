#include "ErrorCheck.h"
#include <stdio.h>
#include <string.h>

int check_error_so_fopen_args(const char *pathname, const char *mode)
{
    if(pathname==NULL || mode==NULL)
    {
        return THROW_NULL;
    }
    if(strcmp(pathname,"")==0)
    {
        return THROW_NULL;
    }
    if((strcmp(mode,"w")!=0)&&(strcmp(mode,"r")!=0)&&(strcmp(mode,"a")!=0)&&(strcmp(mode,"w+")!=0)&&(strcmp(mode,"r+")!=0)&&(strcmp(mode,"a+")!=0))
    {
        return THROW_NULL;
    }
    return MOVEON;
}