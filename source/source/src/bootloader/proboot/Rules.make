#
# Common rules
#

%.o: %.S
#	@echo -e "\n== $@ =="
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
#	@echo -e "\n== $@ =="
	$(CC) $(CFLAGS) -c $< -o $@

#
# Rule to compile a set of .o files into one .o file
#

ifdef TARGET
$(TARGET): $(OBJS)
#	@echo -e "\n== $@ =="
	$(LD) -r -o $@ $(OBJS)
endif

#
# include dependency files if they exist
#

depend:
ifneq ($(SRCS),)
ifeq ($(wildcard .depend),)
	@$(CC) -M $(CFLAGS) $(SRCS) > .depend
endif
endif

ifneq ($(wildcard .depend),)
include .depend
endif

#
# A rule to do nothing
#

dummy:
