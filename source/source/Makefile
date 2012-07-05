# SerComm Network Camera top level Makefile
TOPDIR  = ${shell pwd}

ifndef PROJ_ROOT
$(error You must first source the BSP environment: "source setenv")
endif

INSTALL_DIRS = src/bootloader src/kernel/linux src/kmods src/sysapps src/rootfs
CLEAN_DIRS   = src/bootloader src/kernel/linux src/kmods src/sysapps src/rootfs FW

# Mark off the all
all: toolchain prebuild defconfig bootloader linux base kmods sysapps rootfs FW

# Start of configuration targets

.PHONY: config menuconfig oldconfig silentoldconfig defconfig toolchain

prebuild:
	mkdir -p img/fs
	mkdir -p host/bin
	make -C host/open_source/squashfs-tools

config menuconfig oldconfig silentoldconfig %_defconfig:
# Configure Ingenient BSP
	@$(MAKE) -C src/sysapps/busybox-1.01 $@

# Configure Linux Kernel
	@host/scripts/kernel_config $@
#	@$(MAKE) -C src/kernel/linux $@


$(PROJ_DEVEL_ROOT)/bin/arm-linux-gcc:
	make -C $(PROJ_ROOT)/../toolchain/src

toolchain: $(PROJ_DEVEL_ROOT)/bin/arm-linux-gcc	

defconfig: src/sysapps/busybox-1.01/.config src/kernel/linux-2.4.19/.config


src/sysapps/busybox-1.01/.config:
	@$(MAKE) -C src/sysapps/busybox-1.01 defconfig
	
src/kernel/linux-2.4.19/.config:
	@host/scripts/def_config $@
	@$(MAKE) -C src/kernel/linux defconfig

# End of configuration targets

# Start main top level targets

.PHONY: bootloader linux sysapps kmods rootfs FW

bootloader:
	@$(MAKE) -C src/$@

linux:
	@(if [ ! -h ${PROJ_ROOT}/src/kernel/linux ]; then \
		cd ${PROJ_ROOT}/src/kernel; \
		ln -sf linux-2.4.19 linux;  \
		cd -; \
	fi)
	@$(MAKE) -C src/kernel/linux zinstall

base: 
	@$(MAKE) -C src/sysapps/busybox-1.01 install

kmods:
	mkdir -p $(PROJ_ROOT)/lib/modules/2.4.19-pl1029
	$(MAKE) -C src/kmods all || exit 1

sysapps:
	@$(MAKE) -C src/sysapps/apps || exit 1

rootfs: 
	@$(MAKE) -C src/$@ || exit 1


install:
	@for i in `echo $(INSTALL_DIRS)`; do \
		$(MAKE) -C $$i $@; \
	done

# End main top level targets

# Start Top level clean-up targets

.PHONY: clean
clean:
	@for i in `echo $(CLEAN_DIRS)`; do \
		$(MAKE) -C $$i $@; \
	done
	@rm -rf $(PROJ_ROOT)/img/fs/*


# End top level clean-up targets

# Start Extra Top level Make targets

linux-%:
	@$(MAKE) -C kernels/$@ uImage


FW:
	@$(MAKE) -C FW || exit 1

# End extra top level make targets
