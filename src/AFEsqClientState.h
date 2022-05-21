
/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#ifndef AFESQCLIENTSTATE_H
#define AFESQCLIENTSTATE_H

#include <map>
#include <iostream>
#include "timer.h"

struct file_node{
    bool is_dirty = false;
    std::string srv_path = "";
    std::string cache_path = "";
    std::string tmp_path = "";
    struct timespec last_mod;
    struct timespec last_write;
};

class AFEsqClientState {
    std::map<int, file_node> node_map;
public:    
     
    void add_node(int _fd, std::string _srv_path, 
                            std::string _cache_path,
                            std::string _tmp_path = ""){
        node_map[_fd].srv_path = _srv_path;
        node_map[_fd].cache_path = _cache_path;
        node_map[_fd].tmp_path = _tmp_path;
        set_last_mod(_fd);
        set_last_write(_fd);
    }
    void set_dirty(int fd){node_map[fd].is_dirty=true;}
    bool get_dirty(int fd){return node_map[fd].is_dirty;}
    std::string get_srv_path(int fd){return node_map[fd].srv_path;}
    std::string get_tmp_path(int fd){return node_map[fd].tmp_path;}
    std::string get_cache_path(int fd){return node_map[fd].cache_path;}
    void pop(int fd){node_map.erase (fd);}  
    void set_last_mod(int fd){get_time(&node_map[fd].last_mod);}  
    struct timespec get_last_mod(int fd){return  node_map[fd].last_mod;}
    void set_last_write(int fd){get_time(&node_map[fd].last_write);}  
    struct timespec get_last_write(int fd){return  node_map[fd].last_write;}
};


#endif