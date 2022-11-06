#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "so_stdio.h"
#include "ErrorCheck.h"
#include "utils.h"

#define READ_END 0
#define WRITE_END 1

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
    new_file->pid = -1;

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

    // Inainte de a inchide fisierul, trebuie sa scriu ce am in buffer daca ultima operatie a fost de scriere
    if (stream->last_operation == write_op)
    {
        int ret_value = so_fflush(stream);
        if (ret_value < 0)
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

    // printf("FFlush trebuia sa scrie %d caractere. FFlush a scris %d caractere\n", stream->buffer_length, written_Bytes);

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
    // retine pozitia cursorului in fisier
    stream->bool_is_eof = 0;
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
            if (bytesReaded == 0)
            {
                stream->bool_is_eof = 1;
            }
            else
            {
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

    // verifica daca se poate citi din fisier
    if (is_read_flag_on(stream->openmode) == 0)
    {
        stream->bool_is_error = 1;
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
                if (ret_char < 0 && stream->bool_is_error == 1)
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

    // verifica daca poate scrie in fisier
    if (is_write_flag_on(stream->openmode) == 0)
    {
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    for (int i = 0; i < nmemb; i++)
    {
        for (int j = 0; j < size; j++)
        {
            // pentru fiecare byte din ptr, apelam so_fputc
            int index = i * size + j;
            int char_to_put = *((int *)(ptr + index));
            int ret_value = so_fputc(char_to_put, stream);

            // verificam daca a intampinat vreo eroare
            if (ret_value < 0 && stream->bool_is_error == 1)
            {
                return SO_EOF;
            }
        }
        total_elements_written++;
    }
    stream->last_operation = write_op;
    return total_elements_written;
}

SO_FILE *so_popen(const char *command, const char *type)
{
    // verifica daca type e r sau w
    if ((strcmp(type, "r") != 0) && (strcmp(type, "w") != 0))
    {
        return NULL;
    }

    // cei doi descriptori ale capetelor pipe-ului
    int pipe_fd[2];
    int file_descriptor;
    int ret = pipe(pipe_fd);
    if (ret < 0)
    {
        // eroare la crearea pipe-ului
        return NULL;
    }
    // creare proces copil
    pid_t pid = fork();
    if (pid == -1)
    {
        // eroare la crearea procesului. Inchid capetele pipe-ului
        close(pipe_fd[WRITE_END]);
        close(pipe_fd[READ_END]);
        return NULL;
    }
    if (pid == 0) // COPIL
    {
        // DESCHIDE CAPATUL CORESPUNZATOR COPILULUI
        if (strcmp(type, "r") == 0)
        {
            // inchid capatul nefolosit
            close(pipe_fd[READ_END]);
            // COPILUL SCRIE IN PIPE
            // Iesirea procesului copil trebuie sa fie fisierul de write din pipe
            if (pipe_fd[WRITE_END] != STDOUT_FILENO)
            {
                int ret_val = dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
                if (ret_val == -1)
                {
                    close(pipe_fd[WRITE_END]);
                    return NULL;
                }
            }
            close(pipe_fd[WRITE_END]);
        }
        else
        {
            // inchid capatul nefolosit
            close(pipe_fd[WRITE_END]);
            // COPILUL CITESTE DIN PIPE
            // Intrarea procesului copil trebuie sa fie fisierul de read din pipe
            if (pipe_fd[READ_END] != STDIN_FILENO)
            {
                int ret_val = dup2(pipe_fd[READ_END], STDIN_FILENO);
                if (ret_val == -1)
                {
                    close(pipe_fd[READ_END]);
                    return NULL;
                }
            }
            close(pipe_fd[READ_END]);
        }

        // EXECUTA COMANDA DATA DE COMMAND
        char *argproc[4] = {"sh", "-c", NULL, NULL};
        argproc[2] = (char *)command;
        execvp("sh", argproc);
        return NULL;
    }
    if (pid > 0) // PARINTE
    {
        if (strcmp(type, "r") == 0)
        {
            // PARINTELE VA FOLOSI CAPATUL DE CITIRE
            // inchidem capatul de sciere
            close(pipe_fd[WRITE_END]);
            file_descriptor = pipe_fd[READ_END];
        }
        else
        {
            // PARINTELE VA FOLOSI CAPATUL DE SCRIERE
            // inchidem capatul de citire
            close(pipe_fd[READ_END]);
            file_descriptor = pipe_fd[WRITE_END];
        }
    }
    // initializare structura SO_FILE

    // aloca fisier so_file
    SO_FILE *file = (SO_FILE *)malloc(sizeof(SO_FILE));
    // verifica alocarea
    if (file == NULL)
    {
        return NULL;
    }

    // initializeaza structura SO_FILE
    file->flags = get_flags(type);
    file->filemode = get_filemode(type);
    file->buffer_offset = 0;
    file->buffer_length = 0;
    file->file_offset = 0;
    file->last_operation = none_op;
    file->openmode = get_open_mode(type);
    file->bool_is_eof = 0;
    file->bool_is_error = 0;
    memset(file->buffer, 0, BUFFER_SIZE);
    file->pid = pid;
    file->handle = file_descriptor;

    return file;
}

int so_pclose(SO_FILE *stream)
{
    if (stream->pid == -1)
    {
        return -1;
    }
    int waited_pid = stream->pid;
    int wstatus;

    int ret_fclose = so_fclose(stream);
    if (ret_fclose < 0)
    {
        return -1;
    }

    int ret_value = waitpid(waited_pid, &wstatus, 0);
    if (ret_value == -1)
    {
        return -1;
    }
    return wstatus;
}

int main()
{
    /*
    SO_FILE *file = so_fopen("maine", "w+");
    char buffer[70] = "Acesta este asdasd asd as das d xc v cxv xc vtextul dasdasdt";
    int r = so_fwrite(buffer, 1, strlen(buffer), file);
    printf("%d\n", r);
    printf("Poz cursor:%ld\n", so_ftell(file));
    so_fflush(file);
    so_fseek(file, -10, SEEK_CUR);

    printf("%c\n", so_fgetc(file));

    so_fclose(file);
    */

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