//Including C++ Headers
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include "timer.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <utime.h>

//Boost libraries
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

//Thrift libraries
#include <thrift/protocol/TBinaryProtocol.h>             
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TSocket.h>                    
#include <thrift/transport/TBufferTransports.h>          
#include <thrift/transport/TTransportUtils.h>

//Including the thrift generated files 
#include "gen-cpp/AFesqueSvc.h"
#include "gen-cpp/AFesque_types.h"


//Namespaces
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using boost::make_shared;

// Globals - Almost const
std::string CACHE_PATH = "../tmp/afs";
std::string SERVER_ADDY = "localhost";

// Other globals
AFesqueSvcClient* client;



/*########
# Related to Open
*########*/

std::string get_full_path(std::string userpath){
    int pos = userpath.find("/",0);
    if (pos == 0){
        userpath = userpath.substr(1, userpath.length()-1);
    }
    std::string delay_str = userpath.substr(0, pos);
    return CACHE_PATH + "/" + userpath;
}

std::string get_temp_path(std::string userpath){
    int pos = userpath.find("/",0);
    if (pos == 0){
        userpath = userpath.substr(1, userpath.length()-1);
    }
    // std::cout << "user path : " << userpath << std::endl;
    pos = userpath.rfind(".");
    std::string ext = userpath.substr(pos, userpath.length()-pos);
    userpath = userpath.substr(0, pos);
    std::cout << "ext : " << ext << std::endl;

    // add current timestamp to name
    struct timespec current_time;
    get_time(&current_time);
    userpath = CACHE_PATH + "/" + userpath + "_" + get_time_str(current_time) + ext;
    // std::cout << "Temp path : " << userpath << std::endl;

    return userpath;
}

// /***
// * Get copy of file from server, save to cache
// ***/
// int fetch_to_cache(std::string userpath){

//     // Get file from server and check for errors
//     struct FetchNode qry_result;
//     client->Fetch(qry_result, userpath);
//     if (qry_result.response == Response::ERR){
//         std::cerr << qry_result.file << std::endl;
//         return -1;
//     } 
    
//     // Open file for write and check for errors
//     std::ofstream myfile;
//     std::string temp_path = get_temp_path(userpath);
//     myfile.open(temp_path, std::ios_base::binary);
//     if (!myfile.is_open()){
//         std::cerr << "ERROR: Client unable to open file" << std::endl;
//         return -2;
//     }    
//     // Write to temp file and close
    
//     myfile.write(qry_result.file.c_str(), qry_result.file.length());
//     // myfile.write(pChars, qry_result.file.length());
//     myfile.close();

//     //rename file
//     std::string full_path = get_full_path(userpath);
//     if (rename(temp_path.c_str(), full_path.c_str()) != 0){
//         std::cerr << "ERROR: Rename operation failed";
//         return -3;
//     }
//     std::cout << "File is now in local cache." << std::endl;

//     return 0;

// }

