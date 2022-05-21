/** 
 TODO: 
  1) Make checking is_dirty toggelable between
     a) client state
     b) check server timestamps
  2) Add garbage collection to fetch - should make for a good performance graph
  3) Deal with flags on open

 Notes: 
  1 - Client side durability and crash consistency
    a) It may be useful for client to periodically flush writes with fsnch.  This should update
     last modified but may want to verify
    b) When client comes back online with current protocol it will check last modified date
     against server.  So long as cache copy is most recent, it will continue to use cache with
     at least some user data saved.  However, if another client has pushed out its own update
     it would pull new version instead of using client version.  The question is, do we want
     to have a way to track what was being worked on prior to crash, and have a start up protocol
     that opens all local copies and either waits for FUSE to flush, or auto pushes them to server
     so if client re-opens it will have full cached version?  Maybe think about this more and 
     discuss with group

     
*/
//Including C++ Headers
#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <stdio.h>
#include "timer.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "AFEsqClient.h"
#include <errno.h>
#include "shared.h"

std::string STORE_PATH = "../srvfs";

/*#####################
# Public
######################*/


// Constructor
AFEsqClient::AFEsqClient(std::string _cache_path, std::string _server_addy) {
    cache_path = _cache_path;
    server_addy = _server_addy;
    build_cxn();
    client_state = new AFEsqClientState();
}

// AFEsqClient::~AFEsqClient(){
//     delete client;    
//     delete protocol;
//     delete transport;
//     delete socket;
// }



