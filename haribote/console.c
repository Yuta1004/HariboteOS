#include <stdarg.h>
#include "bootpack.h"

void console_newline(struct CONSOLE *console);
void console_command_exec(struct CONSOLE *console, char command[]);
void command_mem(struct CONSOLE *console);
void command_clear(struct CONSOLE *console);
void command_ls(struct CONSOLE *console);
void command_help(struct CONSOLE *console);
int command_app(struct CONSOLE *console, char command[]);
void command_exit();
void command_start(struct CONSOLE *console, char command[]);
void command_open(struct CONSOLE *console, char command[]);
void command_langmode(struct CONSOLE *console);

void task_console_main(struct SHEET *sheet){
    // 自分が動いているタスクを取得
    struct TASK *now_task = task_now();

    // 入力されたコマンドを保持する
    char command[40];
    int command_size = 0;

    // コンソール初期化
    struct CONSOLE console;
    console.sheet = sheet;
    console.cursor_x = 8;
    console.cursor_y = 28;
    console.cursor_color = -1;
    now_task->console = &console;
    now_task->command = command;

    // FILEHANDLE初期化
    struct FILEHANDLE filehandle[8];
    for(int idx = 0; idx < 8; ++ idx){
        filehandle[idx].buf = 0;
    }
    now_task->fhandle = filehandle;

    // カーソル点滅用
    if(console.sheet != 0){
        console.cursor_timer = timer_alloc();
        timer_init(console.cursor_timer, &now_task->fifo, 1);
        timer_set(console.cursor_timer, 25);
    }

    // 言語モード設定
    unsigned char *japanese = (unsigned char *) *((int *) 0xfe8);
    now_task->langmode = (japanese[4096] != 0xff);
    now_task->langbyte1 = 0;

    // 文字表示用
    console_putchar(&console, '>', 1);

    while(1){
        io_cli();

        /*  FIFOが空だった  */
        if(fifo32_status(&now_task->fifo) == 0){
            task_sleep(now_task);
            io_sti();
            continue;
        }

        /*  FIFOデータ受け取り */
        int data = fifo32_get(&now_task->fifo);
        io_sti();

        /*  キー入力  */
        if(256 <= data && data <= 511){
            if(data == 8 + 256){            // BackSpace
                if(console.cursor_x > 16){
                    console_putchar(&console, ' ', 0);
                    console.cursor_x -= 8;
                    console_putchar(&console, ' ', 0);
                    -- command_size;
                }
            }
            else if(data == 10 + 256){     // Enter
                // 改行
                console_putchar(&console, ' ', 0);
                console_newline(&console);

                // コマンド実行
                command[command_size] = 0;
                console_command_exec(&console, command);
                command_size = 0;

                // プロンプト
                console_putchar(&console, '>', 1);

                // コンソール立ち上げなしでアプリを実行した時
                if(console.sheet == 0){
                    command_exit(&console);
                }
            }
            else if(console.cursor_x < 384){       // 一般文字
                console_putchar(&console, data - 256, 1);
                command[command_size] = data - 256;
                ++ command_size;
            }

            // カーソル再表示
            if(console.sheet != 0){
                boxfill8(sheet->buf, sheet->buf_width, console.cursor_color,
                        console.cursor_x, console.cursor_y, console.cursor_x + 7, console.cursor_y + 15);
                sheet_refresh(sheet, console.cursor_x, console.cursor_y, console.cursor_x + 8, console.cursor_y + 16);
            }
            continue;
        }

        /*  カーソル  */
        if(data <= 1){
            timer_init(console.cursor_timer, &now_task->fifo, 1 - data);
            timer_set(console.cursor_timer, 50);

            if(console.cursor_color >= 0){
                int colors[] = {COL8_000000, COL8_FFFFFF};
                console.cursor_color = colors[1 - data];
                if(console.sheet != 0){
                    boxfill8(console.sheet->buf, console.sheet->buf_width, console.cursor_color,
                            console.cursor_x, console.cursor_y, console.cursor_x + 7, console.cursor_y + 15);
                    sheet_refresh(console.sheet, console.cursor_x, console.cursor_y, console.cursor_x + 8, console.cursor_y + 16);
                }
            }
            continue;
        }

        /*  カーソル表示・非表示処理  */
        if(2 <= data && data <= 3){
            console.cursor_color = (data - 2) ? 0 : -1;
            if(console.cursor_color < 0 && console.sheet != 0){
                boxfill8(console.sheet->buf, console.sheet->buf_width, COL8_000000,
                         console.cursor_x, console.cursor_y, console.cursor_x + 7, console.cursor_y + 15);
                sheet_refresh(console.sheet, console.cursor_x, console.cursor_y, console.cursor_x + 8, console.cursor_y + 16);
            }
        }

        /* コンソール終了要請 */
        if(data == 4){
            command_exit(&console);
        }
    }
}

