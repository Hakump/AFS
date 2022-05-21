#include <dirent.h>
#include <iostream>

int main(){
    DIR *dp;
	struct dirent *de;
    const char* true_path = "/users/jmadrek/p2/srvfs";
    dp = opendir(true_path);
    std::cout << "....about to start while loop\n";

	try {
		de = readdir(dp);
		std::cout << "just read " << de->d_name << "\n" << std::flush;
	} catch (const std::exception& e){
        std::cout << "e returned " << e.what() << std::endl;
		perror("...EXCEPTION:");
	}
    closedir(dp);
    return 0;
}