/**
 open()
 Follow Open() protocol
    a) Check and see if file is on server, if so get last modified
    b) if local file does not exist, or does exist but is older 
        than server, pull down new version from server
    c) If local file does exist, and is >= server version, user local file
    Ends with correct version of file in local cache
    should then be opened
 TODO: 
 1) a. Do we need to deal with renames?
 5) finally if I really have time I should be able to stream binary instead of string
***/
int AFEsqClient::open(const char* pathname, int flags, mode_t mode ){    
    
    struct stat local_stat;    
    struct FileStat remote_stat;
    std::string userpath = convertToString(pathname, strlen(pathname));
    std::string full_path = get_cache_path(userpath);
    int fd;

    while(true){
        try {
            transport->open();

            try{
                client->TestAuth(remote_stat, userpath);
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
                
            // Check and see if file is in cache and if up to date     
            if (::stat(full_path.c_str(), &local_stat) < 0 ||
                local_stat.st_mtime < remote_stat.st_mtimes) {            
                // Get file from server and check for errors
                std::string srv_file;
                try{
                    client->Fetch(srv_file, userpath);          
                } catch (const ServerException se){
                    errno = se.err_no;
                    return -1;
                } 
                            
                // Atomically Create new file from string, get fd
                // will result in cached version
                fd = atomic_save(full_path, srv_file);
                if (fd<0) return -1;
                // Now need to close so we can re-open with user flags
                if (::close(fd) < 0){return -1;}
            } 
            // Open cache version
            fd = ::open(full_path.c_str(), flags, mode);
            if (fd < 0){return -1;}   
            transport->close();
            break;
        } catch (TException &tx){
            cxn_err_handler(tx);
        }  
    }   
    client_state->add_node(fd, userpath);
    // std::cout << "Open is returning FD " << fd << std::endl;
    return fd;
}

/**
 close()
 Close Protocol
    1) Flush to disk
    2) If dirty
        a) Convert to string
        b) Send to server
        c) Receive timestamp from server
        d) Set local timestamp to match
    3) close fd
 */
int AFEsqClient::close(int fd){
    // Check if dirty
    int orig_fd = fd;
    if (client_state->get_dirty(fd)) { 
        // Force flush
        std::cout << "closing FD " << fd << std::endl;
        if (fsync(fd) < 0){
            ::close(fd);
            return -1;
        }

        // can't change open flags after open
        // but can't risk user flags not allowing for read
        // So close, and then re-open
        if (::close(fd) <0){return -1;}   
        // re-open file with necessary flags
        std::string full_path = get_cache_path(client_state->get_srv_path(fd));
        fd = ::open(full_path.c_str(), O_RDONLY, DEF_MODE);

        // get file deets and write to string
        int length = lseek(fd, 0, SEEK_END);
        if (length<0){return -1;}
        if (lseek(fd, 0, SEEK_SET) < 0) {return -1;}
        char *pChars = new char[length];
        // int len_read = 0;
        // while (len_read < length){
        //     read(fd, pChars, length-len_read);
        // }
        int read_length = read(fd, pChars, length);
        std::string file = convertToString(pChars, length);

        // Try to send data to server
        while (true){
            try{
                transport->open();
                try{
                    // Send file to server, get server time created and update here
                    FileStat resp;
                    client->Store(resp, client_state->get_srv_path(orig_fd), file);
                    // Server wrote file, update our timestamp to match server 
                    std::string cache_path = get_cache_path(client_state->get_srv_path(orig_fd));
                    struct utimbuf times;
                    times.actime = resp.st_atimes;
                    times.modtime = resp.st_mtimes;
                    if (utime(cache_path.c_str(), &times) < 0){
                        return -1;
                    }
                } catch (const ServerException se){
                    errno = se.err_no;
                    return -1;
                } 
                transport->close();
                break;
            } catch (TException &tx){
                cxn_err_handler(tx);
            }
        }
    }    
    ::close(fd);
    return 0;
}

ssize_t AFEsqClient::write(int fd, const void *buf, size_t count){
    client_state->set_dirty(fd);
    return ::write(fd, buf, count);
}

ssize_t AFEsqClient::pwrite(int fd, const void *buf, size_t count, off_t offset){
    client_state->set_dirty(fd);
    return pwrite(fd, buf, count, offset);
}

/**
 creat()
 Creates a new file
 return: 0 on success, -1 on fail, check errno for details
*/
int AFEsqClient::creat(const char* userpath, mode_t mode){
    while (true){
        try{
            transport->open();
            try{
                // client->Create(userpath, mode);            
                client->Create(userpath, mode);            
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
            transport->close();
            break;
        } catch (TException &tx){
            cxn_err_handler(tx);
        }
    }
    return 0;
}

/**
 unlink()
 deletes a file
 return: 0 on success, -1 on fail, check errno for details
*/
int AFEsqClient::unlink(const char* userpath){
    while (true){
        try{
            transport->open();
            try{
                client->Remove(userpath);
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
            transport->close();
            break;
        } catch (TException &tx){
            cxn_err_handler(tx);
        }
    }
    return 0;
}

/**
 mkdir()
 makes new directory
 return: 0 on success, -1 on fail, check errno for details
*/
int AFEsqClient::mkdir(const char* userpath, mode_t mode) {
    while(true){
        try{
            transport->open();
            try{
                client->MakeDir(userpath, mode);
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
            transport->close();
            break;
        } catch (TException &tx){
            cxn_err_handler(tx);
        }
    }    
    return 0;
}


/**
 rmdir()
 removes directory
 return: 0 on success, -1 on fail, check errno for details
*/
int AFEsqClient::rmdir(const char* userpath){
    while(true){
        try{
            transport->open();
            try{
                client->RemoveDir(userpath);
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
            transport->close();
            break;
        } catch (TException &tx){
            cxn_err_handler(tx);
        }
    }
    return 0;
}


int AFEsqClient::stat(const char* userpath, struct stat *buff){
    std::string file_path = convertToString(userpath, strlen(userpath));
    file_path = STORE_PATH + "/" + file_path;
    //std::cout << "Pulling stat from " << file_path <<std::endl;
    return stat(file_path.c_str(), buff);
    
    // while(true){
    //     try{
    //         transport->open();
    //         try{
    //             client->GetFileStat(&buff, userpath);
    //         } catch (const ServerException se){
    //             errno = se.err_no;
    //             return -1;
    //         } 
    //         transport->close();
    //         break;
    //     } catch (TException &tx){
    //         cxn_err_handler(tx);
    //     }
    // }    
    //return 0;
}

struct dirent *readdir(char* userpath){
    std::string file_path = convertToString(userpath, strlen(userpath));
    file_path = STORE_PATH + "/" + file_path;
    DIR* dp = opendir(file_path.c_str());
    struct dirent *de;
    de = readdir(dp);
    closedir(dp);
    return de;
}
/***
* Can be used to test concurrenncy
****/
void AFEsqClient::server_test_local(std::string args){        
    int pos = args.find(" ",0);
    std::string delay_str = args.substr(0, pos);
    std::string msg = args.substr(pos+1, args.length());
    int delay = std::stoi(delay_str);
    transport->open();
    client->server_test(msg, delay);
    transport->close();
}


/*#####################
# Protected\Helper
######################*/

/**
 May not be needed, but this is where we will deal with pipe broken cxn error if we can replicate
 likely just need to rerun some part of constructor (client = new AFes... likely)
*/
void AFEsqClient::build_cxn(){
    //TODO: Def should be a cleaner way to do this, but I was having issues with 
    //make_shared;
    std::shared_ptr<TTransport> _socket(new TSocket(server_addy, 9090));
    socket = std::move(_socket);
    std::shared_ptr<TTransport> _transport(new TBufferedTransport(socket));
    transport = std::move(_transport);
    std::shared_ptr<TProtocol> _protocol(new TBinaryProtocol(transport));
    protocol = std::move(_protocol);
    client = new AFesqueSvcClient(protocol);
}

/**
 Deal with different possible cxn errors
 if err = TSocket::open() connect() <Host: localhost Port: 9090>: Connection refused 
 then reopening server fixes
 but if err = TSocket::write_partial() send() <Host: localhost Port: 9090>: Broken pipe
 then reopening server does not resolve, may need to rebuild pipe
    NOTE: Having issues replicating this error
*/
void AFEsqClient::cxn_err_handler(TException &tx){
    std::cout << "TEx:" << tx.what() << ":" << std::endl;
    std::string what_str = convertToString(tx.what(), strlen(tx.what()));
    if (what_str.find("Broken pipe") != std::string::npos){
        std::cout << "Broken pipe, oh no, deal with that here" << std::endl;
        build_cxn();
    }
    sleep(1);
}

/**
 get_cache_path()
 converts path to file on server to file name used in local cache
 changes dir1/dir2/file.txt to dir1.dir2.file.txt
 Not the cleanest approach, but it works for a prototpye
*/
std::string AFEsqClient::get_cache_path(std::string userpath){
    int pos = userpath.find("/",0);
    if (pos == 0){
        userpath = userpath.substr(1, userpath.length()-1);
    }
    //std::string delay_str = userpath.substr(0, pos);
    replace_all(userpath, "/", ".");
    // std::cout << "full path userpath = " << userpath << std::endl;
    return cache_path + "/" + userpath;
}

