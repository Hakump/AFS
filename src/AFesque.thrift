 #include <sys/stat.h>

/*******
* Author: C. Todd Hayes-Birchler
* Date: 2/19/22
* CS 739, UW Madison, CS Department
*
* Thrift i32erface for stub generation
********/


exception ServerException {
    1: i32 err_no;
}

struct FStatShort {
    1: i64 st_mtimes_tv_sec;
    2: i64 st_mtimes_tv_nsec;
    3: i64 st_atimes_tv_sec;
    4: i64 st_atimes_tv_nsec;
    5: bool is_compressed;
    6: i32 num_chunks;
}

struct FStat {
    1: i64 st_dev;
    2: i64 st_ino;
    3: i32 st_mode;
    4: i64 st_nlink;
    5: i32 st_gid;
    6: i64 st_rdev;
    7: i64 st_size;
    8: i64 st_blksize;
    9: i64 st_blocks;
    10: i64 st_mtimes_tv_sec;
    11: i64 st_mtimes_tv_nsec;
    12: i64 st_atimes_tv_sec;
    13: i64 st_atimes_tv_nsec;
    14: i64 st_ctimes_tv_sec;
    15: i64 st_ctimes_tv_nsec;
}

struct tDirent {
    1: i64 d_ino;
    2: i64 d_off;
    3: i16 d_reclen;
    4: i8 d_type;
    5: string d_name;
}

// typedef list<tDirent> DentList;

// Our Marshaling service
service AFesqueSvc {
  
  // Fetch (pathname) --> Return contents of file at pathname (full path from srvfs)
  string Fetch(1:string path) throws (1: ServerException se);
  string Fetch_Chunk(1:string path, 2:i32 chunk) throws (1: ServerException se);
  
  // Store() --> Store a file on the server  
  FStatShort Store(1:string path, 2:string file_str) throws (1: ServerException se);
  
  // remove(pathname) --> remove file from server
  void Remove(1:string path) throws (1: ServerException se);

  // create(path) --> Creates file on server
  void Create(1:string path, 2:i32 mode) throws (1: ServerException se);

  // makedir(path) --> Make dir
  void MakeDir(1:string path, 2:i32 mode) throws (1: ServerException se);

  // removedir()
  void RemoveDir(1:string path) throws (1: ServerException se);
  
  // GetFileStat() --> Get stat info for file
  FStatShort TestAuth(1:string path) throws (1: ServerException se);

  FStat GetFileStat(1:string path) throws (1: ServerException se);
  
  // ListDir() --> List contents of a directory
  list<tDirent> ListDir(1:string path);

  // For concurrency testing
  void server_test(1:string msg, 2:i32 delay);
}
