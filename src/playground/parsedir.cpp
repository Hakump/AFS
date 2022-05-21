
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

// void check_dir(std::string path){
//   struct stat buffer;
//   std::vector<std::string> path_vec;
//   // get rid of file, see if anything left
//   int pos = path.rfind("/",0);
//   if (pos == std::string::npos || pos == 0){return;} 
//   // remaining path
//   path = path.substr(0, pos-1);  
//   std::cout << "Path is " << path; 


//   // iterate through pathway checking
//   // build list until we have valid path or at 
//   int resp;
  
//   // Path does not exist, lets creat it
//   while(true){
//     resp = stat(path.c_str(), &buffer);
//     if (resp<0 && errno == 2){        
//       perror("Check_Dir");
//       int pos = path.rfind("/",0);
//       if (pos == std::string::npos || pos == 0){break;} 
//       path_vec.push_back(path.substr(pos, path.length()));
//       path = path.substr(0, pos); 
//       std::cout << "Unable to find " << path_vec[0] << std::endl;         
//     }
//   }
        
// }

void check_dir_rec(std::string path, bool start){
    struct stat sb;
    // std::cout << "Checking " << path << std::endl;
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
          std::cout << "mkdir: " << path << std::endl;  
          // mkdir(path.c_str(), 0777);
        }                
    }     
}


int main(){
    std::string path = "/usrs/jmadrek/p2/srvfs2/test1/test2/test1.txt";
    check_dir_rec(path, true);    
}