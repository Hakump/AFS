#include <fstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

const std::string file_path = "../../../srvfs/test1.txt";

using namespace std;

void test1(){
    string str; 
	fstream file; 
	file.open(file_path); //text file i.e.(rj.txt) 
	while(!file.eof()){ 
		file>>str; 
		cout<< str<<endl; 
	} 
}

void test2(){
    int fd = open(file_path.c_str(), O_RDWR);
    char buf[255];
    read(fd, &buf, 255);
    printf("\n\n%s\n", buf);
    close(fd);
}

int main(){
    test1();
    test2();

  return 0;
}