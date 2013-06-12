
gcc ~/exos-tools/davinci_sdcard_boot/main.c -o ~/exos-tools/davinci_sdcard_boot/davinci_sdcard_boot.elf

~/exos-tools/davinci_sdcard_boot/davinci_sdcard_boot.elf "ARM Debug/ubl.bin" "ARM Debug/ubl.img" 512 32

read -p "This script will dump to block 0 of /dev/disk1. All data in /dev/disk1 will be lost. Continue? (y/n)" yn

read -p "Really really sure? Btw, this script requires su permission (y/n)" yn

case $yn in
	[Yy]* ) dd if="ARM Debug/ubl.img" of=/dev/disk1; diskutil eject /dev/disk1;;
	[Nn]* ) exit;;
esac


