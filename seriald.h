#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "mongo.h"
#include "json/json.h"
#include "tcl.h"

typedef struct HANDLE {
    char *path;
    Tcl_HashTable files;
} HANDLE;

typedef struct HEADER {
    time_t begin;
    time_t end;
    uint64_t n_events;
    uint64_t peak_events;
} HEADER;

typedef struct FRAME {
    uint32_t bytes;
    uint32_t prev_bytes;
    char data[1];
} FRAME;

typedef struct DBFILE {
    Tcl_HashEntry *he;
    HEADER *hdr;
    int fd;
} DBFILE;


// json.c
char * json_to_bson(char *js, int *size);

// core.c
char **srld_key_to_argv(char *key, int *nkeys);
void srld_argv_free(char **argv);
int srld_find(bson_iterator *i, int keyc, char **keyv, char *data, int depth);

