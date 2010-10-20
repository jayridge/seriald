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
    fprintf(stderr, "\n");
    exit(1);
}

static void do_import(int argc, char **argv)
{
    FILE *fp;
#define BUFSZ 1024*32
    char line[BUFSZ], *b;
    struct iovec iov[3];
    int rc, fd, size=0;
    uint32_t prev_sz = 0;

    if (argc != 4) usage();
    fp = fopen(argv[3], "r");
    if (!fp) return;
    fd = open(argv[2], O_RDWR|O_APPEND|O_CREAT, 0655);
    if (fd < 0) {
        fprintf(stderr,"open(%s) failed: %s\n", argv[2], strerror(errno));
        exit(errno);
    }
    while (fgets(line, BUFSZ, fp)) {
        if ((b = json_to_bson(line, &size))) {
            print_stats(stderr, 0);
            iov[0].iov_base = (caddr_t) &size;
            iov[0].iov_len = sizeof(int);
            iov[1].iov_base = (caddr_t) &prev_sz;
            iov[1].iov_len = sizeof(int);
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
    int fd, size, prev_sz;
    struct iovec iov[2];
    char buf[1024*32];
    bson_iterator i;
    double val, sum=0;


    if (argc != 4) usage();
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
        buf[0] = '\0';
        read(fd, buf, size);
        bson_iterator_init(&i, buf);
        while(bson_iterator_next(&i)){
            if (strcmp(argv[3], bson_iterator_key(&i)) == 0) {
                val = bson_iterator_double(&i);
                printf("%f\n", val);
                sum += val;
            }
        }
    }
    printf("sum: %f\n", sum);

    close(fd);
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
    } else {
        usage();
    }
    return 0;
}
