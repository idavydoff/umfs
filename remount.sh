rm ./umfs 2>/dev/null
make
mkdir test-fs 2>/dev/null
umount ./test-fs 2>/dev/null
echo "Running..."
./umfs ./test-fs -f