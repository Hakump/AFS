#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <time.h>
#include <dirent.h>

using namespace std;

int main(){
    struct stat* mstat;
    struct timespec* ts;

    cout << "Size of dev_t = "  << sizeof(mstat->st_dev) << endl;
    cout << "Size of ino_t = "  << sizeof(mstat->st_ino) << endl;
    cout << "Size of mode_t = "  << sizeof(mstat->st_mode) << endl;
    cout << "Size of nlink_t = "  << sizeof(mstat->st_nlink) << endl;
    cout << "Size of gid_t = "  << sizeof(mstat->st_gid) << endl;
    cout << "Size of dev_t = "  << sizeof(mstat->st_dev) << endl;
    cout << "Size of off_t = "  << sizeof(mstat->st_size) << endl;
    cout << "Size of blksize_t = "  << sizeof(mstat->st_blksize) << endl;
    cout << "Size of blkcnt_t = "  << sizeof(mstat->st_blocks) << endl;
    cout << "Size of timespec = "  << sizeof(ts->tv_sec) << endl;
    cout << "Size of timespec = "  << sizeof(ts->tv_nsec) << endl;
    cout << "Size of st_ctim = "  << sizeof(mstat->st_ctim) << endl;

    struct dirent ment;

    cout << "Size of d_ino = "  << sizeof(ment.d_ino) << endl;
    cout << "Size of d_off = "  << sizeof(ment.d_off) << endl;
    cout << "Size of d_reclen = "  << sizeof(ment.d_reclen) << endl;
    cout << "Size of d_type = "  << sizeof(ment.d_type) << endl;
    cout << "Size of d_name = "  << sizeof(ment.d_name) << endl;

}