// 1文字出力
void console_putchar(struct CONSOLE *console, int char_id, char move){
    char s[2];
    s[0] = char_id;
    s[1] = 0;

    if(s[0] == 0x09){   // \t
        while(1){
            if(console->sheet != 0){
                putstr8_ref(console->sheet, console->cursor_x, console->cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
            }
            console->cursor_x += 8;

            if(console->cursor_x == 8 + 384){
                console_newline(console);
            }
            if(((console->cursor_x - 8) % 0x1f) == 0){
                break;
            }
        }
        return;
    }

    if(s[0] == 0x0a){   // \n
        console_newline(console);
        return;
    }

    if(s[0] == 0x0d){   // do nothing
        return;
    }

    // 通常文字
    if(console->sheet != 0){
        putstr8_ref(console->sheet, console->cursor_x, console->cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
    }
    if(move != 0){
        console->cursor_x += 8;
        if(console->cursor_x == 8 + 384){
            console_newline(console);
        }
    }
    return;
}

// 1行出力([0]文字が来るまで)
void console_putstr(struct CONSOLE *console, char *str){
    while(*str != 0){
        console_putchar(console, *str, 1);
        ++ str;
    }
    return;
}

// 1行出力(サイズ指定)
void console_putstr_with_size(struct CONSOLE *console, char *str, int size){
    for(int idx = 0; idx < size; ++ idx){
        console_putchar(console, *str, 1);
        ++ str;
    }
    return;
}

void console_newline(struct CONSOLE *console){
    int x0 = 8, y0 = 28, width = 384, height = 256;
    struct TASK *task = task_now();

    // 改行
    if(console->cursor_y < 28 + height - 16){
        console->cursor_y += 16;
    }else{
        if(console->sheet != 0){
            // ずらし
            for(int y = y0; y < y0 + height - 16; ++ y){
                for(int x = x0; x < x0 + width; ++ x){
                    console->sheet->buf[x + y * console->sheet->buf_width] = console->sheet->buf[x + (y + 16) * console->sheet->buf_width];
                }
            }

            // 新しい行を作成
            for(int y = y0 + height - 16; y < y0 + height; ++ y){
                for(int x = x0; x < x0 + width; ++ x){
                    console->sheet->buf[x + y * console->sheet->buf_width] = COL8_000000;
                }
            }
            sheet_refresh(console->sheet, x0, y0, x0 + width, y0 + height);
        }
    }

    console->cursor_x = 8;
    if(task->langmode == 1 && task->langbyte1 != 0){
        console->cursor_x += 8;
    }
    return;
}

void console_command_exec(struct CONSOLE *console, char command[]){
    /*  nothing input  */
    if(command[0] == 0) return;

    /* default command */
    if(console->sheet != 0){
        /*  mem  */
        if(mstrcmp(command, "mem") == 0){
            command_mem(console);
            return;
        }

        /*  clear  */
        if(mstrcmp(command, "clear") == 0){
            command_clear(console);
            return;
        }

        /*  ls  */
        if(mstrcmp(command, "ls") == 0){
            command_ls(console);
            return;
        }

        /*  help  */
        if(mstrcmp(command, "help") == 0){
            command_help(console);
            return;
        }

        /* exit */
        if(mstrcmp(command, "exit") == 0){
            command_exit(console);
            return;
        }

        /* langmode */
        if(mstrncmp(command, "langmode ", 9) == 0){
            command_langmode(console);
            return;
        }
    }

    /* start */
    if(mstrncmp(command, "start ", 6) == 0){
        command_start(console, command);
        return;
    }

    if(mstrncmp(command, "open ", 5) == 0){
        command_open(console, command);
        return;
    }

    /*  exec app */
    int exec_result = command_app(console, command);
    if(exec_result == 1) return;

    /* bad command */
    console_putstr(console, "Bad Command...\n\n");
    return;
}

void command_mem(struct CONSOLE *console){
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    char s[40];
    msprintf(s, "total : %d MB\n", mem_size / (1024 * 1024));
    console_putstr(console, s);
    msprintf(s, "free : %d MB\n", memman_total(memman) / (1024 * 1024));
    console_putstr(console, s);
    console_newline(console);
}

void command_clear(struct CONSOLE *console){
    for(int y = 28; y < 28 + 256; ++ y){
        for(int x = 8; x < 8 + 384; ++ x){
            console->sheet->buf[x + y * console->sheet->buf_width] = COL8_000000;
        }
    }
    sheet_refresh(console->sheet, 8, 28, 8 + 384, 28 + 256);
    console->cursor_y = 28;
}

void command_ls(struct CONSOLE *console){
    // ファイル情報が置かれている所のポインタ
    struct FILEINFO *file_info = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);

    // ファイル情報読み込み->表示
    for(int idx = 0; idx < 224; ++ idx){
        // 終わり
        if(file_info[idx].name[0] == 0x00){
            break;
        }

        // ファイル情報がある
        if(file_info[idx].name[0] != 0xe5 && (file_info[idx].type & 0x18) == 0){
            char s[30];
            msprintf(s, "filename.ext    : %d\n", file_info[idx].size);
            for(int n_idx = 0; n_idx < 8; ++ n_idx){
                s[n_idx] = file_info[idx].name[n_idx];
            }
            s[9] = file_info[idx].ext[0];
            s[10] = file_info[idx].ext[1];
            s[11] = file_info[idx].ext[2];
            console_putstr(console, s);
        }
    }

    console_newline(console);
}

void command_help(struct CONSOLE *console){
    char help_list[7][50] = {
        "mem            : Display memory infomation.\n",
        "ls             : List directory contents.\n",
        "clear          : Clear the console screen.\n",
        "help           : Display default-command list.\n",
        "exit           : Exit Console.\n",
        "start [comamnd]: Exec command in new console.\n",
        "open [comamnd] : Exec app.\n"
    };

    for(int idx = 0; idx < 7; ++ idx){
        console_putstr(console, help_list[idx]);
        for(int c_idx = 0; c_idx < 45; ++ c_idx){
            help_list[idx][c_idx] = '\0';
        }
    }

    console_newline(console);
}

int command_app(struct CONSOLE *console, char command[]){
    // ファイル名取り出し
    char file_name[18];
    for(int idx = 0; idx < 18; ++ idx){
        file_name[idx] = ' ';
    }

    int idx = 0;
    for(; idx < 13; ++ idx){
        if(command[idx] <= ' ') break;
        file_name[idx] = command[idx];
    }
    file_name[idx] = 0;

    // 実行ファイル存在確認
    struct FILEINFO *file_info = file_search(file_name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if(file_info == 0 && file_name[idx - 1] != '.'){
        file_name[idx] = '.';
        file_name[idx + 1] = 'h';
        file_name[idx + 2] = 'r';
        file_name[idx + 3] = 'b';
        file_name[idx + 4] = 0;
        file_info = file_search(file_name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }

    // 実行
    if(file_info != 0){
        struct TASK *task = task_now();
        struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
        struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
        int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
        char *exec_addr = (char *) memman_alloc_4k(memman, file_info->size);
        int cursor_x = 8;

        // ファイル読み込み
        read_fat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
        load_file(file_info->clustno, file_info->size, exec_addr, fat, (char *) (ADR_DISKIMG + 0x003e00));

        // 実行ファイルのシグネチャがHariなら実行
        if(file_info->size >= 36 && mstrncmp(exec_addr + 4, "Hari", 4) == 0 && *exec_addr == 0x00){
            // .hrb内のデータ情報を持ってくる
            int seg_size = *((int *) (exec_addr + 0x0000));
            int esp = *((int *) (exec_addr + 0x000c));
            int data_size = *((int *) (exec_addr + 0x0010));
            int data_hrb_addr = *((int *) (exec_addr + 0x0014));
            char *app_mem_addr = (char *) memman_alloc_4k(memman, seg_size);

            // セグメント設定
            // アクセス権に0x60を足すとアプリケーション用のセグメントになる
            task->ds_base = (int) app_mem_addr;
            set_segmdesc(task->ldt + 0, file_info->size - 1, (int) exec_addr, AR_CODE32_ER + 0x60);
            set_segmdesc(task->ldt + 1, seg_size - 1, (int) app_mem_addr, AR_DATA32_RW + 0x60);

            // データをデータセグメントに移す
            for(int idx = 0; idx < data_size; ++ idx){
                app_mem_addr[esp + idx] = exec_addr[data_hrb_addr + idx];
            }

            // 実行
            char s[40];
            start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));

            // ウィンドウが閉じられていなかったら閉じる
            struct SHEET_CTL *sheet_ctl = (struct SHEET_CTL *) *((int *) 0xfe4);
            for(int sheet_idx = 0; sheet_idx < sheet_ctl->top; ++ sheet_idx){
                struct SHEET *sheet = sheet_ctl->sheets[sheet_idx];
                if((sheet->flags & 0x11) == 0x11 && sheet->task == task){
                    sheet_free(sheet);
                    break;
                }
            }

            // ファイルが閉じられていなかったら閉じる
            for(int f_idx = 0; f_idx < 8; ++ f_idx){
                if(task->fhandle[f_idx].buf != 0){
                    console_putstr(task->console, "test\n");
                    memman_free_4k(memman, (int) task->fhandle[f_idx].buf, task->fhandle[f_idx].size);
                    task->fhandle[f_idx].buf = 0;
                }
            }

            // 後処理1
            task->langbyte1 = 0;
            timer_all_cancel(&task->fifo);
            memman_free_4k(memman, (int) app_mem_addr, seg_size);
        }else{
            console_putstr(console, ".hrb File Format Error!\n");
        }

        // 後処理2
        memman_free_4k(memman, (int) fat, 4 * 2880);
        memman_free_4k(memman, (int) exec_addr, file_info->size);
        console_newline(console);
        return 1;
    }

    return 0;
}

// コンソール終了コマンド
void command_exit(struct CONSOLE *console){
    // 終了処理
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_now();
    struct SHEET_CTL *sheet_ctl = (struct SHEET_CTL *) *((int *) 0xfe4);
    struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0xfec);
    timer_cancel(console->cursor_timer);

    // メインタスクに自分を殺すよう要請を出す
    io_cli();
    if(console->sheet == 0){
        fifo32_put(fifo, task - task_ctl->tasks + 1024);                    // 1024 ~ 2023
    }else{
        fifo32_put(fifo, console->sheet - sheet_ctl->sheet_data + 768);     // 768 ~ 1023
    }
    io_sti();

    // 殺されるまで待機
    while(1){
        task_sleep(task);
    }
}

