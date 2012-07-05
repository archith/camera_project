include $(PROJ_ROOT)/src/sysapps/apps/hydra/Defs.mak
PWD	= $(shell pwd)
LIB	= libevent.a
EVENT_DIR	= $(HYDRA_DIR)/event
INC_DIR	= $(EVENT_DIR)/inc
OBJ_DIR	= $(EVENT_DIR)/obj
CFLAGS	+= -I$(INC_DIR)

HDRS	= $(patsubst %.h, $(INC_DIR)/%.h, $(filter-out $(UNEXPORTED_HDRS), $(wildcard *.h)))
SRCS	= $(filter-out $(UNEXPORTED_SRCS), $(wildcard *.c))
OBJS	= $(SRCS:.c=.o)

#$(process-subdir)	// process sub-directories which should not be skipped
define process-subdir
	@for i in $(SUBDIR); do \
		$(MAKE) -C $$i $@; \
		ret=$$?; \
		echo "!!return form $$i is $$ret"; \
		if [ $$ret -eq 100 ]; \
		then \
			if [ "$@" = "gpl-clean" ]; \
			then \
				echo "going to remove $$i"; \
			fi; \
		else \
			true; \
		fi;\
	done	
endef
#$(skip-dir)	// skip this dir is SKIP==yes
define skip-dir
	@if [ "$(SKIP)" = "yes" ]; \
	then \
		echo "!!$(notdir $(PWD)) must be skipped"; \
		exit 100; \
	fi
endef

ifeq	($(LIBRARY_ROOT),yes)
SUBDIR	= $(filter-out $(notdir $(INC_DIR) $(OBJ_DIR)), $(shell ls -p -1| grep / | sed 's/\///'))
all:	$(INC_DIR) $(OBJ_DIR) h $(OBJS)
	$(skip-dir)
	$(process-subdir)
	$(AR) ro $(LIB) $(OBJ_DIR)/*.o

$(INC_DIR) $(OBJ_DIR):
	mkdir -p $@
else
SUBDIR	= $(shell ls -p -1| grep / | sed 's/\///')
all:	h $(OBJS)
	$(skip-dir)
	$(process-subdir)
endif

$(HDRS):
	if [ ! -d $(INC_DIR) ]; \
	then \
		mkdir $(INC_DIR); \
	fi
	ln -sf $(PWD)/$(@F) $(INC_DIR)/$(@F)

h:	$(HDRS)
	$(skip-dir)
	$(process-subdir)

%.o: %.c
	if [ ! -d $(OBJ_DIR) ]; \
	then \
		mkdir $(OBJ_DIR); \
	fi
	$(CC) -c $(CFLAGS) $< -o $@
	install -D $@ $(OBJ_DIR)/$@

clean:
	$(skip-dir)
	$(process-subdir)
	@rm -f $(OBJS)
ifeq	($(LIBRARY_ROOT),yes)
	@rm -rf $(INC_DIR) $(OBJ_DIR)
endif

gpl-clean:
	$(skip-dir)
	$(process-subdir)
	rm -f $(SRC) *.c



