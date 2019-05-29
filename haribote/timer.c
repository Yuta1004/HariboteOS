#include "bootpack.h"

void init_pit(void){
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9e);
    io_out8(PIT_CNT0, 0x2c);

    timer_ctl.count = 0;
    timer_ctl.next = 0xffffffff;
    for(int idx = 0; idx < MAX_TIMER; ++ idx){
        timer_ctl.timer[idx].flags = 0;
    }

    // 番兵の設置
    struct TIMER *tmp_timer = timer_alloc();
    tmp_timer->timeout = 0xffffffff;
    tmp_timer->flags = TIMER_FLAG_USING;
    tmp_timer->next_timer = 0;
    timer_ctl.timer_lead = tmp_timer;
    timer_ctl.next = 0xffffffff;
}

// 確保されていないタイマーを見つけて返す
struct TIMER *timer_alloc(void){
    for(int idx = 0; idx < MAX_TIMER; ++ idx){
        if(timer_ctl.timer[idx].flags == 0){
            timer_ctl.timer[idx].flags = TIMER_FLAG_ALLOC;
            timer_ctl.timer[idx].flags_auto_cancel = 0;
            return &timer_ctl.timer[idx];
        }
    }
    return 0;
}

// 使い終わったタイマーを解放する
void timer_free(struct TIMER *timer){
    timer->flags = 0;
}

// タイマー初期化
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned int data){
    timer->fifo = fifo;
    timer->data = data;
}

// タイマーに時間をセットする
void timer_set(struct TIMER *timer, unsigned int timeout){
    // 割り込み禁止
    int eflags = io_load_eflags();
    io_cli();

    // 時間セット
    timer->timeout = timer_ctl.count + timeout;
    timer->flags = TIMER_FLAG_USING;

    // LinkedListの適切な位置にTimerを配置する処理
    // 1.先頭に配置する場合
    if(timer->timeout <= timer_ctl.timer_lead->timeout){
        timer->next_timer = timer_ctl.timer_lead;
        timer_ctl.timer_lead = timer;
        timer_ctl.next = timer->timeout;
        io_store_eflags(eflags);
        return;
    }

    // 2.配置する場所を全探索
    struct TIMER *bef_timer, *aft_timer = timer_ctl.timer_lead;
    while(1){
        bef_timer = aft_timer;
        aft_timer = aft_timer->next_timer;
        if(aft_timer == 0) break;

        if(timer->timeout <= aft_timer->timeout){
            bef_timer->next_timer = timer;
            timer->next_timer = aft_timer;
            io_store_eflags(eflags);
            return;
        }
    }
}

// タイマーキャンセル
int timer_cancel(struct TIMER *timer){
    int eflags = io_load_eflags();
    io_cli();

    // 取消し処理が必要か判断
    if(timer->flags == TIMER_FLAG_USING){
        if(timer == timer_ctl.timer_lead){      // TIMER_CTLの先頭タイマーを取り消す
            struct TIMER *next_timer = timer->next_timer;
            timer_ctl.timer_lead = next_timer;
            timer_ctl.next = next_timer->timeout;
        }else{                                  // TIMER_CTLの先頭以外のタイマーを取り消す
            struct TIMER *timer_select = timer_ctl.timer_lead;
            while(1){
                if(timer_select->next_timer == timer){
                    break;
                }
                timer_select = timer_select->next_timer;
            }
            timer_select->next_timer = timer->next_timer;
        }

        timer->flags = TIMER_FLAG_ALLOC;
        io_store_eflags(eflags);
        return 1;
    }

    io_store_eflags(eflags);
    return 0;
}

// アプリが使用していたタイマを全てキャンセル
void timer_all_cancel(struct FIFO32 *fifo){
    int eflags = io_load_eflags();
    io_cli();

    for(int idx = 0; idx < MAX_TIMER; ++ idx){
        struct TIMER *timer = &(timer_ctl.timer[idx]);
        if(timer->flags != 0 && timer->flags_auto_cancel != 0 && timer->fifo == fifo){
            timer_cancel(timer);
            timer_free(timer);
        }
    }

    io_store_eflags(eflags);
    return;
}