// 指定アプリを別ウィンドウで実行
void command_start(struct CONSOLE *console, char command[]){
    // 新規コンソール立ち上げ
    struct SHEET_CTL *sheet_ctl = (struct SHEET_CTL *) *((int *) 0xfe4);
    struct SHEET *sheet = open_console(sheet_ctl, mem_size);
    struct FIFO32 *fifo = &sheet->task->fifo;
    sheet_slide(sheet, 300, 300);
    sheet_updown(sheet, sheet_ctl->top);

    // 引数に指定されたコマンドを実行
    for(int idx = 6; command[idx] != 0; ++ idx){
        fifo32_put(fifo, command[idx] + 256);
    }
    fifo32_put(fifo, 256 + 0x0a);
    console_newline(console);
    return;
}

// コマンド実行(新規コンソール立ち上げなし)
void command_open(struct CONSOLE *console, char command[]){
    // 新規コンソールタスク立ち上げ
    struct TASK *task = open_console_task(0);
    struct FIFO32 *fifo = &task->fifo;

    // 引数に指定されたコマンドを実行
    for(int idx = 5; command[idx] != 0; ++ idx){
        fifo32_put(fifo, command[idx] + 256);
    }
    fifo32_put(fifo, 256 + 0x0a);
    console_newline(console);
    return;
}

