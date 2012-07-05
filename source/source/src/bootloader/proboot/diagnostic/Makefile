AFLAGS		:= -x assembler-with-cpp -D__ASSEMBLY__

#march.o: march.S
%.o: %.S
#	@echo -e "\n== $@ =="
	#$(CC) -I$(TOPDIR)/include -x assembler-with-cpp -D__ASSEMBLY__ -march=armv4 -EL -fpic -c -o march.out march.S
	$(CC) -I$(TOPDIR)/include -x assembler-with-cpp -nostartfiles -fpic -shared march.S -o march.out
	#$(CC) -I$(TOPDIR)/include -x assembler-with-cpp -mapcs-32 -c march.S -o march.out
	#arm-linux-as -EL -march=armv4 -o march.out march.S
	$(OBJCOPY) -O binary -j .text march.out march.bin
 	## $(OBJCOPY) -I binary -O $(OFORMAT) march.bin march.o
	$(LD) -r -o march.o -b binary march.bin
	#$(STRIP) march.o 2> /dev/null

march.bin: march.o
