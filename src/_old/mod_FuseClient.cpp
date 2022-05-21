#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <map>
#include "AFEsqClient.h"

using namespace std;

AFEsqClient* afesq;
std::string CACHE_PATH = "../tmp/afs";
std::string SERVER_ADDY = "localhost";

static int fill_dir_plus = 0;

static void *xmp_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}


int fuse_getattr(const char *path, struct stat *stbuf,struct fuse_file_info *fi)
{
	// int res;
	
	// // res = lstat(path, stbuf);
	// res = stat(path, stbuf);
	//  //afesq->stat(path, stbuf);
	// if (res == -2)
	//        return -2;
	// if (res == -1)
	//        return -errno;
	// return 0;
	(void) fi;
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}


int fuse_mkdir(const char *path, mode_t mode){
    return afesq->mkdir(path,mode);
}

int fuse_rmdir(const char *path){
    return afesq->rmdir(path);
}

int fuse_create(const char *path, mode_t mode,
		struct fuse_file_info *fi)
{
	int res;

	res = afesq->open(path, fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

int fuse_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = afesq->open(path, fi->flags);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

int fuse_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	if(fi == NULL)
		fd = afesq->open(path, O_RDONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}


int fuse_unlink(const char *path){
    return afesq->unlink(path);
}

int fuse_pread(int fd, void *buf, size_t count, off_t offset){
    int res = pread(fd, buf, count, offset);
    if (res == -1)
		res = -errno;
    return res;
}

int fuse_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi) {
	int fd;
	int res;

	(void) fi;
	fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

int fuse_pwrite(int fd, const void *buf, size_t count, off_t offset){
    int res = pwrite(fd, buf, count, offset);
    if (res == -1)
		res = -errno;
    return res;
}

int fuse_stat(const char *path, struct stat *buf){
    //return afesq->stat(path,buf);
	return stat(path, buf);
	//return 0;
}

int fuse_close(int fd) {
    return afesq->close(fd);
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	(void) flags;

	const char* true_path = "/users/jmadrek/p2/srvfs/";

	dp = opendir(true_path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
			break;
	}

	closedir(dp);
	return 0;
}

static struct fuse_operations fuse_oper = {
	.init           = xmp_init,
	.getattr	= fuse_getattr,	
	.mkdir		= fuse_mkdir,
	.rmdir		= fuse_rmdir,
	.open           = fuse_open,
//	.create	        = fuse_create,
	.read		= fuse_read,
	.write		= fuse_write,
	.readdir	= fuse_readdir,
	.create         = fuse_create
};


int main( int argc, char *argv[] ) {
	afesq = new AFEsqClient(CACHE_PATH, SERVER_ADDY);
	return fuse_main( argc, argv, &fuse_oper, NULL );
}
