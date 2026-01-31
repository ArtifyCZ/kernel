grub-iso:
	mkdir -p build
	dd if=bootloader/grub-0.97-binaries/stage1 of=build/grub-floppy.img bs=512 count=1
	dd if=bootloader/grub-0.97-binaries/stage2 of=build/grub-floppy.img bs=512 seek=1

kernel.iso:
	mkdir -p build/isofiles/boot/grub
	cp bootloader/grub-0.97-binaries/iso9660_stage1_5 build/isofiles/boot/grub/stage1
	cp bootloader/grub-0.97-binaries/stage2 build/isofiles/boot/grub/stage2
	xorriso -outdev build/kernel.iso -blank as_needed -map build/isofiles / -boot_image grub bin_path=/boot/grub/stage1

qemu:
	qemu-system-x86_64 -boot d -cdrom build/kernel.iso

grub-qemu: grub-iso
	qemu-system-x86_64 -drive file=build/grub-floppy.img,index=0,if=floppy,format=raw
