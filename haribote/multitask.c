#include "bootpack.h"

// マルチタスク管理用
struct TASK *task_init(struct MEMMAN *memman){
    // TASK_CTL初期化
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    task_ctl = (struct TASK_CTL *) memman_alloc_4k(memman, sizeof(struct TASK_CTL));
    for(int i = 0; i < MAX_TASKS; ++ i){
        task_ctl->tasks[i].flags = 0;
        task_ctl->tasks[i].sel = (TASK_GDT0 + i) * 8;
        task_ctl->tasks[i].tss.ldtr = (TASK_GDT0 + i + MAX_TASKS) * 8;
        task_ctl->tasks[i].priority = 1;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &task_ctl->tasks[i].tss, AR_TSS32);
        set_segmdesc(gdt + TASK_GDT0 + i + MAX_TASKS, 15, (int) task_ctl->tasks[i].ldt, AR_LDT);
    }

    // TASK_LEVEL初期化
    for(int level = 0; level < MAX_LEVELS; ++ level){
        task_ctl->level[level].running = 0;
        task_ctl->level[level].now = 0;
    }

    // task_initを呼んだ子の用のタスク
    struct TASK *task;
    task = task_alloc();
    task->flags = 2;
    task->priority = 2;
    task->level = 0;
    task_add(task);
    task_switch_sub();
    load_tr(task->sel);

    // タスクの番兵(アイドルタスク)
    struct TASK *idle_task;
    idle_task = task_alloc();
    idle_task->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
    idle_task->tss.eip = (int) &task_idle_main;
    idle_task->tss.es = 1 * 8;
    idle_task->tss.cs = 2 * 8;
    idle_task->tss.ss = 1 * 8;
    idle_task->tss.ds = 1 * 8;
    idle_task->tss.fs = 1 * 8;
    idle_task->tss.gs = 1 * 8;
    task_run(idle_task, MAX_LEVELS - 1, 1);

    // タスク切り替え用
    task_timer = timer_alloc();
    timer_set(task_timer, task->priority);

    return task;
}

struct TASK *task_alloc(void){
    struct TASK *task;
    for(int i = 0; i < MAX_TASKS; ++ i){
        if(task_ctl->tasks[i].flags == 0){
            task = &task_ctl->tasks[i];
            task->flags = 1;
            task->tss.eflags = 0x00000202;
            task->tss.eax = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.iomap = 0x40000000;
            task->tss.ss0 = 0;
            return task;
        }
    }
}

struct TASK *task_now(void){
    struct TASK_LEVEL *task_level = &task_ctl->level[task_ctl->now_level];
    return task_level->task_addr[task_level->now];
}

void task_add(struct TASK *task){
    struct TASK_LEVEL *task_level = &task_ctl->level[task->level];
    task_level->task_addr[task_level->running] = task;
    ++ task_level->running;
    task->flags = 2;
}

void task_remove(struct TASK *task){
    struct TASK_LEVEL *task_level = &task_ctl->level[task->level];

    // 指定タスクがいる場所を探す
    int idx = 0;
    for(; idx < task_level->running; ++ idx){
        if(task_level->task_addr[idx] == task) break;
    }

    // 値変更
    -- task_level->running;
    if(idx < task_level->now){
        -- task_level->now;
    }
    if(task_level->now >= task_level->running){
        task_level->now = 0;
    }
    task->flags = 1;

    // ずらしを行う
    for(; idx < task_level->running; ++ idx){
        task_level->task_addr[idx] = task_level->task_addr[idx + 1];
    }
}

void task_run(struct TASK *task, int level, int priority){
    if(level < 0) level = task->level;
    if(priority > 0) task->priority = priority;

    // 動作中タスクのレベル変更
    if(task->flags == 2 && task->level != level){
        task_remove(task);      // 引数のタスクのflagsは1になって帰ってくる
    }

    if(task->flags != 2){
        task->level = level;
        task_add(task);
    }

    // レベル関係が崩れた可能性があるので次のスイッチで見直す
    task_ctl->do_level_change = 1;
}

void task_switch(void){
    struct TASK_LEVEL *task_level = &task_ctl->level[task_ctl->now_level];
    struct TASK *now_task = task_level->task_addr[task_level->now];

    // 切り替え対象を変更
    ++ task_level->now;
    if(task_level->now == task_level->running){
        task_level->now = 0;
    }

    // レベル関係を見直す
    if(task_ctl->do_level_change == 1){
        task_switch_sub();
        task_level = &task_ctl->level[task_ctl->now_level];
    }

    // タスク切り替え
    struct TASK *new_task = task_level->task_addr[task_level->now];
    timer_set(task_timer, new_task->priority);
    if(now_task != new_task){
        far_jmp(0, new_task->sel);
    }
}

void task_switch_sub(void){
    // 一番上のレベルを探す
    int level = 0;
    for(; level < MAX_LEVELS; ++ level){
        if(task_ctl->level[level].running > 0) break;
    }
    task_ctl->now_level = level;
    task_ctl->do_level_change = 0;
}

void task_sleep(struct TASK *task){
    // 指定タスクが動作中でない
    if(task->flags != 2) return;

    // タスクを実行リストから削除
    struct TASK *now_task= task_now();
    task_remove(task);

    // 自分自身がスリープ
    if(task == now_task){
        task_switch_sub();
        now_task = task_now();
        far_jmp(0, now_task->sel);
    }
}

void task_idle_main(void){
    while(1){
        io_hlt();
    }
}
