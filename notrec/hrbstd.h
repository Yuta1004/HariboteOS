#ifndef API_LINK_HEAD
#define API_LINK_HEAD

/* api */
void api_putchar(int c_code);
void api_putstr(char *s);
int api_gen_window(char *buf, int width, int height, int back_color, char *title);
void api_putstr_window(int win_id, int x, int y, int color, int str_len, char *str);
void api_boxfill_window(int win_id, int x0, int y0, int x1, int y1, int color);
void api_end(void);
void api_init_memman(void);
char *api_malloc(int size);
void api_mfree(char *addr, int size);
void api_point_window(int win, int x, int y, int color);
void api_refresh_window(int win, int x0, int y0, int x1, int y1);
void api_drawline_window(int win, int x0, int y0, int x1, int y1, int color);
void api_close_window(int win);
int api_keyinput(int mode);
int api_alloc_timer();
void api_init_timer(int timer, int data);
void api_set_timer(int timer, int time);
void api_free_timer(int timer);
void api_beep(int tone);
int api_fopen(char *name);
void api_fclose(int fhandle);
void api_fseek(int fhandle, int offset, int mode);
int api_fsize(int fhandle, int mode);
int api_fread(int fhandle, char *buf, int size);
int api_get_command(char *buf, int maxsize);
int api_check_langmode();

/* dec_hex_c */
int dec2asc(char *str, int dec);
int hex2asc(char *str, int dec);

/* msprintf.c */
void sprintf(char *str, char *fmt, ...);

/* printf.c */
void printf(char *fmt, ...);

/* str.c */
char strncmp(char str_a[], char str_b[], int size);
char strcmp(char str_a[], char str_b[]);
int strlen(char *str);
long strtol(char *nptr, char **endptr, int base);

/* math.c */
int min(int comp_a, int comp_b);
int max(int comp_a, int comp_b);

/* malloc.c */
void *malloc(int size);

/* mfree.c */
void mfree(void *p);

/* exit.c */
void exit();

#endif