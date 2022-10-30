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

#endif