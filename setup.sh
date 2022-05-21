# Build Files
mkdir ../tmp
mkdir ../tmp/afs
mkdir ../tmp/cache
mkdir ../srvfs


# Update dependancies
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install  git make automake flex bison pkg-config
sudo apt-get install libboost-dev libboost-all-dev libboost-test-dev libboost-program-options-dev libboost-filesystem-dev libboost-thread-dev libevent-dev libtool  libssl-dev 
sudo apt-get install libblkid-dev e2fslibs-dev  libaudit-dev
sudo apt install meson cmake fuse3 libfuse3-dev libglib2.0-dev

# Install Thrift
cd ../..
mkdir tars
curl https://dlcdn.apache.org/thrift/0.15.0/thrift-0.15.0.tar.gz -o thrift.tar.gz
tar -xvf thrift.tar.gz

# Configure and make
cd thrift-0.15.0/
./bootstrap.sh
./configure --with-boost #=/usr
sudo make
# OK if check fails some tests
# sudo make check
sudo make install

# Verify thrift
sudo ldconfig
thrift -version

