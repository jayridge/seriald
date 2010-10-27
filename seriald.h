#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <tcutil.h>
#include "bson.h"
#include "json/json.h"

#define SRLD_OK 0
#define SRLD_ERROR 1

#define SLAB_SIZE 1024*1024*1024

#define ID "SRLD"
#define VERSION 0

typedef struct srld_db_header {
    char id[4];
    uint8_t version;
    uint64_t nrecords;
    struct timeval min_time;
    struct timeval max_time;
    off_t last_record_offset;
    off_t end_of_data_offset;
} srld_db_header;

typedef struct srld_stat {
    struct stat st;
    srld_db_header header;
} srld_stat;

typedef struct srld_db {
    char *path;
    int fd;
    size_t map_size;
    void *map_base;
    srld_db_header *header;
    char *data;
} srld_db;

typedef struct srld_iterator {
    srld_db *db;
    off_t current;
    struct timeval start;
    uint64_t iterations;
} srld_iterator;

typedef struct srld_record_header {
    struct timeval timestamp;
    uint64_t size;
    uint64_t previous_size;
} srld_record_header;

typedef struct srld_record {
    srld_record_header header;
    uint64_t allocated;
    char *data;
} srld_record;

// database
int srld_db_open( char *path, int oflag, srld_db *db );
void srld_db_close( srld_db *db );
int srld_db_stat( srld_db *db, srld_stat *st );
int srld_db_seek( srld_db *db, off_t offset, int whence );

// iterator
void srld_iterator_init( srld_db *db, srld_iterator *iterator );
srld_record * srld_next( srld_iterator *iterator );
srld_record * srld_previous( srld_iterator *iterator );


// ----------- old --------------

// json.c
char * json_to_bson(char *js, int *size);

// core.c
char **srld_key_to_argv(char *key, int *nkeys);
void srld_argv_free(char **argv);
int srld_find(bson_iterator *i, int keyc, char **keyv, char *data, int depth);

