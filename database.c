#include "seriald.h"

int srld_db_open( char *path, int oflag, srld_db *db )
{
    struct stat st;
    srld_db_header header;

    db->fd = open(path, oflag, 0655);
    if (db->fd < 0 || fstat(db->fd, &st) != 0) {
        return SRLD_ERROR;
    }
    if (st.st_size < sizeof(srld_db_header)) {
        memset(&header, 0, sizeof(header));
        if (write(db->fd, &header, sizeof(header)) != sizeof(header)) {
            return SRLD_ERROR;
        }
        fstat(db->fd, &st);
    }
    db->map_size = st.st_size;
    db->map_base = mmap(NULL, db->map_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                        db->fd, 0);
    db->header = (srld_db_header *)db->map_base;
    db->data = (char *)(db->map_base + sizeof(srld_db_header));
    db->path = strdup(path);

    return SRLD_OK;
}


void srld_db_close( srld_db *db )
{
    if (db->map_base && db->map_size) {
        munmap(db->map_base, db->map_size);
    }
    close(db->fd);
}


int srld_db_stat( srld_db *db, srld_db_stat *st )
{
    if (read(db->fd, db->header, sizeof(srld_db_header))
        != sizeof(srld_db_header)) {
        return SRLD_ERROR;
    }
    if (fstat(db->fd, st->st) == 0) {
        return SRLD_OK;
    }
    return SRLD_ERROR;
}


int srld_db_seek( srld_db *db, off_t offset, int whence )
{
    return lseek(db->fd, offset, whence);
}


int srld_db_read( srld_db *db, srld_record *record )
{
    if (read(db->fd, &record->header, sizeof(srld_record_header))
        != sizeof(srld_record_header)) {
        return SRLD_ERROR;
    }
    if (record->header.size > record->allocated) {
        record->allocated = record->header.size;
        record->data = realloc(record->data, record->allocated);
    }
    if (read(db->fd, record->data, record->header.size)
        != record->header.size) {
        return SRLD_ERROR;
    }
    return SRLD_OK;
}


int srld_db_write( srld_db_db *db, srld_record *record )
{
    struct iovec iov[2];

    iov[0].iov_base = (caddr_t) record->header;
    iov[0].iov_len = sizeof(srld_record_header);
    iov[1].iov_base = (caddr_t) record->data;
    iov[1].iov_len = record->allocated;

    srldb_seek(db, 0, SEEK_END);
    if (writev(db->fd, iov, 2) < 0) {
        return SRLD_ERROR;
    }
    return SRLD_OK;
}
