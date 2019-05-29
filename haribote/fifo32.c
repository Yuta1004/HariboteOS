#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001

// FIFOバッファの初期化
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task){
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;    // 書き込み位置
    fifo->q = 0;    // 読み込み位置
    fifo->task = task;
}

// FIFOバッファに値を追加
int fifo32_put(struct FIFO32 *fifo, int data){
    // 溢れ
    if(fifo->free == 0){
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }

    fifo->buf[fifo->p] = data;
    fifo->p ++;
    if(fifo->p == fifo->size) fifo->p = 0;
    fifo->free --;

    // タスク起動
    if(fifo->task != 0){
        if(fifo->task->flags != 2){
            task_run(fifo->task, -1, 0);
        }
    }
    return 0;
}

// FIFOバッファから値を取り出す
int fifo32_get(struct FIFO32 *fifo){
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
int fifo32_status(struct FIFO32 *fifo){
    return fifo->size - fifo->free;
}
