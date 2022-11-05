#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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
    new_file->buffer_offset = 0;
    new_file->buffer_length = 0;
    new_file->file_offset = 0;
    new_file->last_operation = none_op;
    new_file->openmode = get_open_mode(mode);
    new_file->bool_is_eof = 0;
    new_file->bool_is_error = 0;
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
        new_file->bool_is_error = 1;
        return NULL;
    }
    return new_file;
}

int so_fclose(SO_FILE *stream)
{
    // Inchide ​fisierul primit ca parametru si
    // Elibereaza memoria folosita de structura ​SO_FILE​.
    // Intoarce 0 in caz de succes sau ​SO_EOF​ in caz de eroare.


    //Inainte de a inchide fisierul, trebuie sa scriu ce am in buffer daca ultima operatie a fost de scriere
    if(stream->last_operation==write_op)
    {
        int ret_value = so_fflush(stream);
        if(ret_value<0)
        {
            return SO_EOF;
        }
    }

    int return_value = close(stream->handle);
    if (return_value < 0)
    {
        // ATENTIE: SO_FILE NU SE VA DEZALOCA IN MOMENTUL DE FATA
        stream->bool_is_error = 1;
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
        stream->bool_is_error = 1;
        return -1;
    }
    return stream->handle;
}

int so_fflush(SO_FILE *stream)
{
    if (check_error_so_fflush(stream) == THROW_SO_EOF)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    int written_Bytes = write(stream->handle, stream->buffer, stream->buffer_length);

    // DACA NU ARE SUCCES SCRIEREA, INTOARCE SO_EOF
    if (written_Bytes < 0)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // Fflush trebuie sa asigure scrierea in fisier mereu
    while (written_Bytes < stream->buffer_length)
    {
        int written_Bytes2 = write(stream->handle, stream->buffer + written_Bytes, stream->buffer_length - written_Bytes);
        if (written_Bytes2 < 0)
        {
            stream->bool_is_error = 1;
            return SO_EOF;
        }
        written_Bytes = written_Bytes + written_Bytes2;
    }

    printf("FFlush trebuia sa scrie %d caractere. FFlush a scris %d caractere\n", stream->buffer_length, written_Bytes);

    // dupa ce a scris in fisier, punem valorile din buffer din nou pe 0 si mutam offset-ul buffer-ului.Ultima operatie se va muta pe fflush_op
    // invalidarea bufferului
    memset(stream->buffer, 0, BUFFER_SIZE);
    stream->buffer_offset = 0;
    stream->buffer_length = 0;
    stream->last_operation = none_op;
    return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (check_error_so_fseek(stream, offset, whence) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }

    if (stream->last_operation == read_op)
    {
        // continutul bufferului trebuie invalidat
        memset(stream->buffer, 0, BUFFER_SIZE);
        stream->buffer_offset = 0;
        stream->buffer_length = 0;
    }

    if (stream->last_operation == write_op)
    {
        // continutul trebuie scris in fisier si apoi invalidat
        int ret_value = so_fflush(stream);
        if (ret_value < 0)
        {
            stream->bool_is_error = 1;
            return -1;
        }
    }

    int fseek_return = lseek(stream->handle, offset, whence);
    // verifica daca pozitionarea cursorului a avut succes
    if (fseek_return < 0)
    {
        stream->bool_is_error = 1;
        return -1;
    }
    // retine pozitia cursorului in fisier si ultima operatie
    stream->bool_is_eof=0;
    stream->file_offset = fseek_return;
    return 0;
}

long so_ftell(SO_FILE *stream)
{
    if (check_error_so_ftell(stream) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }
    return stream->file_offset;
}

int so_feof(SO_FILE *stream)
{
    if (check_error_so_feof(stream) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }
    if (stream->bool_is_eof == 1)
    {
        return 1;
    }
    return 0;
}

int so_ferror(SO_FILE *stream)
{
    if (check_error_so_ferror(stream) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }

    if (stream->bool_is_error == 1)
    {
        return 1;
    }
    return 0;
}

int so_fgetc(SO_FILE *stream)
{
    if (stream == NULL)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca nu poate sa citeasca
    if (is_read_flag_on(stream->openmode) == 0)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca a ajuns la finalul fisierului
    if (stream->bool_is_eof == 1)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca trebuie incarcat bufferul
    if (stream->buffer_length == 0 || stream->buffer_offset == stream->buffer_length)
    {
        // incarc buffer-ul
        int bytesReaded = read(stream->handle, stream->buffer, BUFFER_SIZE);
        // verific daca a citit cu succes si daca a ajuns la finalul fisierului
        if (bytesReaded <= 0)
        {
            if(bytesReaded==0)
            {
                stream->bool_is_eof = 1;
            }
            else
            {
                printf("%s",strerror(errno));
                stream->bool_is_error = 1;
            }
            return SO_EOF;
        }
        // actualizez informatiile despre buffer odata ce l-am incarcat
        stream->buffer_length = bytesReaded;
        stream->buffer_offset = 0;
    }
    stream->file_offset++;
    stream->buffer_offset++;
    stream->last_operation = read_op;

    return (int)stream->buffer[stream->buffer_offset - 1];
}

