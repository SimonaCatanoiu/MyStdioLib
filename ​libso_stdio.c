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

//Deschide fisierul dat ca parametru intr-un anumit mod si ainitializeaza/aloca memoria necesara pentru structura SO_FILE
//In caz de eroare va intoarce NULL. La succes, intoarce structura SO_FILE
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
    new_file->pid = -1; //punem pid-ul pe invalid daca nu e deschis folosind popen

    // deschiderea fisierului si intializare handle
    if (new_file->filemode == -1)
    {
        new_file->handle = open(pathname, new_file->flags);
    }
    else
    {
        //daca fisierul urmeaza sa fie creat, il initializam cu permisiunile implicite
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

// Inchide ​fisierul primit ca parametru si
// Elibereaza memoria folosita de structura ​SO_FILE​.
// Intoarce 0 in caz de succes sau ​SO_EOF​ in caz de eroare
int so_fclose(SO_FILE *stream)
{
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
        printf("Eroare la inchiderea fisierului\n");
        stream->bool_is_error = 1;
        return SO_EOF;
    }
    free(stream);
    stream = NULL;
    return 0;
}

//Intoarce descriptorul fisierului
//La eroare intoarce -1
int so_fileno(SO_FILE *stream)
{

    if (check_error_so_fileno(stream) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }
    return stream->handle;
}

//Are sens doar pentru fisierele pentru care ultima operatie a fost una de scriere
//In urma apelului acestei functii, datele din buffer sunt scrise în fisier. Intoarce 0 în caz de succes sau ​SO_EOF​ in caz de eroare
//FFlush trebuie sa asigure scrierea intreg bufferului in fisier
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
        printf("FFlush:Eroare la scrierea in fisier\n");
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // Fflush trebuie sa asigure scrierea in fisier mereu
    while (written_Bytes < stream->buffer_length)
    {
        int written_Bytes2 = write(stream->handle, stream->buffer + written_Bytes, stream->buffer_length - written_Bytes);
        if (written_Bytes2 < 0)
        {
            printf("FFlush:Eroare la scrierea in fisier\n");
            stream->bool_is_error = 1;
            return SO_EOF;
        }
        written_Bytes = written_Bytes + written_Bytes2;
    }

    //printf("FFlush trebuia sa scrie %d caractere. FFlush a scris %d caractere\n", stream->buffer_length, written_Bytes);

    // dupa ce a scris in fisier, punem valorile din buffer din nou pe 0 si mutam offset-ul buffer-ului.Ultima operatie se va muta pe none_op
    // invalidarea bufferului
    memset(stream->buffer, 0, BUFFER_SIZE);
    stream->buffer_offset = 0;
    stream->buffer_length = 0;
    stream->last_operation = none_op;
    return 0;
}

//Muta cursorul fisierului. Noua pozitie este obtinuta prin adunarea valorii ​offset​ la pozitia specificata de ​whence
//Intoarce 0 in caz de succes si -1 in caz de eroare.
int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (check_error_so_fseek(stream, offset, whence) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }
    //Daca ultima operatie e de citire, continutul bufferului trebuie invalidat
    if (stream->last_operation == read_op)
    {
        memset(stream->buffer, 0, BUFFER_SIZE);
        stream->buffer_offset = 0;
        stream->buffer_length = 0;
    }
    //Daca ultima operatie e de scriere,continutul bufferului trebuie scris in fisier si apoi invalidat
    //Invalidarea se face in so_fflush
    if (stream->last_operation == write_op)
    {   
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
        printf("Fseek: pozitionarea cursorului a esuat\n");
        stream->bool_is_error = 1;
        return -1;
    }
    //punem is_eof pe 0 deoarece sunt sanse sa ne fi mutat de la EOF anterior prin fseek.
    stream->bool_is_eof = 0;
    // retine pozitia cursorului in fisier
    stream->file_offset = fseek_return;
    return 0;
}

//Intoarce pozitia curenta din fisier. In caz de eroare functia intoarce -1
long so_ftell(SO_FILE *stream)
{
    if (check_error_so_ftell(stream) == THROW_ERR)
    {
        stream->bool_is_error = 1;
        return -1;
    }
    return stream->file_offset;
}

//Intoarce o valoarea diferita de 0 daca s-a ajuns la sfarsitul fisierului sau 0 in caz contrar
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

//Intoarce o valoarea diferita de 0 dacă s-a întalnit vreo eroare in urma unei operatii cu fisierul sau 0 in caz contrar
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

