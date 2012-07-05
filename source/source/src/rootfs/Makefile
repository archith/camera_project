
# Makefile to build a cram file-system image.
#
# Gary Lin, Prolific.
#

ROOT_DIR=$(PROJ_ROOT)/img/fs/root

CRAMFS          = $(PROJ_ROOT)/host/bin/mkcramfs
SQUASHFS        = $(PROJ_ROOT)/host/bin/mksquashfs


all: banner check cram-image install
	@echo [Success] all jobs done 

banner:
	@echo "*** build CRAM root filesystem 'cram-image' ***"
	@echo 1. Should build busybox and install it to shadow_root directory
	@echo 2. Edit files under etc directory
	@echo 3. Require root privilege 
	@echo 4. Require CRAM file-system utilitis
	@echo "***"

check:
#	@[ "`whoami`" == "root" ] || (echo "[error] require root privilege"; exit 1)
	@[ -d ./etc ] || (echo "[error] require a etc directory in current diretory"; exit 3)
	@[ "$(PROJ_ROOT)" != "" ] || (echo "[error] should run 'setenv' to enter pl1029 project"; exit 4)
	@[ -d $(PROJ_ROOT)/img/fs/shadow_root ] || (echo "[error] should build busybox and install it to shadow before make rootfs.gz"; exit 5)
#	@[ -f $(CRAMFS) ] || (echo "please install cramfs utilities first" ; exit 6)
	@[ -f $(SQUASHFS) ] || (echo "please install squashfs utilities first" ; exit 6)

cram-image: _rootfs
	@rm -f fs.img
#	@$(CRAMFS)   $(ROOT_DIR) fs.img
	@$(SQUASHFS) $(ROOT_DIR) fs.img
	@echo "root filesystem is generated..."
	@ls -l fs.img
	@chmod 666 fs.img

_rootfs:
	@rm -rf $(ROOT_DIR)
	install -m 755 -d $(ROOT_DIR)
	cp -a etc $(ROOT_DIR)
#	cp -a misc $(ROOT_DIR)
#	chown -R root.root $(ROOT_DIR)/etc
	cp -a $(PROJ_ROOT)/img/fs/shadow_root/* $(ROOT_DIR)
	cp -a $(PROJ_ROOT)/lib                  $(ROOT_DIR)
	ln -sf  /mnt/ramdisk/system.conf $(ROOT_DIR)/etc/system.conf
	ln -sf  /mnt/ramdisk/resolv.conf $(ROOT_DIR)/etc/resolv.conf
	ln -sf  /mnt/ramdisk/wpa_supplicant.conf $(ROOT_DIR)/etc/wpa_supplicant.conf
##	ln -sf  /mnt/ramdisk/ld.so.conf  $(ROOT_DIR)/etc/ld.so.conf
##	ln -sf  /mnt/ramdisk/ld.so.cache $(ROOT_DIR)/etc/ld.so.cache
	find $(ROOT_DIR) -name CVS | xargs rm -rf
	find $(ROOT_DIR) -name Thumbs.db -exec rm -rf '{}' ';'
	find $(ROOT_DIR) -iname license*.txt -exec rm -rf '{}' ';'
	find $(ROOT_DIR) -iname license*.doc -exec rm -rf '{}' ';'
	install -m 755 -d $(ROOT_DIR)/dev
	cd $(ROOT_DIR)/dev;ln -sf /tmp/console .
#	(cd $(ROOT_DIR)/dev; mknod -m 0600 console c 5 1;);
#	(cd $(ROOT_DIR)/dev ; \
#		mknod -m 0600 console c   5   1 ; \
#		mknod         ptya0   c   2 176 ; \
#		mknod         ptya1   c   2 177 ; \
#		mknod         ptya2   c   2 176 ; \
#		mknod         ttya0   c   3 176 ; \
#		mknod         ttya1   c   3 177 ; \
#		mknod         ttya2   c   3 178 ; \
#		mknod         tty0    c   4   0 ; \
#		mknod         tty1    c   4   1 ; \
#		mknod         tty2    c   4   2 ; \
#		mknod         tty3    c   4   3 ; \
#		mknod         tty4    c   4   4 ; \
#		mknod         tty5    c   4   5 ; \
#		mknod         tty6    c   4   6 ; \
#		mknod         tty7    c   4   7 ; \
#		mknod         ttyS0   c   4  64 ; \
#		mknod         ttyS1   c   4  65 ; \
#	);
	install -m 755 -d $(ROOT_DIR)/proc
	install -m 755 -d $(ROOT_DIR)/mnt
	install -m 755 -d $(ROOT_DIR)/mnt/ramdisk
	ln -sf  /mnt/ramdisk/tmp     $(ROOT_DIR)/tmp
	ln -sf  /mnt/ramdisk/var     $(ROOT_DIR)/var
	ln -sf  /mnt/ramdisk/root    $(ROOT_DIR)/root
##	install -m 755 -d $(ROOT_DIR)/tmp
##	install -m 755 -d $(ROOT_DIR)/var
##	 ( cd $(ROOT_DIR)/var ; \
##	    ln -s /tmp lock ; \
##	    ln -s /tmp log ; \
##	    ln -s /tmp run ; \
##	    ln -s /tmp tmp ; \
##	) ; 
# I would put shared libraries copying here for now
	install -m 755 -d $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/ld-uClibc-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/ld-uClibc.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libuClibc-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libc.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libfloat.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libfloat.so.1 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libdl.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libgcc_s.so $(ROOT_DIR)/lib
# the above three libraries are almost mandatory.
	cp -a $(PROJ_DEVEL_ROOT)/lib/libpthread-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libpthread.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libz.so.1.2.3 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libz.so.1 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libz.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libm-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libm.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libthread_db-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libthread_db.so.1 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libcrypt-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libcrypt.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libdl-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libdl.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libnsl-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libnsl.so.0 $(ROOT_DIR)/lib
#	cp -a $(PROJ_DEVEL_ROOT)/lib/libresolv-0.9.28.so $(ROOT_DIR)/lib
#	cp -a $(PROJ_DEVEL_ROOT)/lib/libresolv.so.0 $(ROOT_DIR)/lib
#	cp -a $(PROJ_DEVEL_ROOT)/lib/librt-0.9.28.so $(ROOT_DIR)/lib
#	cp -a $(PROJ_DEVEL_ROOT)/lib/librt.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libutil-0.9.28.so $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libutil.so.0 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libgcc_s.so.1 $(ROOT_DIR)/lib
	cp -a $(PROJ_DEVEL_ROOT)/lib/libgcc_s.so $(ROOT_DIR)/lib
	find $(PROJ_ROOT)/img/fs/root -name .svn | xargs rm -rf
	du -h  $(ROOT_DIR)	
	
install: cram-image
	cp -f fs.img $(PROJ_ROOT)/img

clean:
	@rm -rf fs.img 

distclean:
	@echo "clean shadow root from $(ROOT_DIR)/fs"
	rm -rf fs.img $(ROOT_DIR)
	rm -f $(ROOT_DIR)/fs/shadow_root
