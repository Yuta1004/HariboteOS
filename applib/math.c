#include "hrbstd.h"

/*  min  */
int min(int comp_a, int comp_b){
    if(comp_a > comp_b){
        return comp_b;
    }else{
        return comp_a;
    }
}

/*  max  */
int max(int comp_a, int comp_b){
    if(comp_a >= comp_b){
        return comp_a;
    }else{
        return comp_b;
    }
}
