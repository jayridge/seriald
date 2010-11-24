#include "seriald.h"
#include "errno.h"

char *progname = "";

void print_stats(FILE *fp, int finalize)
{
    int lbrk = 10, dbrk = 50;
    static int lines=0, dots=0;
    static struct timeval t1, t2;
    double elapsedTime;

    if (lines++ == 0) {
        gettimeofday(&t1, NULL);
    } else if (finalize || lines % lbrk == 0) {
        fprintf(fp,".");
        if (finalize || ++dots % dbrk == 0) {
            gettimeofday(&t2, NULL);
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
            elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
            fprintf(fp, " ( %'d items processed @ %'.3fk/sec )\n", lines,
                    ((lbrk*dbrk)/(elapsedTime/1000))/1000);
            gettimeofday(&t1, NULL);
        }
        fflush(fp);
    }
}

void usage()
{
    fprintf(stderr, "%s: Test program for seriald.\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  %s import db json_file\n", progname);
    fprintf(stderr, "  %s export db\n", progname);
    fprintf(stderr, "  %s sum db key\n", progname);
    fprintf(stderr, "  %s find db key\n", progname);
    fprintf(stderr, "  %s group db key\n", progname);
    fprintf(stderr, "\n");
    exit(1);
}


void print_record(srld_record *rec)
{
    bson_print_raw(rec->data, 1);
}


static void do_export(int argc, char **argv)
{
    srld_db db;
    srld_record *rec = NULL;
    srld_iterator it;

    if (argc != 3) usage();
    if (srld_db_open(argv[2], O_RDWR, &db) != SRLD_OK) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }
    srld_iterator_init(&db, &it);
    rec = srld_first(&it);
    while (rec) {
        print_record(rec);
        rec = srld_next(&it);
    }
    srld_db_close(&db);
}


static void do_import(int argc, char **argv)
{
    srld_db db;
    FILE *fp;
#define BUFSZ 1024*32
    char json[BUFSZ];

    if (argc != 4) usage();
    fp = fopen(argv[3], "r");
    if (!fp) return;
    if (srld_db_open(argv[2], O_RDWR|O_CREAT, &db) != SRLD_OK) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }

    while (fgets(json, BUFSZ, fp)) {
        if (srld_json_put(&db, json) != SRLD_OK) {
            fprintf(stderr,"writev() failed: %s\n", strerror(errno));
        }
    }
    print_stats(stderr, 1);
    fclose(fp);
    srld_db_close(&db);
}

int main( int argc, char **argv )
{
    progname = argv[0];
    if(argc > 1 && !strcmp(argv[1], "import")){
        do_import(argc, argv);
    } else if(argc > 1 && !strcmp(argv[1], "export")){
        do_export(argc, argv);
    } else {
        usage();
    }
    return 0;
}
