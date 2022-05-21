/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#include <iostream>
#include "AFEsqClient.h"
#include <fcntl.h>

std::string CACHE_PATH = "../tmp/afs";
std::string SERVER_ADDY = "localhost";
bool is_durable = false;
const int MAIN_DEF_MODE = 0777;

/**
* Parse input args to get server and cache path
* path: -p pathname
* server: -i ip
**/
int parse_args(int argc, char** argv){    
    int arg = 1;
    std::string argx;
    while (arg < argc){
        if (argc < arg) return 0;
        argx = std::string(argv[arg]);
        if (argx == "-ip"){
            SERVER_ADDY = std::string(argv[arg+1]);
        } else if (argx == "-path") {
            CACHE_PATH = std::string(argv[arg+1]);
        } else if (argx == "-dur") {
            is_durable = true;
        } else {
            std::cout << "Usage: prog -ip <server IP> (default = 'localhost')\n" 
                      << "            -path <cache path> (default = '/temp/afs')\n";
                return 1;        
        }                
        arg += 2;
    }
    return 0;
}

/**
* pseudo-shell while we figure out exactly how the user interacts
**/
void shell(){
    AFEsqClient* afesq = new AFEsqClient(CACHE_PATH, SERVER_ADDY, is_durable);
    int fd;

    std::string input = "";
    std::string args = "";
    while (input != "quit"){
        //Print menu
        std::cout << "\n\n********\nMenu\n********" << std::endl
                  << "  open <path to file on server>" << std::endl                  
                  << "  close" << std::endl
                  << "  (p)write <text>" << std::endl
                  << "  stat <path to file on server>" << std::endl
                  << "  creat <path to new file on server>" << std::endl
                  << "  unlink <path to file on server>" << std::endl
                  << "  rmdir <path to new dir on server>" << std::endl
                  << "  mkdir <path to dir on server>" << std::endl
                  << "  server_test <delay (s)> <msg to show on server>" << std::endl
                  << "  quit" << std::endl; 
                   
        std::cout << "$ ";
        std::cin >> input;
        std::cout << std::endl;
        
        if (!std::cin){
            std::cin.clear();
            std::cin.ignore(10000,'\n');
        }

        std::getline(std::cin, args);
        if (args.length() > 1) args = args.substr(1, args.length()); 

        // Make server calls when necessary
        try{
            if (input == "open") {                           
                fd = afesq->open(args.c_str(), O_RDWR, MAIN_DEF_MODE); 
                if (fd < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("open() returned ");
                    std::cout << std::endl;
                } 
                // else {::close (fd);};       
            } else if (input == "close") {
                int resp = afesq->close(fd);
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("close() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "write") {
                int resp = afesq->write(fd, args.c_str(), args.length());
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("write() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "pwrite") {
                int resp = afesq->pwrite(fd, args.c_str(), args.length(), 0);
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("write() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "stat") {
                struct stat buf;
                int resp = afesq->stat(args.c_str(), &buf);
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("write() returned ");
                    std::cout << std::endl;                  
                }
                std::cout << buf.st_dev << std::endl;
                std::cout << buf.st_ino << std::endl;
                std::cout << buf.st_mode << std::endl;
                std::cout << buf.st_nlink << std::endl;
                std::cout << buf.st_uid << std::endl;
                std::cout << buf.st_gid << std::endl;
                std::cout << buf.st_rdev << std::endl;
                std::cout << buf.st_size << std::endl;
                std::cout << buf.st_blksize << std::endl;
                std::cout << buf.st_blocks << std::endl;
                std::cout << buf.st_atime << std::endl;
                std::cout << buf.st_mtime << std::endl;
                std::cout << buf.st_ctime << std::endl;
                std::cout << "End of stat" << std::endl;
            } else if (input == "creat") {
                fd = afesq->creat(args.c_str(), O_RDWR, MAIN_DEF_MODE);
                std::cout << "Returned FD = " << fd;
                if (fd < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("creat() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "unlink") {
                int resp = afesq->unlink(args.c_str());
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("unlink() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "mkdir") {
                int resp = afesq->mkdir(args.c_str(), MAIN_DEF_MODE);
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("mkdir() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "rmdir") {
                int resp = afesq->rmdir(args.c_str());
                if (resp < 0){
                    std::cout << "ErrNo: " << errno << std::endl;
                    perror("rmdir() returned ");
                    std::cout << std::endl;
                }
            } else if (input == "server_test"){
                std::cout << "About to call server_test_local" << std::endl;
                afesq->server_test_local(args);
            } else if (input != "quit"){
                std::cout << "\nCommand Not Recognized\n";
            }
        }catch (TException &tx){
            std::cout<<"Error: " <<tx.what() <<std::endl;
        }
    }
    delete afesq;
}

int main(int argc, char* argv[]){
    // Parse server args and verify
    if (parse_args(argc, argv) == 1) {return 0;}
    shell();
    return 0;
}