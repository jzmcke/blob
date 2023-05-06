CC = gcc
BLOB_ROOT = /home/jzmcke/blob
OBJS += $(BLOB_ROOT)/src/blob_jbuf_frag.o
OBJS += $(BLOB_ROOT)/src/packet.o
OBJS += $(BLOB_ROOT)/test/test_blob_jbuf/test_blob_jbuf.o
INC = -I$(BLOB_ROOT)/include -I$(BLOB_ROOT)/src

DEBUG_OBJS = ${OBJS:.o=_debug.o}

CFLAGS = -Wall -std=gnu99

CFLAGS_RELEASE = ${CFLAGS}
CFLAGS_RELEASE += -O3

CFLAGS_DEBUG = ${CFLAGS}
CFLAGS_DEBUG += -g

%.o: %.c
	$(CC) $(CFLAGS_RELEASE) -c -DBLOB_SOCKET=1 $< -o $@ $(INC)

%_debug.o: %.c
	$(CC) $(CFLAGS_DEBUG) -c -DBLOB_SOCKET=1 $< -o $@ $(INC)

debug: $(DEBUG_OBJS)
	mkdir -p bin
	$(CC) -g -DBLOB_SOCKET=1 -o bin/test_blob_jbuf_debug $^ $(INC) $(LD_DIRS) $(LD_LIBS)

release: $(OBJS)
	mkdir -p bin
	$(CC) -DBLOB_SOCKET=1 -o bin/test_blob_jbuf_release $^ $(INC) $(LD_DIRS) $(LD_LIBS)

clean :
	rm -f $(OBJS)
	rm -f $(DEBUG_OBJS)
	rm -rf bin
	
