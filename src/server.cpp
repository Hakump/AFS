/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

//Including the thrift generated files 
#include "gen-cpp/AFesqueSvc.h"
#include "gen-cpp/AFesque_types.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <boost/make_shared.hpp>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using boost::make_shared;

// Additional includes
#include <fstream>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "shared.h"
#include <dirent.h>
#include <list>

// Globals
std::string STORE_PATH = "../srvfs";
bool RUN_CONCURRENT = false;
bool USE_COMPR = false;
bool USE_CHUNK = false;
int CHUNK_SIZE = -1;

// Helpful functions
std::string GetFullPath(std::string path);
void check_dir_rec(std::string path, bool start);

/*#####################
# AFEsq Interface Implementation
######################*/

class AFesqueSvcHandler : virtual public AFesqueSvcIf {
 public:
  AFesqueSvcHandler() {
    printf("The server is listening, serving files from %s.\n", STORE_PATH.c_str());
  }

  /**
  * streams binary file content back to client
  * _return: file + status we return to client
  * path: path of file client is requesting from persistant store
  ***/
  void Fetch(std::string& _return, const std::string& path) {    
    std::string full_path = GetFullPath(path);
    std::cout << "Fetch " << path << std::endl;

    // Open file to start, verify open
    std::ifstream file(full_path, std::ios::binary|std::ios::ate);
    if (!file.is_open()){
      std::cerr << "...(FETCH) errno: " << errno << std::endl;
      perror("...(FETCH) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    }

    // Read in file to memory
    std::ifstream::pos_type pos = file.tellg();
    int length = pos;
    char *pChars = new char[length];
    file.seekg(0, std::ios::beg);
    file.read(pChars, length);
    file.close();
    
    // Return file plus status
      std::string ret(pChars);
//    ret = convertToString(pChars, length);
    delete pChars;
    // if (USE_COMPR) {      
    //   Gzip::compress(ret);
    // }
    
    //ret = convertToString(pChars, length); 
    
    _return = ret;
  }

  void Fetch_Chunk(std::string& _return, const std::string& path, int chunk) {    
    std::string full_path = GetFullPath(path);
    std::cout << "Fetch (" << chunk+1 << ") " << path << std::endl;

    // Open file to start, verify open
    std::ifstream file(full_path, std::ios::binary|std::ios::ate);
    if (!file.is_open()){
      std::cerr << "...(FETCH) errno: " << errno << std::endl;
      perror("...(FETCH) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    }    

    //calculate offset
    long offset = chunk*CHUNK_SIZE;
    int chunk_size = CHUNK_SIZE;
    std::ifstream::pos_type pos = file.tellg();
    long length = pos;
    if (offset + CHUNK_SIZE > length){
      //end of file, only return what is needed
      chunk_size = length % CHUNK_SIZE;
    }

    // Read in file to memory
    char *pChars = new char[chunk_size];    
    file.seekg(offset, std::ios::beg);
    file.read(pChars, chunk_size);
    file.close();
    
    // Return file plus status
    std::string ret(pChars);
    // std::cout << "Returning (" << chunk << ":" << ret << ":" << std::endl ;
    delete pChars;
    // if (USE_COMPR) {      
    //   Gzip::compress(ret);
    // }
    
    _return = ret;
  }

  /**
   Store file to server, return timestamps
   1)?? verify file is present.  If not do we need to create it??
   2) atomic_save file to server
   3) get timestamps from file and return 
  */
  void Store(FStatShort& _return, const std::string& path, const std::string& file_str) {    
    struct stat buf;  
    std::string full_path = GetFullPath(path);
    std::cout << "Store " << full_path << std::endl;
    // Trust client, verify directory exists and create it if it does not
    check_dir_rec(path, true);
    int fd = atomic_save(full_path, file_str);
    if (fd<0 || fstat(fd, &buf) < 0){
      std::cerr << "...(Store) errno: " << errno << std::endl;
      perror("...(Store) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    };
    

    _return.st_mtimes_tv_sec = buf.st_mtim.tv_sec;
    _return.st_mtimes_tv_nsec = buf.st_mtim.tv_nsec;
    _return.st_atimes_tv_sec = buf.st_atim.tv_sec;
    _return.st_atimes_tv_nsec = buf.st_atim.tv_nsec;
    close(fd);
  }



  /**
   call unlink() on file sent in
  */
  void Remove(const std::string& path) {
    // Your implementation goes here
    std::string full_path = GetFullPath(path);
    std::cout << "Remove " << path << std::endl;
    if (unlink(full_path.c_str()) < 0){
      std::cerr << "...(RMV) errno: " << errno << std::endl;
      perror("...(RMV) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    } 
  }

  /**
   Creates a file with mode protection.  
  */
  void Create(const std::string& path,  const int32_t mode) {
    std::string full_path = GetFullPath(path);
    std::cout << "Create " << path << std::endl;
    if (creat(full_path.c_str(), mode) < 0){
      std::cerr << "...(CR) errno: " << errno << std::endl;
      perror("...(CR) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    } 
  }

  /**
   Makes a new directory with mode protection
   */
  void MakeDir(const std::string& path,  const int32_t mode) {
    std::string full_path = GetFullPath(path);
    std::cout << "MakeDir " << path << std::endl;
    if (mkdir(full_path.c_str(), mode) < 0){
      std::cerr << "...(MD) errno: " << errno << std::endl;
      perror("...(MD) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    } 
  }

  /**
    Removes an existing directory at path
  */
  void RemoveDir(const std::string& path) {
    std::string full_path = GetFullPath(path);
    std::cout << "RemoveDir " << path << std::endl;
    if (rmdir(full_path.c_str()) < 0){
      std::cerr << "...(RD) errno: " << errno << std::endl;
      perror("...(RD) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    }      
  }

  /**
    Return stat data on file at path
    this incudes time created, last modified and last accessed
    TODO: This needs to return more, full set?
  */
  void TestAuth(FStatShort& _return, const std::string& path) {
    // Get file details
    std::string full_path  = GetFullPath(path);
    std::cout << " TestAuth  " << full_path << std::endl;
    // get details
    struct stat remote_stat;
    if (stat(full_path.c_str(), &remote_stat) < 0){
      std::cerr << "...(STAT) errno: " << errno << std::endl;
      perror("...(STAT) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    }

    // Calculate number of chunks and send to user
    int num_chunks;
    if (USE_CHUNK){
      // size = st_size
      // chunk size = CHUNK_SIZE  
      // std::cout << "...File Size: " << remote_stat.st_size << std::endl;
      // std::cout << "...Chunk Size: " << CHUNK_SIZE << std::endl;
      num_chunks = int(remote_stat.st_size / CHUNK_SIZE);
      if (remote_stat.st_size % CHUNK_SIZE != 0) {
        num_chunks++;
      }
      std::cout << "...Chunk Count: " << num_chunks << std::endl;
      _return.num_chunks = num_chunks;
    } else {
      _return.num_chunks = -1;
    }

    _return.st_mtimes_tv_sec = remote_stat.st_mtim.tv_sec;
    _return.st_mtimes_tv_nsec = remote_stat.st_mtim.tv_nsec;
    _return.st_atimes_tv_sec = remote_stat.st_atim.tv_sec;
    _return.st_atimes_tv_nsec = remote_stat.st_atim.tv_nsec;
    if (USE_COMPR) _return.is_compressed = true;
  }

  /**
  * Returns stat() on path
  */
  void GetFileStat(FStat& _return, const std::string& path) {
    // std::cout << "SRVR ENTERING STAT " << std::endl;
    std::string full_path = GetFullPath(path);
    std::cout << "Stat " << full_path << std::endl;
    // stores state return, need to repack into _return
    struct stat true_stat;
    if (lstat(full_path.c_str(), &true_stat) < 0){
    // if (stat("/users/jmadrek/p2/srvfs/test1.txt", &true_stat) < 0){
      std::cout << "...(STAT) errno: " << errno << std::endl;
      perror("...(STAT) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    } 

    //Pack stat
      _return.st_dev = true_stat.st_dev;
      _return.st_ino = true_stat.st_ino;
      _return.st_mode = true_stat.st_mode;
      _return.st_nlink = true_stat.st_nlink;
      _return.st_gid = true_stat.st_gid;
      _return.st_rdev = true_stat.st_rdev;
      _return.st_size = true_stat.st_size;
      _return.st_blksize = true_stat.st_blksize;
      _return.st_blocks = true_stat.st_blocks;
      _return.st_mtimes_tv_sec = true_stat.st_mtim.tv_sec;
      _return.st_mtimes_tv_nsec = true_stat.st_mtim.tv_nsec;
      _return.st_atimes_tv_sec = true_stat.st_atim.tv_sec;
      _return.st_atimes_tv_nsec = true_stat.st_atim.tv_nsec;
      _return.st_ctimes_tv_sec = true_stat.st_ctim.tv_sec;
      _return.st_ctimes_tv_nsec = true_stat.st_ctim.tv_nsec;
  }

  /**
  * returns list containing dir ents for a given path
  */
  void ListDir(std::vector<tDirent>& _return, const std::string& path){
    std::cout << "ListDir :" << path <<  ":" << std::endl; 
    std::string full_path = GetFullPath(path);
    DIR *dp;
    struct dirent* de;
    struct tDirent tde;

    // std::list<struct dirent*> dent_list;
    if ((dp = opendir(full_path.c_str())) < 0){
      std::cerr << "...(LD-O) errno: " << errno << std::endl;
      perror("...(LD-O) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    } 
    while ((de = readdir(dp)) != NULL) {
      tde.d_ino = de->d_ino;
      tde.d_off = de->d_off;
      tde.d_reclen = de->d_reclen;
      tde.d_type = de->d_type;
      tde.d_name = convertToString(de->d_name, strlen(de->d_name));
      _return.push_back(tde);
    }

    if (closedir(dp) < 0){
      std::cerr << "...(LD-C) errno: " << errno << std::endl;
      perror("...(LD-C) Error: ");
      ServerException se;
      se.err_no = errno;
      throw se;
    }     
    // return dent_list;
  }


  /**
   Returns file structur as some container, possibly dictionary
   May not be needed
  */
  // void ListDir(DirList& _return, const std::string& path) {
  //   // Your implementation goes here
  //   printf("ListDir\n");
  // }

  /****
  * Takes in a message and delay, sleeps for delay seconds
  * Should be useful for testing concurrency
  *****/
  void server_test(const std::string& msg, const int32_t delay) {
    std::cout << "Start Server Test: " << msg << " delay: " << delay << " (s)" << std::endl;
    sleep(delay);
    std::cout << "End Server Test: " << msg << " delay: " << delay << " (s)" << std::endl;
  }

};



/*#####################
# Helper Functions
######################*/

/**
 * Look to see if folder exists
 * if it does not, create it
 * case 1 - /test.txt
 * case 2 - /D1/D2/D3/test.txt
 */

void check_dir_rec(std::string path, bool start){
    struct stat sb;
    std::cout << "...Checking " << path << std::endl;
    errno = 0;
    int resp = stat(path.c_str(), &sb);
    // need to build dir
    if (resp<0 && errno == 2){ 
        int pos = path.rfind("/");
        // No more path, and path does not exist, return -1
        if (!(pos == std::string::npos || pos == 0)){            
          std::string check_path = path.substr(0, pos);          
          check_dir_rec(check_path, false);            
        } else if (pos == 0) {
            path = path.substr(1, std::string::npos);
        }
        if (!start){
          path = STORE_PATH + "/" + path;
          std::cout << "...(Store) mkdir: " << path << std::endl;  
          mkdir(path.c_str(), 0777);
        }                
    }     
}


/**
 Converts user pathway to full server pathway based
 on default storage dir
**/
std::string GetFullPath(std::string path){
  int pos = path.find("/",0);
  if (pos == 0){
      // path = path.substr(1, path.length()-1);
      path = path.substr(1, std::string::npos);
  }
  return STORE_PATH + "/" + path;
}

/**
 Parses arg(s) sent into server
*/
int parse_args(int argc, char** argv){    
    int arg = 1;
    std::string argx;
    while (arg < argc){
        if (argc < arg) return 0;
        argx = std::string(argv[arg]);
        if (argx == "-path") {
            STORE_PATH = std::string(argv[++arg]);
        } else if (argx == "-chunk") {
            // int chunk_size = std::stoi(argv[++arg]);
            USE_CHUNK = true;            
            CHUNK_SIZE = std::stoi(argv[++arg]);
            // std::cout << "Chunk size set to " << chunk_size << std::endl;
            // std::cout << "Arg was " << argv[arg] << std::endl;
        } else if (argx == "-con") {
            RUN_CONCURRENT = true;
        } else if (argx == "-comp") {
            USE_COMPR = true;
        } else {
            std::cout << "Usage: prog -path <serve path> (default = '/srvfs)\n"
                      <<  "            -con (run server conncurent, defaults to false)\n"
                      <<  "            -comp (fetch with compression, defaults to false)\n"
                      <<  "            -chunk <chunk_size> (Send as chunks of size <chukn_size>)\n";
                return 1;        
        }                
        arg++;
    }
    return 0;
}

int main(int argc, char **argv) {
  int port = 9090;
  if (parse_args(argc, argv) == 1) {return 0;}
  ::std::shared_ptr<AFesqueSvcHandler> handler(new AFesqueSvcHandler());

  ::std::shared_ptr<TProcessor> processor(new AFesqueSvcProcessor(handler));
  ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  if (RUN_CONCURRENT){
    TThreadedServer serverA(processor, serverTransport, transportFactory, protocolFactory);
    serverA.serve();
  } else {
    TSimpleServer serverB(processor, serverTransport, transportFactory, protocolFactory);
    serverB.serve();
  }
  
  // TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  // TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);

  
  return 0;
}
