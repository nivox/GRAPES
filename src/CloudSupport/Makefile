ifndef BASE
BASE = ../..
else
vpath %.c $(BASE)/src/$(notdir $(CURDIR))
endif
CFGDIR ?= ..

UTILS_DIR = ../Utils

OBJS = cloud_helper.o cloud_helper_utils.o cloud_helper_delegate.o
DELEGATE_HELPERS_DEPS = ../Utils/request_handler.o \
			../Utils/fifo_queue.o \
			../config.o \
			../net_helper$(NH_INCARNATION).o

ifeq ($(PLATFORM), darwin)
DELEGATE_HELPERS = libs3_delegate_helper.dylib mysql_delegate_helper.dylib
else
DELEGATE_HELPERS = libs3_delegate_helper.so mysql_delegate_helper.so
endif

CFLAGS += -I$(UTILS_DIR)

all: $(OBJS)

include $(BASE)/src/utils.mak

delegate_helpers: clean-root $(DELEGATE_HELPERS)

libs3_delegate_helper.so: LDFLAGS += -pthread -shared
libs3_delegate_helper.so: LDFLAGS += -ls3
libs3_delegate_helper.so: CFLAGS += -fPIC
libs3_delegate_helper.so: libs3_delegate_helper.o $(DELEGATE_HELPERS_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

libs3_delegate_helper.dylib: LDFLAGS += -pthread -dynamiclib
libs3_delegate_helper.dylib: LDFLAGS += -ls3
libs3_delegate_helper.dylib: CFLAGS += -fPIC
libs3_delegate_helper.dylib: libs3_delegate_helper.o $(DELEGATE_HELPERS_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)



mysql_delegate_helper.so: LDFLAGS += -pthread -shared
mysql_delegate_helper.so: LDFLAGS += -lmysql
mysql_delegate_helper.so: CFLAGS += -fPIC
mysql_delegate_helper.so: mysql_delegate_helper.o $(DELEGATE_HELPERS_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

mysql_delegate_helper.dylib: LDFLAGS += -pthread -dynamiclib
mysql_delegate_helper.dylib: LDFLAGS += -lmysql
mysql_delegate_helper.dylib: CFLAGS += -fPIC
mysql_delegate_helper.dylib: mysql_delegate_helper.o $(DELEGATE_HELPERS_DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean-root
clean-root:
	$(MAKE) -C .. clean

clean::
	rm -f *.so
