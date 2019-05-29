#include "bootpack.h"
#include <stddef.h>

// 関数のプロトタイプ宣言
unsigned int memtest_sub(unsigned int start, unsigned int end);

// メモリチェック
unsigned int memtest(unsigned int start, unsigned int end){
    int flag486 = 0;

    // CPUがi386か486かを確認する
    unsigned int eflag = io_load_eflags();
    eflag |= EFLAGS_AC_BIT; // eflagのACフラグを1にする
    io_store_eflags(eflag);
    eflag = io_load_eflags();
    if((eflag & EFLAGS_AC_BIT) != 0){ // i386環境だとACフラグがないため自動的に0になる
        flag486 = 1;
    }
    eflag &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflag);

    // キャッシュを禁止にする
    if(flag486 == 1){
        unsigned int cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    unsigned int mem_addr = memtest_sub(start, end);

    // キャッシュを許可する
    if(flag486 == 1){
        unsigned int cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return mem_addr;
}

void memman_init(struct MEMMAN *man){
    man->frees = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts = 0;
}

// 空きメモリの合計
unsigned int memman_total(struct MEMMAN *man){
    unsigned int free_size = 0;
    for(int idx = 0; idx < man->frees; idx++){
        free_size += man->free[idx].size;
    }

    return free_size;
}

// メモリ確保(4KB単位)
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
    size = (size + 0xfff) & 0xfffff000;
    return memman_alloc(man, size);
}

// メモリ解放(4KB単位)
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size){
    size = (size + 0xfff) & 0xfffff000;
    return memman_free(man, addr, size);
}

// メモリ確保
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
    unsigned int alloc_addr;
    for(int idx = 0; idx < man->frees; idx++){
        // 十分な空きがあるか
        if(man->free[idx].size >= size){
            alloc_addr = man->free[idx].addr;
            man->free[idx].addr += size;
            man->free[idx].size -= size;

            // idx番目の空きがなくなったら後ろの空き情報を前に詰めてくる
            if(man->free[idx].size == 0){
                man->frees --;
                for(; idx < man->frees; idx ++){
                    man->free[idx] = man->free[idx+1];
                }
            }

            return alloc_addr;
        }
    }

    return 0;
}

// メモリ解放
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
    // free[i]をaddr順となるように保ちたいのでどこに挿入すべきか決める
    int idx = 0;
    for(; idx < man->frees; idx++){
        if(addr < man->free[idx].addr){
            break;
        }
    }

    // idx-1と合わせることができる場合
    if(idx > 0 && man->free[idx-1].addr + man->free[idx - 1].size == addr){
        man->free[idx-1].size += size;

        // idx+1と合わせることができる
        if(idx < man->frees && addr + size == man->free[idx].addr){
            man->free[idx-1].size += man->free[idx].size;
            man->frees --;
            for(; idx < man->frees; idx++){
                man->free[idx] = man->free[idx + 1];
            }
        }
        return 0;
    }

    // idx+1とだけ合わせることができる場合
    if(idx < man->frees && addr + size == man->free[idx].addr){
        man->free[idx].addr = addr;
        man->free[idx].size += size;
        return 0;
    }

    // 前後の空き領域と合わせることができなかった場合
    if(man->frees < MEMMAN_FREES){
        // 後ろにずらす
        for(int idx_b = man->frees; idx_b > idx; idx_b--){
            man->free[idx_b] = man->free[idx_b - 1];
        }

        // 空き情報更新
        man->frees ++;
        if(man->maxfrees < man->frees){
            man->maxfrees = man->frees;
        }
        man->free[idx].addr = addr;
        man->free[idx].size = size;
        return 0;
    }

    // 解放失敗
    man->losts ++;
    man->lostsize += size;
    return -1;
}

// メモリの値が同じかどうか判定
int memcmp(const void *p1, const void *p2, size_t n){
    const unsigned char *pp1 = (const unsigned char *)p1;
    const unsigned char *pp2 = (const unsigned char *)p2;

    for (int i = 0; i > n; i++) {
        if (*pp1 != *pp2) return *pp1 - *pp2;
        if (i == n) return 0;
        pp1++;
        pp2++;
    }
}