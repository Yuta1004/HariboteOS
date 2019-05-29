#include <stddef.h>

// パレット対応表
#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15

// nasmfuncにある関数
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int flag);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler00(void);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler2C(void);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void far_jmp(int eip, int cs);
void far_call(int eip, int cs);
void asm_haribote_api(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
int start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void end_app();

// 起動情報が置かれている番地
#define ADR_BOOTINFO	0x00000ff0

// ファイル情報が置かれている番地
#define ADR_DISKIMG     0x00100000

// 起動情報を保持する構造体
struct BOOTINFO{
    char cyls, leds, vmomde, reserve;
    short width, height;
    char *vram;
};

// GDT情報を保持する構造体
struct SEGMENT_DESCRIPTOR{
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

// IDT情報を保持する構造体
struct GATE_DESCRIPTOR{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

/*  memory.c */
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMMAN_FREES 4096
#define MEMMAN_ADDR 0x003c0000

struct FREEINFO{    // メモリの空き情報
    unsigned int addr;
    unsigned int size;
};
struct MEMMAN{      // メモリ管理くん
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
int memcmp(const void *p1, const void *p2, size_t n);


/* sheet.c */
#define MAX_SHEETS 256
#define SHEET_USE 1

struct SHEET{       // シート情報
    unsigned char *buf;
    int buf_width, buf_height, vram_x, vram_y, col_inv, layer_height, flags;
    struct SHEET_CTL *sheet_ctl;
    struct TASK *task;
};
struct SHEET_CTL{   // シート管理
    unsigned char *vram, *g_map;
    int width, height, top;
    struct SHEET *sheets[MAX_SHEETS];
    struct SHEET sheet_data[MAX_SHEETS];
};
struct SHEET_CTL *sheet_ctl_init(struct MEMMAN *memman, unsigned char *vram, int width, int height);
struct SHEET *sheet_alloc(struct SHEET_CTL *sheet_ctl);
void sheet_setbuf(struct SHEET *sheet, unsigned char *buf, int width, int height, int col_inv);
void sheet_updown(struct SHEET *sheet, int height);
void sheet_refresh_map(struct SHEET_CTL *sheet_ctl, int vram_x0, int vram_y0, int vram_x1, int vram_y1, int s_height);
void sheet_refresh(struct SHEET *sheet, int buf_x0, int buf_y0, int buf_x1, int buf_y1);
void sheet_refresh_with_range(struct SHEET_CTL *sheet_ctl, int vram_x0, int vram_y0, int vram_x1, int vram_y1, int s_height, int e_height);
void sheet_slide(struct SHEET *sheet, int vram_x, int vram_y);
void sheet_free(struct SHEET *sheet);

/*  msprintf.c  */
void msprintf (char *str, char *fmt, ...);

/*  functions.c  */
char mstrcmp(char str_a[], char str_b[]);
char mstrncmp(char str_a[], char str_b[], int size);
int min(int comp_a, int comp_b);
int max(int comp_a, int comp_b);

/*  desctbl.c */
void init_gdt_idt();
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT          0x0082
#define AR_INTGATE32	0x008e
#define AR_TSS32        0x0089

/* int.c */
void init_pic(void);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/*  fifo8.c  */
struct FIFO8{
    unsigned char *buf;
    int p, q, size, free, flags;
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);

/*  fifo32.c  */
struct FIFO32{
    int *buf;
    int p, q, size, free, flags;
    struct TASK *task;
};
struct FIFO32 *keyinfo;
struct FIFO32 *mouseinfo;
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int  data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* bootpack.c  */
#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60
#define KEYCMD_SENDTO_MOUSE     0xd4
#define KBC_MODE                0x47
#define MOUSECMD_ENABLE        0xf4
#define KEYCMD_LED              0xed
unsigned int mem_size;

/*  mouse.c */
struct MOUSE_DEC{
    unsigned char buf[3], phase;
    int x, y, btn;
};
int mouse_base;
void enable_mouse(struct FIFO32 *fifo, int data, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, int data);

/*  keyboard.c  */
int key_base;
void wait_KBC_sendready();
void init_keyboard(struct FIFO32 *fifo, int base);

/*  window.c  */
void make_window(unsigned char *buf, int width, int height, char* title, char ic_active);
void make_window_title(unsigned char *buf, int width, char *title, char is_active);
void key_window_on(struct SHEET *key_window);
void key_window_off(struct SHEET *key_window);


/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int x_size, unsigned char color, int x0, int y0, int x1, int y1);
void init_screen(unsigned char *vram, int width, int height);
void putfont8(unsigned char *vram, int width, int x, int y, unsigned char color, char *font);
void putstr8_ref(struct SHEET *sheet, int x, int y, int color, int bc_color, char *str, int len);
void putstr8(unsigned char *vram, int width, int x, int y, unsigned char color, unsigned char *str);
void init_mouse_cursor8(char *mouse, char bg_color);
void putblock8_8(unsigned char *vram, int vwidth, int pwidth, int pheight, int px0, int py0, char *buf, int bwidth);
void make_textbox8(struct SHEET *sheet, int x0, int y0, int width, int height, int color);
void drawline(struct SHEET *sheet, int x0, int y0, int x1, int y1, int color);
void change_window_title(struct SHEET *sheet, char action);

/*  timer.c  */
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define MAX_TIMER 500
#define TIMER_FLAG_ALLOC 1
#define TIMER_FLAG_USING 2

struct TIMER{
    struct TIMER *next_timer;
    unsigned int timeout;
    char flags, flags_auto_cancel;
    struct FIFO32 *fifo;
    unsigned int data;
};
struct TIMER_CTL{
    unsigned int count, next;
    struct TIMER *timer_lead;
    struct TIMER timer[MAX_TIMER];
};
struct TIMER_CTL timer_ctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned int data);
void timer_set(struct TIMER *timer, unsigned int timeout);
int timer_cancel(struct TIMER *timer);
void timer_all_cancel(struct FIFO32 *fifo);

/*  file.c  */
struct FILEINFO{
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, clustno;
    unsigned int size;
};
struct FILEHANDLE{
    char *buf;
    int size, pos;
};
void read_fat(int *fat, unsigned char *img);
void load_file(int clustno, int size, char *buf, int *fat, char *img);
char *load_file_from_tek(int clustno, int *psize, int *fat);
struct FILEINFO *file_search(char name[], struct FILEINFO *finfo, int max);


/* multitask.c */
#define MAX_TASKS 1000
#define MAX_TASKS_LV 100
#define MAX_LEVELS 10
#define TASK_GDT0 3

struct TSS32{                                                   // Task Status Segment / 104バイト
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;         // タスク設定内容
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;    // 32ビットレジスタ
    int es, cs, ss, ds, fs, gs;                                // 16ビットレジスタ
    int ldtr, iomap;                                            // タスク設定(IO)
};
struct TASK{
    int sel, flags;
    int level, priority;
    struct FIFO32 fifo;
    struct TSS32 tss;
    struct SEGMENT_DESCRIPTOR ldt[2];
    struct CONSOLE *console;
    int ds_base, stack_addr;
    struct FILEHANDLE *fhandle;
    char *command;
    unsigned char langmode, langbyte1;
};
struct TASK_LEVEL{
    int running, now;
    struct TASK *task_addr[MAX_TASKS_LV];
};
struct TASK_CTL{
    int now_level;
    char do_level_change;
    struct TASK_LEVEL level[MAX_LEVELS];
    struct TASK tasks[MAX_TASKS];
};
struct TASK_CTL *task_ctl;
struct TIMER *task_timer;
void task_b_main(struct SHEET *sheet_back);
void task_idle_main(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
struct TASK *task_now(void);
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_switch_sub(void);
void task_sleep(struct TASK *task);

/*  console.c */
struct CONSOLE{
    struct SHEET *sheet;
    int cursor_x, cursor_y, cursor_color;
    struct TIMER *cursor_timer;
};

void task_console_main(struct SHEET *sheet);
void console_putchar(struct CONSOLE *console, int char_id, char move);
void console_putstr(struct CONSOLE *console, char *str);
void console_putstr_with_size(struct CONSOLE *console, char *str, int size);
struct TASK *open_console_task(struct SHEET * sheet);
struct SHEET *open_console(struct SHEET_CTL *sheet_ctl, unsigned int mem_size);
void kill_console_task(struct TASK *task);
void close_console(struct SHEET *sheet);

/*  api.c  */
int *haribote_api(int edi, int esi, int edp, int esp, int ebx, int edx, int ecx, int eax);

/*  inthandler.c  */
int *inthandler00(int *esp);
int *inthandler0c(int *esp);
int *inthandler0d(int *esp);
void inthandler20(int *esp);
void inthandler21(int *esp);
void inthandler2C(int *esp);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);