// 言語モード変更
void command_langmode(struct CONSOLE *console){
    struct TASK *task = task_now();
    unsigned char mode = task->command[9] - '0';
    if(mode <= 2){
        task->langmode = mode;
    }else{
        console_putstr(console, "Can't change the langmode.");
    }
    console_newline(console);
    return;
}

// 新規コンソールタスク作成
struct TASK *open_console_task(struct SHEET * sheet){
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_alloc();
    int *fifo_console = (int *) memman_alloc_4k(memman, 128 * 4);

    // タスク設定
    task->stack_addr = memman_alloc_4k(memman, 64 * 1024);
    task->tss.esp = task->stack_addr + 64 * 1024 - 8;
    task->tss.eip = (int) &task_console_main;
    task->tss.es = 1 * 8;
    task->tss.cs = 2 * 8;
    task->tss.ss = 1 * 8;
    task->tss.ds = 1 * 8;
    task->tss.fs = 1 * 8;
    task->tss.gs = 1 * 8;
    *((int *) (task->tss.esp + 4)) = (int) sheet;
    task_run(task, 2, 2);
    fifo32_init(&task->fifo, 128, fifo_console, task);

    return task;
}

// 新規コンソール作成
struct SHEET *open_console(struct SHEET_CTL *sheet_ctl, unsigned int mem_size){
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHEET *sheet = sheet_alloc(sheet_ctl);
    unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 400 * 300);

    // バッファ登録 -> ウィンドウ描画
    sheet_setbuf(sheet, buf, 400, 300, -1);
    make_window(buf, 400, 300, "Console", 0);
    make_textbox8(sheet, 8, 28, 384, 256, COL8_000000);

    // シート設定
    sheet->task = open_console_task(sheet);
    sheet->flags |= 0x20;

    return sheet;
}

// コンソールタスクを終了
void kill_console_task(struct TASK *task){
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    task_sleep(task);
    memman_free_4k(memman, task->stack_addr, 64 * 1024);
    memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
    task->flags = 0;    // task_freeの代わり
    return;
}

// コンソールを閉じる
void close_console(struct SHEET *sheet){
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = sheet->task;
    memman_free_4k(memman, (int) sheet->buf, 400 * 300);
    sheet_free(sheet);
    kill_console_task(task);
    return;
}