//Citeste un caracter din fisier. Intoarce caracterul ca ​unsigned char​ extins la ​int,​ sau ​SO_EOF​ in caz de eroare
//Se va citi caracter cu caracter direct din buffer.Cand bufferul este plin sau gol, se foloseste apelul de sistem read() pentru a il umple
int so_fgetc(SO_FILE *stream)
{
    if (stream == NULL)
    {
        printf("Fgetc:null argument\n");
        return SO_EOF;
    }

    // verifica daca nu poate sa citeasca
    if (is_read_flag_on(stream->openmode) == 0)
    {
        printf("Fgetc:fara permisiune de citire\n");
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca a ajuns la finalul fisierului
    if (stream->bool_is_eof == 1)
    {
        printf("Fgetc:Sunt la finalul fisierului\n");
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
                printf("Fgetc:A ajuns la finalul fisierului\n");
                stream->bool_is_eof = 1;
            }
            else
            {
                printf("Fgetc:Eroare la citirea din fisier\n");
                stream->bool_is_error = 1;
            }
            return SO_EOF;
        }
        // actualizez informatiile despre buffer odata ce l-am incarcat
        stream->buffer_length = bytesReaded;
        stream->buffer_offset = 0;
    }
    //a citit cu succes un caracter. Se muta pseudo-cursorul din fisier, cursorul buffer-ului si se actualizeaza ultima operatie
    stream->file_offset++;
    stream->buffer_offset++;
    stream->last_operation = read_op;
    //intorc caracterul din buffer de la pozitia curenta
    return (int)stream->buffer[stream->buffer_offset - 1];
}

//Scrie un caracter in fisier. Intoarce caracterul scris sau ​SO_EOF​ in caz de eroare
//Se va scrie caracter cu caracter direct in buffer.Cand bufferul este plin, se foloseste functia fflush pentru a scrie in fisier si a invalida bufferul
int so_fputc(int c, SO_FILE *stream)
{
    if (stream == NULL)
    {
        printf("Fputc:argument null\n");
        return SO_EOF;
    }

    // verifica daca nu poate sa scrie
    if (is_write_flag_on(stream->openmode) == 0)
    {
        printf("Fputc: fara permisiune de scriere\n");
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // daca bufferul este full, atunci va apela fflush ca sa scrie ce e in el si sa il invalideze
    if (stream->buffer_offset == BUFFER_SIZE)
    {
        int retValue = so_fflush(stream);
        if (retValue < 0)
        {
            printf("Fputc: eroare la fflush pentru buffer full\n");
            stream->bool_is_error = 1;
            return SO_EOF;
        }
    }
    // va scrie in buffer caracterul dat si va muta pseudo-cursorul fisierului, respectiv al bufferului. Actualizam si ultima operatie in cea de scriere
    stream->buffer[stream->buffer_offset] = (unsigned char)c;
    stream->buffer_offset++;
    stream->buffer_length++;
    stream->file_offset++;
    stream->last_operation = write_op;
    return c;
}

//Citeste ​nmemb​ elemente, fiecare de dimensiune ​size​. Datele citite sunt stocate la adresa de memorie specificata prin ​ptr​. 
//Intoarce numarul de elemente citite. In caz de eroare sau daca s-a ajuns la sfarsitul fisierului,​ functia întoarce 0.
//Functia se foloseste de apelul lui so_fgetc pentru a asigura citirea din fisier(se va citi caracter cu caracter direct din buffer).
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int total_read_elements = 0;
    // verifica daca argumentele sunt valide
    if (check_error_so_fread_fwrite(stream, size, nmemb) == THROW_SO_EOF)
    {
        printf("Fread:argumente invalide\n");
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca se poate citi din fisier
    if (is_read_flag_on(stream->openmode) == 0)
    {
        printf("Fread:fara permisiune de citire\n");
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
        //Un element e de dimensiune size.
        total_read_elements++;
    }
    stream->last_operation = read_op;
    // intoarce cate elemente a citit. 
    return total_read_elements;
}

//Scrie ​nmemb​ elemente, fiecare de dimensiune ​size​. 
//Datele ce urmeaza a fi scrise sunt luate de la adresa de memorie specificata prin ​ptr​. 
//Intoarce numarul de elemente scrise, sau 0 in caz de eroare.
//Functia se foloseste de apelul lui so_fputc pentru a asigura scrierea in fisier(se va scrie caracter cu caracter direct in buffer)
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int total_elements_written = 0;
    // verifica daca argumentele sunt valide
    if (check_error_so_fread_fwrite(stream, size, nmemb) == THROW_SO_EOF)
    {
        printf("Fwrite:argumente invalide\n");
        stream->bool_is_error = 1;
        return SO_EOF;
    }

    // verifica daca poate scrie in fisier
    if (is_write_flag_on(stream->openmode) == 0)
    {
        printf("Fwrite: fara permisiune de scriere\n");
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
        //fiecare element e de dimensiune size
        total_elements_written++;
    }
    stream->last_operation = write_op;
    //intoarce cate elemete a scris
    return total_elements_written;
}

