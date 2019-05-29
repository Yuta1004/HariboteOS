#include "bootpack.h"

// FAT圧縮を解読する
void read_fat(int *fat, unsigned char *img){
    int img_idx = 0;
    for(int fat_idx = 0; fat_idx < 2880; fat_idx += 2){
        fat[fat_idx + 0] = (img[img_idx + 0]      | img[img_idx + 1] << 8) & 0xfff;
        fat[fat_idx + 1] = (img[img_idx + 1] >> 4 | img[img_idx + 2] << 4) & 0xfff;
        img_idx += 3;
    }
}

// ファイルの内容を buf に転送する
void load_file(int clustno, int size, char *buf, int *fat, char *img){
    while(1){
        if(size <= 512){
            for(int idx = 0; idx < size; ++ idx){
                *(buf + idx) = img[clustno * 512 + idx];
            }
            break;
        }

        for(int idx = 0; idx < 512; ++ idx){
            *(buf + idx) = img[clustno * 512 + idx];
        }

        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
}

struct FILEINFO *file_search(char name[], struct FILEINFO *file_info, int max){
    char file_name[12];
    for(int idx = 0; idx < 11; ++ idx){
        file_name[idx] = ' ';
    }

    // ファイル名フォーマット
    int c_idx = 0;
    for(int idx = 0; name[idx] != 0 && c_idx < 11; ++ idx){
        if(name[idx] == '.' && c_idx <= 8){  // 拡張子
            c_idx = 8;
        }else{
            file_name[c_idx] = name[idx];
            if('a' <= file_name[c_idx] && file_name[c_idx] <= 'z'){
                file_name[c_idx] -= 0x20;
            }
            ++ c_idx;
        }
    }

    // ファイルを探す
    for(int idx = 0; idx < max; ++ idx){
        // これ以上ファイルはない
        if(file_info[idx].name[0] == 0x00) break;

        // 名前判定
        if((file_info[idx].type & 0x18) == 0){
            char same_flag = 1;
            for(int c_idx = 0; c_idx < 11; ++ c_idx){
                if(same_flag && (file_info[idx].name[c_idx] == 0 || file_name[c_idx] == 0)){
                    same_flag = 0;
                }
                same_flag &= (same_flag && file_info[idx].name[c_idx] == file_name[c_idx]);
            }

            if(same_flag) return file_info + idx;
        }
    }

    return 0;
}

char *load_file_from_tek(int clustno, int *psize, int *fat){
    // 圧縮済みファイルを読み出す
    int tek_size = *psize;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char *tek_buf = (char *) memman_alloc_4k(memman, tek_size), *buf;
    load_file(clustno, tek_size, tek_buf, fat, (char *) (ADR_DISKIMG + 0x003e00));

    // tekデコード
    if(tek_size >= 17){
        int size = tek_getsize(tek_buf);
        if(size > 2){
            buf = (char *) memman_alloc_4k(memman, size);
            tek_decomp(tek_buf, buf, size);
            memman_free_4k(memman, (int) tek_buf, tek_size);
            *psize = size;
            return buf;
        }
    }

    buf = tek_buf;
    return buf;
}