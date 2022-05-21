/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/
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


/*#####################
# Public
######################*/


// Constructor
AFEsqClient::AFEsqClient(std::string _cache_path, std::string _server_addy, bool _durable) {
    cache_path = _cache_path;
    server_addy = _server_addy;
    durable = _durable;
    build_cxn(true);
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
    std::cout << "#### CLIENT OPEN ####" << std::endl;
    std::cout << "#### " << pathname << "   ####" << std::endl;

    struct stat local_stat;    
    struct FStatShort remote_stat;
    struct timespec r_stat;
    std::string userpath = convertToString(pathname, strlen(pathname));
    std::string cache_path = get_cache_path(userpath);
    
    int fd;

    while(true){
        try {
            transport->open();

            try{
                client->TestAuth(remote_stat, userpath);
                // std::cout << "Remote stat returned "
                r_stat.tv_sec = remote_stat.st_mtimes_tv_sec;
                r_stat.tv_nsec = remote_stat.st_mtimes_tv_nsec;
                
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
                
            // Check and see if file is in cache and if up to date     
            if (::stat(cache_path.c_str(), &local_stat) < 0 ||
                difftimespec_ns(r_stat, local_stat.st_mtim) < 0){       
                // local_stat.st_mtime < remote_stat.st_mtimes) {     
                    // difftimespec_ns       
                // Get file from server and check for errors

                /* 
                 Adding chunks
                 1) Client needs to get size from TestAuth
                 2) Client needs know its chunk size
                 3) Client sends in offset and chunk size
                 4) Client calculates next chunk and requests
                 4) Atomic save per chunk?
                 */
                 
                std::string srv_file;                
                try{
                    srv_file = "";
                    if (remote_stat.num_chunks == -1){
                        client->Fetch(srv_file, userpath);  
                    } else {
                        for (int i = 0; i < remote_stat.num_chunks; i++){
                            std::cout << "...Fetchin chunk = " << i+1 << " of " << remote_stat.num_chunks << std::endl;
                            std::string chunk = "";
                            client->Fetch_Chunk(chunk, userpath, i); 

                            // client->Fetch_Chunk(chunk, userpath, i);  
                            srv_file += chunk;
                        }
                    }
                    
                    // if (remote_stat.is_compressed){
                    //     Gzip::decompress(srv_file);
                    // }
                } catch (const ServerException se){
                    errno = se.err_no;
                    return -1;
                } 
                            
                // Atomically Create new file from string, get fd
                // will result in cached version
                fd = atomic_save(cache_path, srv_file);
                std::cout << "AS returned fd " << fd << std::endl;
                if (fd<0) return -1;
                // Now need to close so we can re-open with user flags
                if (::close(fd) < 0){return -1;}
            }             
            transport->close();
            break;
        } catch (TException &tx){
            cxn_err_handler(tx);
        }  
    }         

    /*
     if durable then we want to 
        a. create .tmp file
        b. copy contents of this file into .tmp file
        c. close this fd
        d. return new fd
    otherwise
        a. return current fd
    */
    std::string tmp_path = "";
    if (durable){
        tmp_path = cache_path + ".tmp";
        fd = build_tmp_file(cache_path, tmp_path);
        if (::close(fd) < 0){return -1;}
        fd = ::open(tmp_path.c_str(), flags, mode);
    } else {
        fd = ::open(cache_path.c_str(), flags, mode);
    }
    //user path = server path
    // std::cout << "Open is returning FD " << fd << std::endl;
    // Open FD and return to user
    // std::cout << "AS returned is about to open fd" << std::endl;    
    
    if (fd < 0){return -1;} 
    client_state->add_node(fd, userpath, cache_path, tmp_path);
    return fd;
}

// int AFEsqClient::OpenChunk(const char* pathname, int flags, mode_t mode ){    
//     std::cout << "#### CLIENT OPEN ####" << std::endl;
//     std::cout << "#### " << pathname << "   ####" << std::endl;

//     struct stat local_stat;    
//     struct FStatShort remote_stat;
//     struct timespec r_stat;
//     std::string userpath = convertToString(pathname, strlen(pathname));
//     std::string full_path = get_cache_path(userpath);
    
//     int fd;

//     while(true){
//         try {
//             transport->open();

//             try{
//                 client->TestAuth(remote_stat, userpath);
//                 // std::cout << "Remote stat returned "
//                 r_stat.tv_sec = remote_stat.st_mtimes_tv_sec;
//                 r_stat.tv_nsec = remote_stat.st_mtimes_tv_nsec;
                
//             } catch (const ServerException se){
//                 errno = se.err_no;
//                 return -1;
//             } 
                
//             // Check and see if file is in cache and if up to date     
//             if (::stat(full_path.c_str(), &local_stat) < 0 ||
//                 difftimespec_ns(r_stat, local_stat.st_mtim) < 0){       
//                 // local_stat.st_mtime < remote_stat.st_mtimes) {     
//                     // difftimespec_ns       
//                 // Get file from server and check for errors

//                 /* 
//                  Adding chunks
//                  1) Client needs to get size from TestAuth
//                  2) Client needs know its chunk size
//                  3) Client sends in offset and chunk size
//                  4) Client calculates next chunk and requests
//                  4) Atomic save per chunk?
//                  */
                 
                 
//                 std::string srv_file;
//                 try{
//                     client->Fetch(srv_file, userpath);          
//                 } catch (const ServerException se){
//                     errno = se.err_no;
//                     return -1;
//                 } 
                            
//                 // Atomically Create new file from string, get fd
//                 // will result in cached version
//                 fd = atomic_save(full_path, srv_file);
//                 std::cout << "AS returned fd " << fd << std::endl;
//                 if (fd<0) return -1;
//                 // Now need to close so we can re-open with user flags
//                 if (::close(fd) < 0){return -1;}
//             } 
//             // Open cache version
//             std::cout << "AS returned is about to open fd" << std::endl;
//             fd = ::open(full_path.c_str(), flags, mode);
//             if (fd < 0){return -1;}   
//             transport->close();
//             break;
//         } catch (TException &tx){
//             cxn_err_handler(tx);
//         }  
//     }   
//     client_state->add_node(fd, userpath);
//     std::cout << "Open is returning FD " << fd << std::endl;
//     return fd;
// }

/**
 creat()
 Creates a new file
 return: 0 on success, -1 on fail, check errno for details
 in fuse they try to open with presumably O_CREAT flags
*/
int AFEsqClient::creat(const char* userpath, int flags, mode_t mode){
    int fd;
    while (true){
        try{
            transport->open();
            try{
                client->Create(userpath, mode);  
                fd = AFEsqClient::open(userpath, flags, mode); 
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
    
    // Need to push temp to cache because if we send data first 
    // then crash server version will not match cache version
    // Also need to update timestamps on local version, not temp
    // really just need to call rename at this point
    // and overwrite server version
    std::string cache_path = client_state->get_cache_path(fd);
    if (durable){
        if (fsync(fd) < 0){
            std::cout << "Closing because of fsync" << std::endl;
            ::close(fd);
            return -1;
        }
        std::string tmp_path = client_state->get_tmp_path(fd);
        if (rename(tmp_path.c_str(), cache_path.c_str()) < 0){
            ::close(fd);
            return -1;
        }
    } 

    if (client_state->get_dirty(fd)) { 
        // Force flush
        std::cout << "closing FD " << fd << std::endl;
        if (!durable){
            if (fsync(fd) < 0){
                std::cout << "Closing because of fsync 2" << std::endl;
                ::close(fd);
                return -1;
            }
        }
        
        // can't change open flags after open
        // but can't risk user flags not allowing for read
        // So close, and then re-open
        if (::close(fd) <0){return -1;}   
        // re-open file with necessary flags
        // std::string full_path = get_cache_path(cache_path);
        fd = ::open(cache_path.c_str(), O_RDONLY, DEF_MODE);

        // get file deets and write to string
        int length = lseek(fd, 0, SEEK_END);
        if (length<0){return -1;}
        if (lseek(fd, 0, SEEK_SET) < 0) {return -1;}
        char *pChars = new char[length];
        int read_length = read(fd, pChars, length);

        //TODO: Might be able to drop this converions
        std::string file = convertToString(pChars, length);
        delete pChars;
        // Try to send data to server
        while (true){
            try{
                transport->open();
                try{
                    // Send file to server, get server time created and update here
                    struct FStatShort resp;
                    client->Store(resp, client_state->get_srv_path(orig_fd), file);
                    // Server wrote file, update our timestamp to match server 
                    // std::string cache_path = get_cache_path(client_state->get_srv_path(orig_fd));
                    struct timespec times[2];
                    times[0].tv_sec = resp.st_atimes_tv_sec;
                    times[0].tv_nsec = resp.st_atimes_tv_nsec;
                    times[1].tv_sec = resp.st_mtimes_tv_sec;
                    times[1].tv_nsec = resp.st_mtimes_tv_nsec;
                    // times.actime = resp.st_atimes;
                    // times.modtime = resp.st_mtimes;
                    if (futimens(fd, times) < 0){
                        std::cout << "An error occured on futimens" << std::endl;
                        perror("Err:");
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
    client_state->pop(orig_fd);
    ::close(fd);
    return 0;
}

// Deals with fsync and durability on writes
int AFEsqClient::write_helper(int fd){    
    struct timespec now;
    get_time(&now);

    // 2) Deal with durability
    if (durable){
        if (difftimespec_s(client_state->get_last_write(fd) ,now) > DURABLE_SAVE){
            // std::cout << "SAVING TEMP" << std::endl;
            std::string tmp_file = client_state->get_tmp_path(fd);
            std::string cache_file = client_state->get_cache_path(fd);
            //this may inadvertantly close the file I am working on
            // off_t old_position = lseek(fd, 0, SEEK_CUR);
            if (fsync(fd) < 0){
                std::cout << "Closing because of fsync" << std::endl;
                ::close(fd);
                return -1;
            }
            int cache_fd = build_tmp_file(tmp_file, cache_file);
            if (cache_fd < 0) std::cout << "Bad Copy FD" << std::endl;
            ::close(cache_fd);
            // lseek(fd, old_position, SEEK_SET);  
            client_state->set_last_mod(fd);          
            client_state->set_last_write(fd);
            return 0;
        }
    }
    
    // 1) Check time of last write, if more than 5 seconds, syncs
    if (difftimespec_s(client_state->get_last_mod(fd) ,now) > FSYNC_INT){
        std::cout << "calling FSYNC" << std::endl;
        if (fsync(fd) < 0){
            std::cout << "Closing because of fsync" << std::endl;
            ::close(fd);
            return -1;
        }
        client_state->set_last_mod(fd);
    }

    
    

    return 0;
}

ssize_t AFEsqClient::write(int fd, const void *buf, size_t count){
    client_state->set_dirty(fd);
    int res = ::write(fd, buf, count);
    if (res < 0) return -1;
    write_helper(fd);
    return res;
}

ssize_t AFEsqClient::pwrite(int fd, const void *buf, size_t count, off_t offset){
    client_state->set_dirty(fd);
    int res = ::pwrite(fd, buf, count, offset);
    write_helper(fd);
    return res;
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


int AFEsqClient::stat(const char* userpath, struct stat *_return_stat){
    // std::string file_path = convertToString(userpath, strlen(userpath));
    // std::string true_path = "/users/jmadrek/p2/srvfs" + file_path;
    // std::cout << "\n#--# Asking for stat on userpath " << userpath;
    // std::cout << "\n#--# Asking for stat on file_path " << file_path;
    // std::cout << "\n$$$$ Asking for stat on path " << true_path;
    // file_path = "/users/jmadrek/p2/srvfs";
    //std::cout << "Pulling stat from " << file_path <<std::endl;
    // return stat(file_path.c_str(), buff);
    // return lstat(true_path.c_str(), buff);
        
    // std::cout << "AFEsq Calling GetFileState on " << userpath <<std::endl;
    while(true){
        try{
            transport->open();
            struct FStat remote_stat;
            try{                
                // std::cout << "CALING CLIENT GETFILESTAT" << std::endl;
                client->GetFileStat(remote_stat, userpath);    
                // std::cout << "RETURNING CLIENT GETFILESTAT" << std::endl;            
            } catch (const ServerException se){
                errno = se.err_no;
                return -1;
            } 
            transport->close();
            // unpac stat
            // std::cout << "Todd: Unpacking GetFileStat results on client" << std::endl;
            _return_stat->st_dev = remote_stat.st_dev;
            _return_stat->st_ino = remote_stat.st_ino;
            _return_stat->st_mode = remote_stat.st_mode;
            _return_stat->st_nlink = remote_stat.st_nlink;
            _return_stat->st_gid = remote_stat.st_gid;
            _return_stat->st_rdev = remote_stat.st_rdev;
            _return_stat->st_size = remote_stat.st_size;
            _return_stat->st_blksize = remote_stat.st_blksize;
            _return_stat->st_blocks = remote_stat.st_blocks;
            _return_stat->st_mtim.tv_sec = remote_stat.st_mtimes_tv_sec;
            _return_stat->st_mtim.tv_nsec = remote_stat.st_mtimes_tv_nsec;
            _return_stat->st_atim.tv_sec = remote_stat.st_atimes_tv_sec;
            _return_stat->st_atim.tv_nsec = remote_stat.st_atimes_tv_nsec;
            _return_stat->st_ctim.tv_sec = remote_stat.st_atimes_tv_sec;
            _return_stat->st_ctim.tv_nsec = remote_stat.st_atimes_tv_nsec;            
            break;
        } catch (TException &tx){
            std::cout << "Caught exception in stat" <<std::endl;
            cxn_err_handler(tx);
        }
    }    
    return 0;
}

std::vector<tDirent> AFEsqClient::afs_readdir(const char* userpath){ 
//std::vector<struct dirent*> AFEsqClient::afs_readdir(const char* userpath){
    /*####################
    /* tDirent load from local path
    /*####################*/
    // std::string file_path = convertToString(userpath, strlen(userpath));
    // std::string true_path = "/users/jmadrek/p2/srvfs" + file_path;
    // std::cout << "&&&&&&&&&&&\n Serving path:" << true_path << ":\n&&&&&&&&&&&\n";
    // DIR *dp;
    // struct dirent* de;
    // struct tDirent tde;
    // // std::list<struct dirent*> dent_list;
    // std::vector<tDirent>  dent_list;

    // dp = opendir(true_path.c_str());
    // while ((de = readdir(dp)) != NULL) {
    //     tde.d_ino = de->d_ino;
    //     tde.d_off = de->d_off;
    //     tde.d_reclen = de->d_reclen;
    //     tde.d_type = de->d_type;
    //     tde.d_name = convertToString(de->d_name, strlen(de->d_name));
    //     dent_list.push_back(tde);
    // }
    // closedir(dp);
    // std::cout << "&&&&&&&&&&&\n Returning list of size " << dent_list.size() << "\n&&&&&&&&&&&\n";
    // return dent_list;


    // std::string path = convertToString(userpath, strlen(userpath));
    /*####################
    /* Load from server
    /*###################*/
    std::string file_path = convertToString(userpath, strlen(userpath));
    std::cout << "Entering afs_readdir" << std::endl;
    std::vector<tDirent> dent_list;
    while(true){        
        try{
            
            std::cout << "Client Calling listDir on " << userpath << std::endl;
            std::cout << "dent_list size is " << dent_list.size() << std::endl;
            transport->open();
            try{
                client->ListDir(dent_list, file_path);
                std::cout << "return size is " << dent_list.size() << std::endl;
            } catch (const ServerException se){
                errno = se.err_no;
                perror("");
            } 
            transport->close();
            // dent_list = &send_list;
            break;
        } catch (TException &tx){
            transport->close();
            cxn_err_handler(tx);
        }        
    }
    return dent_list;

    /*####################
    /* Try 2 Load from server
    /*###################*/
    // std::string file_path = convertToString(userpath, strlen(userpath));
    // std::cout << "Entering afs_readdir" << std::endl;
    // std::vector<tDirent> tdent_list;
    // while(true){        
    //     try{
            
    //         std::cout << "Client Calling listDir on " << userpath << std::endl;
    //         std::cout << "dent_list size is " << tdent_list.size() << std::endl;
    //         transport->open();
    //         try{
    //             client->ListDir(tdent_list, file_path);
    //         } catch (const ServerException se){
    //             errno = se.err_no;
    //             perror("");
    //         } 
    //         transport->close();
    //         // dent_list = &send_list;
    //         break;
    //     } catch (TException &tx){
    //         transport->close();
    //         cxn_err_handler(tx);
    //     }        
    // }

    // std::vector<struct dirent*> dent_list;
    // struct dirent* new_de;
    // for (int i = 0; i < tdent_list.size(); i++){
    //     new_de = new struct dirent;
    //     std::cout << "Packing " << tdent_list[i].d_name << std::endl;
    //     new_de->d_ino = tdent_list[i].d_ino;
    //     new_de->d_off = tdent_list[i].d_off;
    //     new_de->d_reclen = tdent_list[i].d_reclen;
    //     new_de->d_type = tdent_list[i].d_type;
    //     strcpy(new_de->d_name, tdent_list[i].d_name.c_str());
    //     // new_de->d_name = tde.d_name;
    //     std::cout << "Packed " << new_de->d_name << std::endl;
    //     dent_list.push_back(new_de);
    // }

    // for (int i = 0; i < dent_list.size(); i++){
    //     std::cout << "Item " << i << " is named " << dent_list[i]->d_name << std::endl;
    // }

    
    // return dent_list;

    // /*####################
    // /* try 2 with 
    // /*####################*/

    // std::string file_path = convertToString(userpath, strlen(userpath));
    // std::string true_path = "/users/jmadrek/p2/srvfs" + file_path;
    // std::cout << "&&&&&&&&&&&\n Serving path " << true_path << "\n&&&&&&&&&&&\n";
    // DIR *dp;
    // struct dirent* de;
    // struct tDirent tde;
    // // std::list<struct dirent*> dent_list;
    // std::vector<tDirent> tdent_list;
    // std::vector<struct dirent*> dent_list;

    // dp = opendir(true_path.c_str());
    // // while ((de = readdir(dp)) != NULL) {
    // //     tde = new tDirent;
    // //     tde->d_ino = de->d_ino;
    // //     tde->d_off = de->d_off;
    // //     tde->d_reclen = de->d_reclen;
    // //     tde->d_type = de->d_type;
    // //     tde->d_name = convertToString(de->d_name, strlen(de->d_name));
    // //     tdent_list.push_back(*tde);
    // //     std::cout << "Getting " << tde->d_name << std::endl;
    // // }
    // // closedir(dp);
    // while ((de = readdir(dp)) != NULL) {
    //     tde.d_ino = de->d_ino;
    //     tde.d_off = de->d_off;
    //     tde.d_reclen = de->d_reclen;
    //     tde.d_type = de->d_type;
    //     tde.d_name = convertToString(de->d_name, strlen(de->d_name));
    //     tdent_list.push_back(tde);
    //     std::cout << "Getting " << tde.d_name << std::endl;
    // }
    // closedir(dp);

    // struct dirent* new_de;
    // for (int i = 0; i < tdent_list.size(); i++){
    //     new_de = new struct dirent;
    //     std::cout << "Packing " << tdent_list[i].d_name << std::endl;
    //     new_de->d_ino = tdent_list[i].d_ino;
    //     new_de->d_off = tdent_list[i].d_off;
    //     new_de->d_reclen = tdent_list[i].d_reclen;
    //     new_de->d_type = tdent_list[i].d_type;
    //     strcpy(new_de->d_name, tdent_list[i].d_name.c_str());
    //     // new_de->d_name = tde.d_name;
    //     std::cout << "Packed " << new_de->d_name << std::endl;
    //     dent_list.push_back(new_de);
    // }

    // for (int i = 0; i < dent_list.size(); i++){
    //     std::cout << "Item " << i << " is named " << dent_list[i]->d_name << std::endl;
    // }
    // std::cout << "&&&&&&&&&&&\n Returning list of size " << dent_list.size() << "\n&&&&&&&&&&&\n";
    // return dent_list;

    // /*####################
    // /* Normal load from local path
    // /*####################*/

    // std::string file_path = convertToString(userpath, strlen(userpath));
    // std::string true_path = "/users/jmadrek/p2/srvfs" + file_path;
    // std::cout << "&&&&&&&&&&&\n Serving path " << true_path << "\n&&&&&&&&&&&\n";
    // DIR *dp;
    // struct dirent* de;
    // // std::list<struct dirent*> dent_list;
    // std::vector<struct dirent*> dent_list;

    // dp = opendir(true_path.c_str());
    // while ((de = readdir(dp)) != NULL) {
    //     dent_list.push_back(de);
    // }
    // closedir(dp);
    // std::cout << "&&&&&&&&&&&\n Returning list of size " << dent_list.size() << "\n&&&&&&&&&&&\n";
    // return dent_list;
    // return 0;
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
void AFEsqClient::build_cxn(bool is_new){
    //TODO: Def should be a cleaner way to do this, but I was having issues with 
    //make_shared;
    if (!is_new){
        socket.reset();
        transport.reset();
        protocol.reset();
        delete client;
    }
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
    if (what_str.find("Broken pipe") != std::string::npos ||
        what_str.find("MaxMessageSize reached") != std::string::npos){
        std::cout << "REBUILDING CXN" << std::endl;
        build_cxn(false);
    }
    // if (what_str.find("No more data to read") != std::string::npos){
    //     sleep(2);
    //     return;
    // } else {
    //     build_cxn();
    // }
    sleep(2);
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
        // userpath = userpath.substr(1, userpath.length()-1);
        userpath = userpath.substr(1, userpath.length()-1);
    }
    //std::string delay_str = userpath.substr(0, pos);
    replace_all(userpath, "/", ".");
    // std::cout << "full path userpath = " << userpath << std::endl;
    return cache_path + "/" + userpath;
}

