/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#ifndef AFESQCLIENT_H
#define AFESQCLIENT_H

//Thrift libraries
#include <thrift/protocol/TBinaryProtocol.h>             
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TSocket.h>                    
#include <thrift/transport/TBufferTransports.h>          
#include <thrift/transport/TTransportUtils.h>

//Boost libraries
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

//Namespaces
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using boost::make_shared;

//Including the thrift generated files 
#include "gen-cpp/AFesqueSvc.h"
#include "gen-cpp/AFesque_types.h"

#include "AFEsqClientState.h"
#include <dirent.h>
#include <list>

const int DEF_MODE = 0777;
// Elapsed time in seconds since last write that will trigger fsync
const int FSYNC_INT = 5;
// If durable, how long in seconds between pushing out save
const int DURABLE_SAVE = 15;

static std::string STORE_PATH2 = "../srvfs";

class AFEsqClient {
private:
    std::shared_ptr<TTransport> socket;
    std::shared_ptr<TTransport> transport;
    std::shared_ptr<TProtocol>  protocol;
    AFesqueSvcClient* client;
    AFEsqClientState* client_state;
    std::string cache_path;
    std::string server_addy;
    bool durable = true;

public:    
    AFEsqClient(std::string _cache_path, std::string _server_addy, bool _is_durable = false); 
    // ~AFEsqClient();   
    int open(const char* userpath, int flags, mode_t mode = DEF_MODE);
    int creat(const char* userpath, int flags, mode_t mode = DEF_MODE); 
    int close(int fd);
    ssize_t write(int fd, const void *buf, size_t count);
    ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);    
    int unlink(const char* userpath);
    int mkdir(const char* userpath, mode_t mode);
    int rmdir(const char* userpath);    
    int stat(const char* userpath, struct stat *return_stat);
    std::vector<tDirent> afs_readdir(const char* userpath);
    // std::vector<struct dirent*>  afs_readdir(const char* userpath);
    void server_test_local(std::string args);
    // May not be needed
    //DirList directory(std::string path);
protected:
    void cxn_err_handler(TException &tx);
    void build_cxn(bool);
    int write_helper(int fd);
    std::string get_cache_path(std::string userpath);
    // std::string get_temp_path(std::string userpath);
};

#endif
