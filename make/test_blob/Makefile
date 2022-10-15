CC = gcc 
OBJS = $(GIT_ROOT)/blob/src/blob_core.o
OBJS += $(GIT_ROOT)/blob/src/blob_node.o
OBJS += $(GIT_ROOT)/blob/src/blob_node_tree.o
OBJS += $(GIT_ROOT)/blob/src/blob_minws.o
OBJS += $(GIT_ROOT)/blob/src/blob_espws.o
OBJS += $(GIT_ROOT)/blob/src/blob.o
OBJS += $(GIT_ROOT)/blob/src/blob_file.o
OBJS += $(GIT_ROOT)/blob_support/frontend/test_blob.o
OBJS += $(GIT_ROOT)/minimal_websocket/src/minimal_websocket.o
INC = -I$(GIT_ROOT) -I/usr/local/opt/openssl/include

LD_DIRS=-L/usr/local/opt/openssl/lib -L/usr/local/lib
LD_LIBS=-lwebsockets -lssl -lcrypto

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
	$(CC) -g -DBLOB_SOCKET=1 -o bin/test_blob_debug $^ $(INC) $(LD_DIRS) $(LD_LIBS)

release: $(OBJS)
	mkdir -p bin
	$(CC) -DBLOB_SOCKET=1 -o bin/test_blob_release $^ $(INC) $(LD_DIRS) $(LD_LIBS)

clean :
	rm -f $(OBJS)
	rm -f $(DEBUG_OBJS)
	rm -rf bin
	
