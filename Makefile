grub-iso:
	mkdir -p build
	dd if=bootloader/grub-0.97-binaries/stage1 of=build/grub-floppy.img bs=512 count=1
	dd if=bootloader/grub-0.97-binaries/stage2 of=build/grub-floppy.img bs=512 seek=1

kernel.elf:
	# compile kernel objects
	nasm -felf32 kernel-32-bit/multiboot_header.asm -o build/multiboot_header.o
	nasm -felf32 kernel-32-bit/multiboot_entry.asm -o build/multiboot_entry.o
	clang -target i386-linux-gnu -ffreestanding -Wall -Wextra -c kernel-32-bit/kernel.c -o build/basic_kernel.o
	# link the kernel together
	#i386-unknown-linux-gnu-ld -T 32-bit-kernel/kernel.ld -o build/kernel.elf build/multiboot_header.o build/multiboot_entry.o build/basic_kernel.o
	ld -m elf_i386 -T kernel-32-bit/kernel.ld -o build/kernel.elf build/multiboot_header.o build/multiboot_entry.o build/basic_kernel.o

kernel.iso: kernel.elf
	# pack iso file
	mkdir -p build/isofiles/boot/grub
	cp bootloader/grub-0.97-binaries/iso9660_stage1_5 build/isofiles/boot/grub/stage1
	cp bootloader/grub-0.97-binaries/stage2 build/isofiles/boot/grub/stage2
	cp build/kernel.elf build/isofiles/boot/kernel.elf
	cp bootloader/menu.lst build/isofiles/boot/grub/
	cp bootloader/data_file build/isofiles/boot/
	xorriso -outdev build/kernel.iso -blank as_needed -map build/isofiles / -boot_image grub bin_path=/boot/grub/stage1

qemu: kernel.elf
	qemu-system-i386 -kernel build/kernel.elf
	#qemu-system-i386 -boot d -cdrom build/kernel.iso

grub-qemu: kernel.iso
	qemu-system-i386 -cdrom build/kernel.iso
