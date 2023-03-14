
ifdef C3_SANITIZE
	MODULE_CFLAGS += -fsanitize=address
	EXTRA_OBJ_FILES += /usr/lib/gcc/x86_64-linux-gnu/9/libasan.a

	MODULE_CFLAGS += -fsanitize=undefined
	EXTRA_OBJ_FILES += /usr/lib/gcc/x86_64-linux-gnu/9/libubsan.a
endif

XED_INSTALL_PATH = $(project_dir)/lib/xed/kits/xed
XED_LIB = $(XED_INSTALL_PATH)/lib/libxed.a
XED_CFLAGS = -I$(XED_INSTALL_PATH)/include/xed -DUSE_XED=1
