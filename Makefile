CC=gcc
CFLAGS=-Wall -g -fsanitize=address `pkg-config fuse3 --cflags --libs` `pkg-config --cflags --libs glib-2.0`

SRC=
SRC += umfs.c
SRC += utils.c
SRC += users.c
SRC += groups.c
SRC += ./operations/read.c
SRC += ./operations/readlink.c
SRC += ./operations/readdir.c
SRC += ./operations/open.c
SRC += ./operations/opendir.c
SRC += ./operations/getattr.c
SRC += ./operations/init.c 
SRC += ./operations/mkdir.c 
SRC += ./operations/rename.c 

umfs: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@