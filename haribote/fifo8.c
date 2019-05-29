#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001

// FIFOバッファの初期化
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;    // 書き込み位置
    fifo->q = 0;    // 読み込み位置
}

// FIFOバッファに値を追加
int fifo8_put(struct FIFO8 *fifo, unsigned char data){
    // 溢れ
    if(fifo->free == 0){
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }

    fifo->buf[fifo->p] = data;
    fifo->p ++;
    if(fifo->p == fifo->size) fifo->p = 0;
    fifo->free --;
    return 0;
}

// FIFOバッファから値を取り出す
int fifo8_get(struct FIFO8 *fifo){
    // 要素なし
    if(fifo->free == fifo->size){
        return -1;
    }

    int data = fifo->buf[fifo->q];
    fifo->q ++;
    if(fifo->q == fifo->size) fifo->q = 0;
    fifo->free ++;
    return data;
}

// FIFOバッファにどれだけデータが溜まってるかを返す
int fifo8_status(struct FIFO8 *fifo){
    return fifo->size - fifo->free;
}
