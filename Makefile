obj-m := s2fs.o

all:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	sudo make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rm -r mnt
test:
	sudo mkdir mnt
	sudo insmod s2fs.ko
	sudo mount -t s2fs nodev mnt
	sudo mount
	sudo cat mnt/0/1/1
	#sudo umount ./mnt
	#sudo mount
	#sudo rmmod s2fs
	#sudo dmesg
