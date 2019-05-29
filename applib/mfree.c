#include "hrbstd.h"

void mfree(void *p){
    char *q = p;
    if(q != 0){
        q -= 16;
        int size = *((int *) q);
        api_mfree(q, size);
    }
    return;
}