/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file COPYING.
*/

/**
 Included above header as we used pasthrough.c as a model for our implementation
**/

/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#define FUSE_USE_VERSION 31
#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <list>
#include <sys/time.h>
#include <iostream>
#include "AFEsqClient.h"

AFEsqClient* afesq;
std::string CACHE_PATH = "/users/jmadrek/p2/tmp/afs";
std::string SERVER_ADDY = "localhost";

static fuse_fill_dir_flags fill_dir_plus;

static void *fuse_init(struct fuse_conn_info *conn,
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
	cfg->entry_timeout = 50;
	cfg->attr_timeout = 50;
	cfg->negative_timeout = 50;

	return NULL;
}

static int fuse_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;
	res = afesq->stat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

/**
 A little hacky, but it works for this project
**/
static int fuse_access(const char *path, int mask)
{
	int res;
	res = 0;
	if (res == -1)
		return -errno;

	return 0;
}

int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{ 
		
	// ##########################
	// AFESQ
	// ##########################	
	(void) offset;
	(void) fi;
	(void) flags;

	std::vector<tDirent> dent_list = afesq->afs_readdir(path);
	dent_list = afesq->afs_readdir(path);
	// while (!dent_list.empty())
	for (int i = 0; i < dent_list.size(); i++)
	{		
		char node_name[256];
		strcpy(node_name, dent_list[i].d_name.c_str());
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = dent_list[i].d_ino;
		st.st_mode = dent_list[i].d_type << 12;
		//fill_dir_plus or FUSE_FILL_DIR_PLUS
		if (filler(buf, node_name, &st, 0, FUSE_FILL_DIR_PLUS))
			break;
	}

	return 0;

	// ##########################
}

static int fuse_mkdir(const char *path, mode_t mode)
{
	int res;

	// res = mkdir(path, mode);
	res = afesq->mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int fuse_unlink(const char *path)
{
	int res;

	//res = unlink(path);
	res = afesq->unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int fuse_rmdir(const char *path)
{
	int res;

	res = afesq->rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int fuse_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	// This is kept in or else you have issues with touch

	return 0;
}


static int fuse_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi)
{
	int res;

	//res = open(path, fi->flags, mode);
	res = afesq->creat(path, fi->flags, mode);
	//res = afesq->open(path, fi->flags, mode); 
	if (res == -1){
		std::cout << "Create returned an error" << std::endl;
		return -errno;
	}
		

	fi->fh = res;
	return 0;
}


static int fuse_open(const char *path, struct fuse_file_info *fi)
{
	std::cout << "#### FUSE is opening " << path << std::endl;
	int fd = afesq->open(path, fi->flags, 0777); 
	if (fd < 0){
		return -errno;
	} 
	fi->fh = fd;
	return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	if(fi == NULL)
		fd = open(path, O_RDONLY);
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

static int fuse_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	
	(void) fi;

	int fd = fi->fh;
	int res;

	res = afesq->pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;
	return res;

}


// Need this to avoid errors on touch.
static int fuse_flush(const char *, struct fuse_file_info *){
	return 0;
}


static int fuse_release(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	int res;
	res = afesq->close(fi->fh);
	if (res == -1)
		return -errno;
	return 0;
}

static int fuse_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	// fsync dealt with in client write, this is just a stub

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}


static const struct fuse_operations xmp_oper = {
	.getattr	= fuse_getattr,
	.mkdir		= fuse_mkdir,
	.unlink		= fuse_unlink,
	.rmdir		= fuse_rmdir,
	.open		= fuse_open,
	.read		= fuse_read,
	.write		= fuse_write,	
	.flush      = fuse_flush,
	.release	= fuse_release,
	.fsync		= fuse_fsync,
	.readdir	= fuse_readdir,
	.init       = fuse_init,
	.access		= fuse_access,
  	.create 	= fuse_create,
	.utimens	= fuse_utimens,
};

int main(int argc, char *argv[])
{
	

	enum { MAX_ARGS = 10 };
	int i,new_argc;
	char *new_argv[MAX_ARGS];
	bool is_durable;
	std::string argx;
	umask(0);
			/* Process the "--plus" option apart */
	for (i=0, new_argc=0; (i<argc) && (new_argc<MAX_ARGS); i++) {
		argx = std::string(argv[i]);
        if (argx == "-ip"){
            SERVER_ADDY = std::string(argv[++i]);
        } else if (argx == "-path") {
            CACHE_PATH = std::string(argv[++i]);
        } else if (argx == "-dur") {
            is_durable = true;
        } else {
            new_argv[new_argc++] = argv[i];
        }                
	}

	std::cout << "Cache Path is " << CACHE_PATH << std::endl;
	std::cout << "IP addy is " << SERVER_ADDY << std::endl;

	afesq = new AFEsqClient(CACHE_PATH, SERVER_ADDY, is_durable);
	return fuse_main(new_argc, new_argv, &xmp_oper, NULL);
}