/**
* Follow Open() protocol
* a) Check and see if file is on server, if so get last modified
* b) if local file does not exist, or does exist but is older 
*    than server, pull down new version from server
* c) If local file does exist, and is >= server version, user local file
* Ends with correct version of file in local cache
* should then be opened
***/
int open(std::string userpath){    
    
    struct stat local_stat;
    bool need_to_fetch = false;

    // Need server file stat data either way
    struct FileStat remote_stat;
    client->GetFileStat(remote_stat, userpath);
    
    // Check if file on server
    if (remote_stat.response == Response::ERR){
        std::cerr << "Error: File not present on server" << std::endl;
        return -1;
    }

    // Check and see if file is in cache
    std::string full_path = get_full_path(userpath);
    if (stat(full_path.c_str(), &local_stat) < 0 ||
        local_stat.st_mtime < remote_stat.st_mtimes) {            
        // file does not exist, need to fetch
        //std::cout << "Local file does not exist or is stale" << std::endl;        
        need_to_fetch = true;
    }     

    // std::cout << "local_stat = " << local_stat.st_mtime << std::endl
    //           << "remote_stat = " << remote_stat.st_mtimes << std::endl;
    
    // If needed create version in local cache.
    if (need_to_fetch){
        // Get file from server and check for errors
        struct FetchNode qry_result;
        client->Fetch(qry_result, userpath);
        if (qry_result.response == Response::ERR){
            std::cerr << "Error: " << qry_result.file << std::endl;
            return -2;
        } 
        
        // Open file for write and check for errors
        std::ofstream myfile;
        std::string temp_path = get_temp_path(userpath);
        myfile.open(temp_path, std::ios_base::binary);
        if (!myfile.is_open()){
            std::cerr << "ERROR: Client unable to open file" << std::endl;
            return -3;
        }    

        // Write to temp file and close        
        myfile.write(qry_result.file.c_str(), qry_result.file.length());
        // myfile.write(pChars, qry_result.file.length());
        myfile.close();

        // atomically rename file
        if (rename(temp_path.c_str(), full_path.c_str()) != 0){
            std::cerr << "ERROR: Rename operation failed";
            return -4;
        }
        std::cout << "File is now in local cache." << std::endl;
    }
    
    // Now have file locally
    // Open file
    // Not my problem bruh.
    return 0;
}

/***
* Can be used to test concurrenncy
****/
void server_test_local(std::string args){
    int pos = args.find(" ",0);
    std::string delay_str = args.substr(0, pos);
    std::string msg = args.substr(pos+1, args.length());
    int delay = std::stoi(delay_str);
    client->server_test(msg, delay);
    std::cout << "Test completed successfully" << std::endl;
}

/*########
# Related to main
*########*/


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
        } else {
            std::cout << "Usage: prog -ip <server IP> (default = 'localhost')\n" 
                      << "            -path <cache path> (default = '/temp/afs')\n";
                return 1;        
        }                
        arg += 2;
    }
    return 0;
}



//std::shared_ptr<TTransport> socket;
// std::shared_ptr<TTransport> transport;

/**
* pseudo-shell while we figure out exactly how the user interacts
**/
void shell(){
     // Setup Thrift socket
    std::shared_ptr<TTransport> socket(new TSocket(SERVER_ADDY, 9090));

    //socket = std::make_shared<TTransport>(new TSocket(SERVER_ADDY, 9090));
    std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    client = new AFesqueSvcClient(protocol);

    std::string input = "";
    std::string args = "";
    while (input != "quit"){
        //Print menu
        std::cout << "\n********\nMenu\n********" << std::endl
                  << "  open <path to file on server>" << std::endl
                  << "  server_test <delay (s)> <msg to show on server>" << std::endl
                  << "  quit" << std::endl; 
                   
        std::cout << "$ ";
        std::cin >> input;
        if (!std::cin){
            std::cin.clear();
            std::cin.ignore(10000,'\n');
        }

        std::getline(std::cin, args);
        if (args.length() > 1) args = args.substr(1, args.length()); 

        // Make server calls when necessary
        try{
            transport->open();
            if (input == "open") {            
                open(args);        
            } else if (input == "server_test"){
                server_test_local(args);
            }
            transport->close();
        }catch (TException &tx){
            std::cout<<"Error: " <<tx.what() <<std::endl;
        }
    }
}

int main(int argc, char* argv[]){
    // Parse server args and verify
    if (parse_args(argc, argv) == 1) {return 0;}
    shell();
    // // Setup Thrift socket
    // std::shared_ptr<TTransport> socket(new TSocket(SERVER_ADDY, 9090));
    // std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    // std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    // client = new AFesqueSvcClient(protocol);

    // try{
    //     transport->open();
    //     shell();
    //     transport->close();
    // }catch (TException &tx){
    //   std::cout<<"Error: " <<tx.what() <<std::endl;
    // }
}