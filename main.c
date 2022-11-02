#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "so_stdio.h"
#include "ErrorCheck.h"
#include "utils.h"

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    // verifica parametrii functiei
    if (check_error_so_fopen_args(pathname, mode) == THROW_NULL)
    {
        printf("Parametrii invalizi\n");
        return NULL;
    }
    // aloca un pointer SO_FILE
    SO_FILE *new_file = (SO_FILE *)malloc(sizeof(SO_FILE));
    if (new_file == NULL)
    {
        printf("Nu s-a putut aloca un SO_FILE\n");
        return NULL;
    }
    // initializeaza structura SO_FILE
    new_file->flags = get_flags(mode);
    new_file->filemode = get_filemode(mode);
    new_file->buffer_offset = -1;
    new_file->file_offset = 0;
    new_file->last_operation = none_op;
    new_file->openmode = get_open_mode(mode);
    new_file->bool_is_eof=0;
    new_file->bool_is_error=0;
    memset(new_file->buffer, 0, BUFFER_SIZE); // initializare buffer cu valorile pe 0

    // deschiderea fisierului si intializare handle
    if (new_file->filemode == -1)
    {
        new_file->handle = open(pathname, new_file->flags);
    }
    else
    {
        new_file->handle = open(pathname, new_file->flags, new_file->filemode);
    }

    // verifica ca fisierul s-a deschis cu succes
    if (new_file->handle < 0)
    {
        printf("Eroare la deschiderea fisierului\n");
        new_file->bool_is_error=1;
        return NULL;
    }
    return new_file;
}

int so_fclose(SO_FILE *stream)
{
    // Inchide ​fisierul primit ca parametru si
    // Elibereaza memoria folosita de structura ​SO_FILE​.
    // Intoarce 0 in caz de succes sau ​SO_EOF​ in caz de eroare.

    int return_value = close(stream->handle);
    if (return_value < 0)
    {
        //ATENTIE: SO_FILE NU SE VA DEZALOCA IN MOMENTUL DE FATA
        stream->bool_is_error=1;
        return SO_EOF;
    }
    free(stream);
    stream = NULL;
    return 0;
}

int so_fileno(SO_FILE *stream)
{

    if (check_error_so_fileno(stream) == THROW_ERR)
    {
        stream->bool_is_error=1;
        return -1;
    }
    return stream->handle;
}

int so_fflush(SO_FILE *stream)
{
    if (check_error_so_fflush(stream) == THROW_SO_EOF)
    {
        stream->bool_is_error=1;
        return SO_EOF;
    }

    int ret_value = write(stream->handle, stream->buffer, (stream->buffer_offset + 1) * sizeof(char));

    // DACA NU ARE SUCCES SCRIEREA, INTOARCE SO_EOF
    if (ret_value < 0)
    {
        stream->bool_is_error=1;
        return SO_EOF;
    }
    printf("FFlush trebuia sa scrie %d caractere. FFlush a scris %d caractere\n", stream->buffer_offset + 1, ret_value);

    // dupa ce a scris in fisier, punem valorile din buffer din nou pe 0 si mutam offset-ul buffer-ului.Ultima operatie se va muta pe fflush_op
    //invalidarea bufferului
    memset(stream->buffer, 0, BUFFER_SIZE);
    stream->buffer_offset = -1;
    stream->last_operation = fflush_op;

    return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (check_error_so_fseek(stream, offset, whence) == THROW_ERR)
    {
        stream->bool_is_error=1;
        return -1;
    }

    if(stream->last_operation==read_op)
    {
        //continutul bufferului trebuie invalidat
        memset(stream->buffer, 0, BUFFER_SIZE);
        stream->buffer_offset = -1;
    }

    if(stream->last_operation==write_op)
    {
        //continutul trebuie scris in fisier si apoi invalidat
        int ret_value=so_fflush(stream);
        if(ret_value<0)
        {
            stream->bool_is_error=1;
            return -1;
        }
    }
    
    int fseek_return =lseek(stream->handle,offset,whence);
    //verifica daca pozitionarea cursorului a avut succes
    if(fseek_return<0)
    {
        stream->bool_is_error=1;
        return -1;
    } 
    //retine pozitia cursorului in fisier si ultima operatie
    stream->file_offset=fseek_return;
    stream->last_operation=fseek_op;
    return 0;
}

long so_ftell(SO_FILE *stream)
{
    if(check_error_so_ftell(stream)==THROW_ERR)
    {
        stream->bool_is_error=1;
        return -1;
    }
    return stream->file_offset;
}

int so_feof(SO_FILE *stream)
{
    if(check_error_so_feof(stream)==THROW_ERR)
    {
        stream->bool_is_error=1;
        return -1;
    }
    if(stream->bool_is_eof==1)
    {
        return 1;
    }
    return 0;
}

int so_ferror(SO_FILE *stream)
{
    if(check_error_so_ferror(stream)==THROW_ERR)
    {
        stream->bool_is_error=1;
        return -1;
    }

    if(stream->bool_is_error==1)
    {
        return 1;
    }
    return 0;
}

int main()
{
    SO_FILE *file = so_fopen("maine", "w");
    printf("Descriptorul asociat fisierului este:%d\n", so_fileno(file));

    strcpy(file->buffer, "Acesta este textul de afisat\n");
    file->buffer_offset = strlen(file->buffer) - 1;
    file->last_operation = write_op;

    so_fflush(file);
    file->last_operation = write_op;
    so_fflush(file);

    file->last_operation = read_op;
    strcpy(file->buffer, "Acesta este textul de afisat\n");
    file->buffer_offset = strlen(file->buffer) - 1;

    printf("%d\n",so_fseek(file,-11,SEEK_END));
    so_fclose(file);
    printf("%d\n",so_ferror(file));

    // TO DO: TREBUIE ACTUALIZAT LASTOPERATION DUPA SCRIERE,CITIRE SI ALTE OPERATII
    // ATENTIE: S-AR PUTEA CA ATUNCI CAND DAM CLOSE, SA TREBUIASCA SA APELAM FFLUSH INAINTE

    return 0;
}