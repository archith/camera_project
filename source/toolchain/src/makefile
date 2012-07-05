PWD=$(shell pwd)
BIN_DIR=$(subst /,\/,$(PWD))
all:
	#build toolchain
	tar xfj buildroot-0.9.27.tar.bz2
	cd buildroot; patch -p1 < ../buildroot_pl1029.patch
	cd buildroot; ln -s ../../pkg/ dl
	cd buildroot;cp config.arm.pl1029 ./.config;
	sed 's/\/opt\/prolific/$(BIN_DIR)/' buildroot/.config > buildroot/.config_tmp
	cp -f buildroot/.config_tmp buildroot/.config
	rm -f buildroot/.config_tmp
	make -C buildroot oldconfig
	make -C buildroot
	rm -rf ../uc_arm
	ln -s src/uc_arm ..
	
	#squashfs
	tar xvzf squashfs2.1.tar.gz
	make -C squashfs2.1/squashfs-tools
	rm -rf $(PROJ_ROOT)/host/bin
	install -D squashfs2.1/squashfs-tools/mksquashfs $(PROJ_ROOT)/host/bin/mksquashfs
	
clean:
	rm -rf buildroot
