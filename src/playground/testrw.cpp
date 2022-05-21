
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>

std::string in_full_path = "../srvfs/FunnyMonkey.png";
std::string out_full_path = "../srvfs/test.png";
// std::string in_full_path = "../srvfs/simple_test.txt";
// std::string out_full_path = "../srvfs/test.txt";


std::string convertToString(char* a, int size)
{
    int i;
    std::string s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    
    std::cout << "String:\n" << s << ":" << std::endl << std::endl;
    printf("char array:\n%s:\n\n", a);
    return s;
}

std::string ReadFile(){
    // Read file
    
    std::ifstream file(in_full_path, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = file.tellg();
    int length = pos;
    char *pChars = new char[length];
    file.seekg(0, std::ios::beg);
    file.read(pChars, length);
    file.close();

    // save as string
    std::cout << "Length pChars = " << length << std::endl;
    std::string str = ""; 
    str = convertToString(pChars, length);
    std::cout << "Length str = " << str.length() << std::endl;
    return str;
}

void WriteFile(std::string str){
    // write
    // Save file
    std::cout << "Getting n" << std::flush;
    int n = str.length();
    std::cout << "Allocating pChars size " << n << std::endl << std::flush;
    char pChars[n + 1];
    std::cout << "Copy string to pChars\n" << std::flush;
    //strcpy(pChars, str.c_str());
    for (int i = 0; i < n; i++){
        pChars[i] = str[i];
    }

    std::cout << "Done with copy\n"  << std::flush;


    std::ofstream myfile;
    std::string full_path;
    myfile.open(out_full_path, std::ios_base::binary);
    if (!myfile.is_open()){
        std::cout << "Unable to open client fd" << std::endl;
    }
    //myfile.write((char *)&str, length);    
    std::cout << "About to write file\n" << std::flush;
    myfile.write(pChars, n);
    std::cout << "File written\n" << std::flush;
    //myfile << str;
    myfile.close();
}

int main(){
    std::string str;
    std::cout << "Reading File\n";
    str = ReadFile();
    std::cout << "Writing File\n";
    WriteFile(str);
    return 0;
}