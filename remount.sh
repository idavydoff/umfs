rm ./umfs 2>/dev/null
make > /dev/null
mkdir test-fs 2>/dev/null
umount ./test-fs 2>/dev/null
echo "Running..."
./umfs ./test-fs -o allow_other -o default_permissions -o gid=10 -f