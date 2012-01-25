fsstudy: Simple drivers for vfat, ext2, mbr, gpt
==

# Status

* vfat: read/write support
* ext2/3: read support
* hfsp: not functional
* ntfs: not functional

# Examples

Use mbr to determine a disk's partitions:

	./mbr /dev/disk0

example usage of vfat driver: `vfat/main.c`

	./vfat -o [lba partition offset] -f [file/disk path] [cmd] [cmd parameters]
	
To list the root directory contents

	./vfat -f /dev/disk0 -o 0x3f ls /
	
To output a file's contents to stout

	./vfat -f /dev/disk0 -o 0x3f read /test.txt
	
To write to a file

	./vfat -f /dev/disk0 -o 0x3f write /dir/test.out < test.in

example usage of ext2 driver: `ext2/main.c`

