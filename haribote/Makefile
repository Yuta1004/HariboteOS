SOURCE_FILES = bootpack.c nasmfunc.o graphic.c desctbl.c int.c fifo32.c msprintf.c hankaku.c mouse.c keyboard.c memory.c sheet.c window.c timer.c multitask.c console.c functions.c file.c api.c inthandler.c tek.c
AS_BIN = nasm -f bin
AS_ELF = nasm -f elf
CC_APP = i386-elf-gcc -m32 -march=i486 -nostdlib -fno-builtin -T ../har.ld

# make haribote.img
img :
	make haribote.img
	make haribote.sys
	make clean

# .img
other_disk.img : other_disk.asm
	$(AS_BIN) -o other_disk.img other_disk.asm

haribote.img : other_disk.img ipl20.asm
	$(AS_BIN) -o haribote.img ipl20.asm
	cat other_disk.img >> haribote.img

# .o
nasmfunc.o : nasmfunc.asm
	$(AS_ELF) -o nasmfunc.o nasmfunc.asm

#.sys
haribote.sys : asmhead.asm har.ld ${SOURCE_FILES}
	$(AS_BIN) -o asmhead.bin asmhead.asm
	$(CC_APP) -Wl,-Map=bootpack.map -o bootpack.hrb ${SOURCE_FILES}
	cat asmhead.bin bootpack.hrb > haribote.sys

clean :
	rm *.o
	rm *.hrb
	rm *.bin
