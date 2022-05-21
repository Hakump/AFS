#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <iostream>
#include <utime.h>

void getFileCreationTime(std::string path) {
    // Get stats from source
    struct stat attr, attr2;
    
    int n = path.length();
    char char_array[n + 1];
    strcpy(char_array, path.c_str());
    printf("Checking file %s\n", char_array);

    stat(char_array, &attr);
    printf("Last modified time: %ld\n", attr.st_mtime);
    printf("Last modified time: %s\n", ctime(&attr.st_mtime));

    // Set stats on new
    std::string new_path = "../tmp/afs/FunnyMonkey.png";
    int n2 = new_path.length();
    char char_array_2[n2 + 1];
    strcpy(char_array_2, new_path.c_str());
    printf("Checking file %s\n", char_array_2);
    fflush(stdout);

    struct utimbuf new_times;
    new_times.modtime = attr.st_mtime;
    new_times.actime = attr.st_atime;
    utime(char_array_2, &new_times);

    //Check times
    stat(char_array_2, &attr2);
    printf("Last modified time: %ld\n", attr2.st_mtime);
    printf("Last modified time: %s\n", ctime(&attr2.st_mtime));
}

int main(){
    getFileCreationTime("../srvfs/FunnyMonkey.png");
}