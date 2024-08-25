CC=gcc
CFLAGS=-Wall -g -fsanitize=address `pkg-config fuse3 --cflags --libs` `pkg-config --cflags --libs glib-2.0`

SRC=
SRC += ./src/umfs.c
SRC += ./src/utils.c
SRC += ./src/users.c
SRC += ./src/groups.c
SRC += ./src/operations/read.c
SRC += ./src/operations/readlink.c
SRC += ./src/operations/readdir.c
SRC += ./src/operations/open.c
SRC += ./src/operations/opendir.c
SRC += ./src/operations/getattr.c
SRC += ./src/operations/init.c 
SRC += ./src/operations/mkdir.c 
SRC += ./src/operations/rename.c 
SRC += ./src/operations/rmdir.c 
SRC += ./src/operations/write.c 

umfs: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@