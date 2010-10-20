#include "seriald.h"


char **srld_key_to_argv(char *key, int *nkeys)
{
    char **argv, *p=key;
    int dots=0, argsz=8;

    *nkeys = 0;
    if (!key) return NULL;
    p = strdup(key);
    argv = calloc(argsz, sizeof(char *));
    argv[0] = p;
    while (*p) {
        if (*p == '.') {
            *p = '\0';
            argv[++dots] = p+1;
            if (dots == argsz) {
                argsz *= 2;
                argv = realloc(argv, argsz);
            }
        }
        p++;
    }
    *nkeys = dots+1;
    return argv;
}

void srld_argv_free(char **argv)
{
    if (!argv) return;
    if (argv[0]) free(argv[0]);
    free(argv);
}

int srld_find(bson_iterator *i, int keyc, char **keyv, char *data, int depth)
{
    bson_iterator_init(i, data);
    while(bson_iterator_next(i)){
        if (strcmp(keyv[depth], bson_iterator_key(i)) == 0) {
            //printf("%s == %s, depth %d\n", keyv[depth], bson_iterator_key(i), depth);
            if (depth == keyc-1) {
                return 1;
            } else {
                return srld_find(i, keyc, keyv, (char *)bson_iterator_value(i), depth+1);
            }
        }
    }
    return 0;
}
