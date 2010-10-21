#include "seriald.h"

char *progname = "srldtest";

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
    fprintf(stderr, "  %s dump db\n", progname);
    fprintf(stderr, "  %s sum db key\n", progname);
    fprintf(stderr, "  %s find db key\n", progname);
    fprintf(stderr, "  %s group db key\n", progname);
    fprintf(stderr, "\n");
    exit(1);
}

static void do_import(int argc, char **argv)
{
    FILE *fp;
#define BUFSZ 1024*32
    char line[BUFSZ], *b;
    struct iovec iov[3];
    int rc, fd, size=0, prev_sz=0;

    if (argc != 4) usage();
    fp = fopen(argv[3], "r");
    if (!fp) return;
    fd = open(argv[2], O_RDWR|O_APPEND|O_CREAT, 0655);
    if (fd < 0) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }

    iov[0].iov_base = (caddr_t) &size;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = (caddr_t) &prev_sz;
    iov[1].iov_len = sizeof(int);

    while (fgets(line, BUFSZ, fp)) {
        if ((b = json_to_bson(line, &size))) {
            print_stats(stderr, 0);
            iov[2].iov_base = b;
            iov[2].iov_len = size;
            rc = writev(fd, iov, 3);
            prev_sz = size;
            if (rc < 0) {
                fprintf(stderr,"writev() failed: %s\n", strerror(errno));
            }
        }
    }
    print_stats(stderr, 1);
    close(fd);
}

static void do_read(int argc, char **argv)
{
    int fd, size, prev_sz;
    struct iovec iov[2];
    char buf[1024*32];

    if (argc != 3) usage();
    fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }

    iov[0].iov_base = (caddr_t) &size;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = (caddr_t) &prev_sz;
    iov[1].iov_len = sizeof(int);
    while (readv(fd, iov, 2)) {
        fprintf(stdout, "size: %d\nprev: %d\n", size, prev_sz);
        read(fd, buf, size);
        bson_print_raw(buf, 1);
    }
    close(fd);
}

static void do_sum(int argc, char **argv)
{
    int fd, size, prev_sz, keyc;
    struct iovec iov[2];
    char buf[1024*32];
    char **keyv;
    bson_iterator i;
    double val, sum=0;


    if (argc != 4) usage();
    fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }

    keyv = srld_key_to_argv(argv[3], &keyc);
    iov[0].iov_base = (caddr_t) &size;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = (caddr_t) &prev_sz;
    iov[1].iov_len = sizeof(int);
    while (readv(fd, iov, 2)) {
        if (read(fd, buf, size) == size && srld_find(&i, keyc, keyv, buf, 0)) {
            val = bson_iterator_double(&i);
            sum += val;
        }
    }
    printf("sum: %f\n", sum);

    close(fd);
}


static void do_find(int argc, char **argv)
{
    int fd, size, prev_sz, keyc;
    struct iovec iov[2];
    char buf[1024*32];
    char **keyv;
    bson_iterator i;

    if (argc != 4) usage();
    fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }

    keyv = srld_key_to_argv(argv[3], &keyc);
    iov[0].iov_base = (caddr_t) &size;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = (caddr_t) &prev_sz;
    iov[1].iov_len = sizeof(int);
    while (readv(fd, iov, 2)) {
        if (read(fd, buf, size) == size && srld_find(&i, keyc, keyv, buf, 0)) {
            bson_type t = bson_iterator_type(&i);
            switch (t) {
                case bson_int: printf("%d\n" ,bson_iterator_int(&i)); break;
                case bson_double: printf("%f\n" ,bson_iterator_double(&i)); break;
                case bson_bool: printf("%s\n" ,bson_iterator_bool(&i) ? "true" : "false"); break;
                case bson_string: printf("%s\n" ,bson_iterator_string(&i)); break;
                case bson_null: printf("null\n"); break;
                case bson_oid:
                case bson_object:
                case bson_array:
                default:
                    fprintf(stderr ,"can't print type : %d\n" ,t);
            }
        }
    }
    srld_argv_free(keyv);

    close(fd);
}


static void do_group(int argc, char **argv)
{
    int fd, size, prev_sz, keyc;
    struct iovec iov[2];
    char buf[1024*32];
    char **keyv;
    bson_iterator i;
    TCMAP *counts;
    int *count;
    const char *key;
    
    counts = tcmapnew();
    
    if (argc != 4) usage();
    fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }

    keyv = srld_key_to_argv(argv[3], &keyc);
    iov[0].iov_base = (caddr_t) &size;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = (caddr_t) &prev_sz;
    iov[1].iov_len = sizeof(int);
    while (readv(fd, iov, 2)) {
        if (read(fd, buf, size) == size && srld_find(&i, keyc, keyv, buf, 0)) {
            bson_type t = bson_iterator_type(&i);
            switch (t) {
                case bson_int: printf("%d\n" ,bson_iterator_int(&i)); break;
                case bson_double: printf("%f\n" ,bson_iterator_double(&i)); break;
                case bson_bool: printf("%s\n" ,bson_iterator_bool(&i) ? "true" : "false"); break;
                case bson_string: 
                  tcmapaddint(counts, bson_iterator_string(&i), strlen(bson_iterator_string(&i)), 1);
                  break;
                printf("%s\n" ,bson_iterator_string(&i)); break;
                case bson_null: printf("null\n"); break;
                case bson_oid:
                case bson_object:
                case bson_array:
                default:
                    fprintf(stderr ,"can't print type : %d\n" ,t);
            }
        }
    }
    srld_argv_free(keyv);

    close(fd);
    
    tcmapiterinit(counts);
    while((key = tcmapiternext2(counts)) != NULL) {
      count = (int *)tcmapget(counts, key, strlen(key), &size);
      fprintf(stderr, "%s: %d\n", key, *count);
    }
}

int main(int argc, char **argv)
{
    progname = argv[0];
    if(argc > 1 && !strcmp(argv[1], "import")){
        do_import(argc, argv);
    } else if(argc > 1 && !strcmp(argv[1], "dump")){
        do_read(argc, argv);
    } else if(argc > 1 && !strcmp(argv[1], "sum")){
        do_sum(argc, argv);
    } else if(argc > 1 && !strcmp(argv[1], "find")){
        do_find(argc, argv);
    } else if(argc > 1 && !strcmp(argv[1], "group")){
        do_group(argc, argv);
    } else {
        usage();
    }
    return 0;
}
