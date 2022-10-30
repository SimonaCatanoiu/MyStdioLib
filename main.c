#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include  "so_studio.h"
#include "ErrorCheck.h"
#include "utils.h"

//FOPEN PENTRU LINUX
SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    //verifica parametrii functiei
    if(check_error_so_fopen_args(pathname,mode)==THROW_NULL)
    {
        printf("Parametrii invalizi\n");
        return NULL;
    }
    //aloca un pointer SO_FILE
    SO_FILE *new_file=(SO_FILE*)malloc(sizeof(SO_FILE));
    if(new_file==NULL)
    {
        printf("Nu s-a putut aloca un SO_FILE\n");
        return NULL;
    }
    //initializeaza structura SO_FILE
    new_file->flags=get_flags(mode);
    new_file->filemode=get_filemode(mode);
    new_file->buffer_offset=0;
    new_file->last_operation=none_op;
    new_file->openmode=get_open_mode(mode);
    memset(new_file->buffer,0,BUFFER_SIZE); // initializare buffer cu valorile pe 0
    
    //deschiderea fisierului si intializare handle
    if(new_file->filemode==-1)
    {
        new_file->handle=open(pathname,new_file->flags);
    }
    else
    {
        new_file->handle=open(pathname,new_file->flags,new_file->filemode);
    }

    //verifica ca fisierul s-a deschis cu succes
    if(new_file->handle<0)
    {
        printf("Eroare la deschiderea fisierului\n");
        return NULL;
    }
    return new_file;
}

//FCLOSE PENTRU LINUX
int so_fclose(SO_FILE *stream)
{
    //Inchide ​fisierul primit ca parametru si
    //Elibereaza memoria folosita de structura ​SO_FILE​.
    //Intoarce 0 in caz de succes sau ​SO_EOF​ in caz de eroare.
    int return_value=close(stream->handle);
    if(return_value<0)
    {
        free(stream);
        stream=NULL;
        return SO_EOF;
    }
    free(stream);
    stream=NULL;
    return 0;
}

int main()
{
    SO_FILE* file=so_fopen("maine","a+");
    so_fclose(file);
    return 0;
}