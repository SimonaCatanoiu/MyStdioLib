#ifndef SO_ERRORCHECK_H
#define SO_ERRORCHECK_H

#include "so_stdio.h"

#define THROW_NULL -1
#define THROW_ERR -2
#define THROW_SO_EOF -3
#define MOVEON 1


int check_error_so_fopen_args(const char *pathname, const char *mode);

int check_error_so_fileno(SO_FILE* stream);

int check_error_so_fflush(SO_FILE* stream);

int check_error_so_fseek(SO_FILE* stream,long offset,int whence);

int check_error_so_ftell(SO_FILE* stream);

int check_error_so_feof(SO_FILE* stream);

int check_error_so_ferror(SO_FILE* stream);

int check_error_so_fread_fwrite(SO_FILE *stream,size_t size, size_t nmemb);

#endif