//Lanseaza un proces nou, care va executa comanda specificata de parametrul ​command​. Ca implementare,​ se va executa ​sh -c command,
//folosind un pipe pentru a redirecta intrarea standard/​iesirea standard a noului proces. 
//Funtia intoarce o structură ​SO_FILE​ pe care apoi se pot face operatiile uzuale de citire/​scriere,​ ca si cum ar fi un fisier obișnuit.
//Valorile parametrului ​type​ pot fi:
//r​ - fisierul intors este read-only. Operatiile ​so_fgetc/​so_fread​ executate pe fisier vor citi de la iesirea standard a procesului creat.
//w​ - fisierul intors este write-only. Operatiile ​so_fputc/​so_fwrite​ executate pe fisier vor scrie la intrarea standard a procesului creat.
//Daca apelul ​fork/​CreateProcess​ esuează se va întoarce NULL.
SO_FILE *so_popen(const char *command, const char *type)
{
    // verifica daca type e r sau w
    if ((strcmp(type, "r") != 0) && (strcmp(type, "w") != 0))
    {
        printf("Popen: argument type invalid\n");
        return NULL;
    }

    int pipe_fd[2];    // cei doi descriptori ale capetelor pipe-ului
    int file_descriptor;
    int ret = pipe(pipe_fd);
    if (ret < 0)
    {
        // eroare la crearea pipe-ului
        printf("Popen: eroare la crearea pipe-ului\n");
        return NULL;
    }
    // creare proces copil
    pid_t pid = fork();
    if (pid == -1)
    {
        // eroare la crearea procesului. Inchid capetele pipe-ului
        printf("Popen: eroare la apelarea lui fork()\n");
        close(pipe_fd[WRITE_END]);
        close(pipe_fd[READ_END]);
        return NULL;
    }
    if (pid == 0) // COPIL
    {
        // DESCHIDE CAPATUL CORESPUNZATOR COPILULUI
        if (strcmp(type, "r") == 0)
        {
            // inchid capatul nefolosit de copil
            close(pipe_fd[READ_END]);
            // COPILUL SCRIE IN PIPE
            // Iesirea procesului copil trebuie sa fie fisierul de write din pipe
            if (pipe_fd[WRITE_END] != STDOUT_FILENO)
            {
                int ret_val = dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
                if (ret_val == -1)
                {
                    printf("Popen:Eroare la duplicarea iesirii standard\n");
                    close(pipe_fd[WRITE_END]);
                    return NULL;
                }
            }
            //sterg descriptorul care ramane nefolosit dupa duplicare
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
                    printf("Popen:Eroare la duplicarea intrarii standard\n");
                    close(pipe_fd[READ_END]);
                    return NULL;
                }
            }
            //sterg descriptorul care ramane nefolosit dupa duplicare
            close(pipe_fd[READ_END]);
        }

        //EXECUTA COMANDA DATA DE COMMAND
        char *argproc[4] = {"sh", "-c", NULL, NULL};
        argproc[2] = (char *)command;
        execvp("sh", argproc);
        //va ajunge la NULL daca apare eroare la execvp.
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
        printf("Popen:eroare la alocarea SO_FILE\n");
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

//Asteapta terminarea procesului lansat de ​so_popen​ si elibereaza memoria ocupata de structura ​SO_FILE​. 
//Intoarce codul de iesire al procesului (cel obtinut in urma apelului ​waitpid​). 
//Daca apelul ​waitpid​ esueaza, se va intoarce -1.
int so_pclose(SO_FILE *stream)
{
    if (stream->pid == -1)
    {
        printf("Pclose: nu exista proces deschis cu popen asociat stream-ului\n");
        return -1;
    }
    int waited_pid = stream->pid;
    int wstatus;

    //inchidem si eliberam memoria ocupata de SO_FILE
    int ret_fclose = so_fclose(stream);
    if (ret_fclose < 0)
    {
        printf("Popen: eroare la inchiderea stream-ului\n");
        return -1;
    }
    //asteapta terminarea procesului copil
    int ret_value = waitpid(waited_pid, &wstatus, 0);
    if (ret_value == -1)
    {
        printf("Pclose:eroare waitpid\n");
        return -1;
    }
    return wstatus;
}
