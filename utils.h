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

int get_flags(const char* mode);

int get_filemode(const char* mode);

OPENMODE get_open_mode(const char* mode);

int is_read_flag_on(OPENMODE mode);

int is_write_flag_on(OPENMODE mode);

#endif