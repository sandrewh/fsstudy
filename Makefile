all :
	make -C gpt/
	make -C mbr/
	make -C ext2/
	make -C vfat/
	make -C ntfs/
	
clean :
	make -i -C gpt/ clean
	make -i -C mbr/ clean
	make -i -C ext2/ clean
	make -i -C vfat/ clean
	make -i -C ntfs/ clean
