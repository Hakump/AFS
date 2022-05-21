/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#include "shared.h"
#include "timer.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

/**
  Converts char array to string
*/
std::string convertToString(const char* a, int size)
{
    int i;
    // std::cout << "c2s size is : " << size << std::endl;
    std::string s = "";
    for (i = 0; i < size; i++) {
        s += a[i];
    }    
    return s;
}

// char * convertToChar256(std::string){
//     char[256] ret;
//     memset(&ret, 0, sizeof(ret));
//     std::cout << "^^^^^^\nRet is " << ret << "/n^^^^^^/n";
//     strcpy(char_array, s.c_str());

//     std::cout << "^^^^^^\nRet is " << ret << "/n^^^^^^/n";

// }

void replace_all(std::string& str, std::string find, std::string replace){
    int index;
    while((index = str.find(find)) != std::string::npos){
        str.replace(index, find.length(), replace);
    }
}

/**
 Creates a temp file, writes char array to it, then renames it to save_path
*/

int atomic_save(std::string full_path, std::string data){
    //return atomic_save_char(full_path, data.c_str());
    int fd;
    // std::cout << "Atomic_save temp_path " << std::endl;
    std::string temp_path = get_temp_path(full_path);
    // std::cout << "Atomic_save temp_path done " << std::endl;
    //Open file for write
    // std::cout << "Atomic_save Opening " << temp_path << std::endl;
    fd = ::open(temp_path.c_str(), O_CREAT | O_WRONLY | O_SYNC, 0777);
    if (fd < 0){return -1;}    
    // Write to temp file and close   
    // std::cout << "Atomic_save  The size of data is not important " <<  std::endl;
    if (write(fd, data.c_str(), data.length()) < 0){
        ::close(fd);
        return -1;
    }    
    // Force flush
    if (fsync(fd) < 0){
        ::close(fd);
        return -1;
    }
    // rename temp file true file name
    // std::cout << "Atomic_save about to rename" << std::endl;
    if (rename(temp_path.c_str(), full_path.c_str()) < 0){
        ::close(fd);
        return -1;
    }
    // std::cout << "A_S is returning FD " << fd << std::endl;
    return fd;
}

int atomic_save_char(std::string full_path, char* data){
    int fd;
    // std::cout << "Atomic_save temp_path " << std::endl;
    std::string temp_path = get_temp_path(full_path);
    // std::cout << "Atomic_save temp_path done " << std::endl;
    //Open file for write
    // std::cout << "Atomic_save Opening " << temp_path << std::endl;
    fd = ::open(temp_path.c_str(), O_CREAT | O_WRONLY | O_SYNC, 0777);
    if (fd < 0){return -1;}    
    // Write to temp file and close   
    // std::cout << "Atomic_save  The size of data is not important " <<  std::endl;
    if (write(fd, data, strlen(data)) < 0){
        ::close(fd);
        return -1;
    }    
    // Force flush
    if (fsync(fd) < 0){
        ::close(fd);
        return -1;
    }
    // rename temp file true file name
    // std::cout << "Atomic_save about to rename" << std::endl;
    // std::cout << "Atomic_save Renaming to " << full_path << std::endl;
    if (rename(temp_path.c_str(), full_path.c_str()) < 0){
        ::close(fd);
        return -1;
    }
    // std::cout << "A_S is returning FD " << fd << std::endl;
    return fd;
}
/**
 Should be named copy_file
 makes copy of cache_path and saves it as tmp_path
 returns FD on tmp_path

 Create a .tmp file to write to for added durability
 Easiest way to do this is open cache file, which needs
 to exist by this point, copy it into a string,  send
 it into atomic rename
*/

int build_tmp_file(std::string cache_path, std::string tmp_path){
    int fd;
    // open cache file
    fd = ::open(cache_path.c_str(), O_RDONLY, 0777);
    if (fd < 0){return -1;}  

    // get file deets and write to string
    int length = lseek(fd, 0, SEEK_END);
    if (length<0){return -1;}
    if (lseek(fd, 0, SEEK_SET) < 0) {return -1;}
    char *pChars = new char[length];
    int read_length = read(fd, pChars, length);
    
    // now have cache file as string, close cache file
    // and save data to temp location
    if (::close(fd) < 0){return -1;}
    fd = atomic_save_char(tmp_path, pChars);
    delete pChars;
    return fd;
}


/**
 get_temp_path()
 Generates temporary file name by adding date time to file name
*/
std::string get_temp_path(std::string userpath){
    //seperate out extension
    // int pos = userpath.rfind(".");
    // if (pos)
    // std::string ext = userpath.substr(pos, userpath.length()-pos);
    // userpath = userpath.substr(0, pos);

    // add current timestamp to name    
    userpath = userpath + get_time_str();
    // std::cout << "Temp path : " << userpath << std::endl;

    return userpath;
}

