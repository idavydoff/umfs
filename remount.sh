rm ./umfs 2>/dev/null
gcc -Wall -g -fsanitize=address  utils.c users.c groups.c umfs.c `pkg-config fuse3 --cflags --libs` `pkg-config --cflags --libs glib-2.0` -o umfs
mkdir test-fs 2>/dev/null
umount ./test-fs 2>/dev/null
echo "Running..."
./umfs ./test-fs