int so_fputc(int c, SO_FILE *stream)
{
    if (stream == NULL)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca nu poate sa scrie
    if (is_write_flag_on(stream->openmode) == 0)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // daca bufferul este full, atunci va apela fflush
    if (stream->buffer_offset == BUFFER_SIZE)
    {
        int retValue = so_fflush(stream);
        if (retValue < 0)
        {
            stream->bool_is_error = 1;
            return SO_EOF;
        }
    }
    // va scrie in buffer si va muta cursorul
    stream->buffer[stream->buffer_offset] = (unsigned char)c;
    stream->buffer_offset++;
    stream->buffer_length++;
    stream->file_offset++;
    stream->last_operation = write_op;
    return c;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int total_read_elements = 0;
    // verifica daca argumentele sunt valide
    if (check_error_so_fread_fwrite(stream, size, nmemb) == THROW_SO_EOF)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    //verifica daca se poate citi din fisier
    if(is_read_flag_on(stream->openmode)==0)
    {   
        stream->bool_is_error=1;
        return SO_EOF;
    }

    for (int i = 0; i < nmemb; i++)
    {
        for (int j = 0; j < size; j++)
        {
            // pentru fiecare byte citit, apelam so_fgetc
            int ret_char = so_fgetc(stream);
            // daca a ajuns la finalul fisierului, intoarce cate elemente a citit
            if (ret_char < 0 && stream->bool_is_eof == 1)
            {
                return total_read_elements;
            }
            else
            { // daca a dat eroare la fgetc, intoarce SO_EOF
                if (ret_char < 0&& stream->bool_is_error==1)
                {
                    return SO_EOF;
                }
            }
            // pune byte in ptr
            *(unsigned char *)ptr = (unsigned char)ret_char;
            ptr++;
        }
        total_read_elements++;
    }
    // intoarce cate elemente a citit
    stream->last_operation = read_op;
    return total_read_elements;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int total_elements_written = 0;
    // verifica daca argumentele sunt valide
    if (check_error_so_fread_fwrite(stream, size, nmemb) == THROW_SO_EOF)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    //verifica daca poate scrie in fisier
    if(is_write_flag_on(stream->openmode)==0)
    {
        stream->bool_is_error=1;
        return SO_EOF;
    }

    for (int i = 0; i < nmemb; i++)
    {
        for (int j = 0; j < size; j++)
        {
            // pentru fiecare byte din ptr, apelam so_fputc
            int index = i * size + j;
            int char_to_put = *((int *)(ptr + index));
            int ret_value = so_fputc(char_to_put,stream);

            // verificam daca a intampinat vreo eroare
            if (ret_value < 0 && stream->bool_is_error==1)
            {
                return SO_EOF;
            }
        }
        total_elements_written++;
    }
    stream->last_operation = write_op;
    return total_elements_written;
}

int main()
{

    /*
   printf("Descriptorul asociat fisierului este:%d\n", so_fileno(file));

   strcpy(file->buffer, "Acesta este textul de afisat\n");
   file->buffer_length = strlen(file->buffer);
   file->last_operation = write_op;
   so_fflush(file);
   strcpy(file->buffer, "Text2\n");
   file->buffer_length = strlen(file->buffer);
   file->last_operation = write_op;
   so_fflush(file);
*/
    /*
        so_fseek(file, 0, SEEK_SET);
        printf("Poz cursor:%ld\n",so_ftell(file));
        printf("%c", so_fgetc(file));
        printf("Poz cursor:%ld\n",so_ftell(file));

        so_fseek(file,file->file_offset,SEEK_SET);
        printf("Poz cursor:%ld\n",so_ftell(file));
        for(int i=0;i<5;i++)
        {
            so_fputc('y',file);
        }
        */
    /*
    printf("Poz cursor:%ld\n",so_ftell(file));
    so_fflush(file);
    printf("Poz cursor:%ld\n",so_ftell(file));
    printf("%c", so_fgetc(file));
    printf("Poz cursor:%ld\n",so_ftell(file));
    printf("%c", so_fgetc(file));
    printf("Poz cursor:%ld\n",so_ftell(file));
    */
    /*
     so_fflush(file);
     char buffer[2];
     so_fread(buffer, 1, 3, file);
 */
    SO_FILE *file = so_fopen("maine", "w+");
    char buffer[40] = "1234567sad[;,.cxzc(*#((@Q)#8";
    int r = so_fwrite(buffer, 1, strlen(buffer), file);
    printf("%d\n",r);
    printf("Poz cursor:%ld\n",so_ftell(file));
    so_fflush(file);
    so_fseek(file,-10,SEEK_CUR);

    printf("%c\n",so_fgetc(file));
    /*

    so_fputc((int)'y',file);
    printf("Poz cursor:%ld\n",so_ftell(file));
    so_fflush(file);
    printf("%d\n",so_feof(file));
    printf("%c\n",so_fgetc(file));
    printf("%d\n",so_feof(file));
    printf("Poz cursor:%ld\n",so_ftell(file));
    so_fseek(file,0,SEEK_SET);
    printf("%d\n",so_feof(file));
    printf("%c",so_fgetc(file));

*/


    /*
    char buffer2[5];
    int readed = so_fread(buffer2,1,4,file);
    buffer2[4]='\0';
    printf("%s",buffer2);
    */
    so_fclose(file);

    return 0;
}