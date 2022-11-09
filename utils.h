#ifndef SO_UTILS_H
#define SO_UTILS_H

typedef enum{
    w,
    r,
    a,
    wplus,
    rplus,
    aplus,
    unset
} OPENMODE;

#define BUFFER_SIZE 4096

typedef enum{
    read_op,
    write_op,
    none_op
} LOPERATION;

struct _so_file{
    int handle;
    int flags;
    int filemode;
    char buffer[BUFFER_SIZE];
    int buffer_offset;
    int buffer_length;
    int file_offset;
    int bool_is_eof;
    int bool_is_error;
    int pid;
    LOPERATION last_operation;
    OPENMODE openmode;
};

int get_flags(const char* mode);

int get_filemode(const char* mode);

OPENMODE get_open_mode(const char* mode);

int is_read_flag_on(OPENMODE mode);

int is_write_flag_on(OPENMODE mode);

#endif