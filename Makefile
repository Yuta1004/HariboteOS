DISK_PATH = /dev/disk3
AS_BIN = nasm -f bin
AS_ELF = nasm -f elf
CC = i386-elf-gcc
CC_APP = i386-elf-gcc -m32 -march=i486 -nostdlib -fno-builtin -T har.ld
QEMU = qemu-system-i386 -m 32 -vga std -fda


haribote.img :
	make -C haribote
	make build_app
	make cp_build_result
	make install

build_app :
	make -C timer
	make -C colorwin
	make -C walkgame
	make -C primenum
	make -C cat
	make -C iroha
	make -C chklang
	make -C notrec
	make -C bball
	make -C invador
	make -C calc
	make -C tviewer
	make -C mmlplay
	make -C imgview

install :
	hdid build_result/haribote.img
	cp build_result/haribote.sys	/volumes/HARIBOTEOS
	cp build_result/*.hrb 			/volumes/HARIBOTEOS
	cp japanese_font/jpn_tek.fnt 	/Volumes/HARIBOTEOS
	cp mmlfiles/*.org				/Volumes/HARIBOTEOS
	cp images/*						/Volumes/HARIBOTEOS
	diskutil umount ${DISK_PATH}
	diskutil eject ${DISK_PATH}

cp_build_result:
	cp haribote/haribote.img 	build_result
	cp haribote/haribote.sys 	build_result
	cp timer/timer.hrb 			build_result
	cp colorwin/colorwin.hrb 	build_result
	cp walkgame/walkgame.hrb	build_result
	cp primenum/primenum.hrb	build_result
	cp cat/cat.hrb				build_result
	cp iroha/iroha.hrb			build_result
	cp chklang/chklang.hrb		build_result
	cp notrec/notrec.hrb		build_result
	cp bball/bball.hrb			build_result
	cp invador/invador.hrb		build_result
	cp calc/calc.hrb			build_result
	cp tviewer/tviewer.hrb		build_result
	cp mmlplay/mmlplay.hrb 		build_result
	cp imgview/imgview.hrb		build_result

write :
	diskutil unMountDisk ${DISK_PATH}
	sudo dd if=./build_result/haribote.img of=${DISK_PATH}
	diskutil eject ${DISK_PATH}

run :
	make haribote.img
	make qemu_run

qemu_run :
	${QEMU} build_result/haribote.img
