#include "hrbstd.h"

void *malloc(int size){
    char *p = api_malloc(size);
    if(*p != 0){
        *((int *) p) = size;
        p += 16;
    }

    return p;
}
