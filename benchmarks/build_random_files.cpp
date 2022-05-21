/**
* Author: Todd Hayes-Birchler
* Generate files of varying sizes to test server transfer
* Populate files randomly with ASCII
*/

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string>


const std::string PATH = "../../srvfs/";
#define kb 1e3
#define mb 1e6
#define gb 1e9


void gen_file(std::string f_name, long start_val, int num_files, long mult = 1, long add = 0 ){
    std::string file_path;
    std::ofstream out;
    srand(time(NULL));


    //  Build each file
    long f_size = start_val;
    for (int i = 1; i <= num_files; i++){
        file_path = PATH + f_name + std::to_string(i) + ".txt";
        std::cout << "(" << file_path << ") is " << f_size << std::endl;
        out.open(file_path, std::ofstream::out);
        for (long j = 0; j < f_size; j++) out << ((char) (rand() % 94 + 32)); //32 - 126
        out.close();
        f_size *= mult;
        f_size += add;
    }
}
int main(){
//    gen_file("Test_Range", 1000, 710, 10, 0 ); // 1Kb - 1Gb
//    gen_file("Test_Large", 4000000000, 4, 1 , 2000000000 ); // 1Kb - 1Gb
//    gen_file("tst_med_", 200000000, 1,1, 0);

    // Test suite range for proto
//    gen_file("suite_range_proto_", 25000000, 5,1, 25000000);
//    gen_file("suite_range_KB_", 0, 4,1, 250*kb);
//    gen_file("suite_range_KB_", 125*kb, 1,1, 0);
    gen_file("suite_range_MB_", 0, 4,1, 250*mb);
    gen_file("suite_range_MB_", 125*mb, 1,1, 0);
//    gen_file("suite_range_GB_", 0, 6,1, 2*gb);
//    gen_file("suite_range_GB_", 1*gb, 1,1, 0);
//    gen_file("suite_range_GB_Largest_", 20*gb, 1,1, 0);

}