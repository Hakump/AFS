# AFesque
Distributed file system following AFS like protocol

Dependencies - Thrift and Fuse
See tool_setup.txt for install instructions for thrift, fuse

# Build
from /AFesque
make fuse
make server
optional - make main -  Main is pseudo shell that can be used to test calls outside of fuse


# Mount Fuse
./AFesque/fuseClient -f -d /users/jmadrek/p2/mnt
./AFesque/fuseClient -f -d /home/devbox/Code/CS739/P2/mnt

# Unmount fuse - ctrl C if you use -f -d, below if you run into issue or don't use flags
sudo umount -f home/devbox/Code/CS739/P2/mnt
or
fusermount -u /home/devbox/Code/CS739/P2/mnt

# Server Flags
-path <file you want to server from>
-chunk <chunk size>, otherwise loads without chunks and can't xfer large messages
-con --> Runs server with threaded pool for some concurrency

# Fuse Flags
-ip <server IP>, otherwise runs as localhost
-path <cache path> --> Where you are putting cache files
-dur --> Writes occur to tmp file instead of cache file, better consistency, some performance loss
