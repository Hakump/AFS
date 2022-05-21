//
// Created by devbox on 3/11/22.
//

#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "../src/timer.h"
#include <sys/types.h>
#include <fcntl.h>
#include <string>
#include<list>
const std::string CACHE_PATH = "../../tmp/afs/";
const std::string MOUNT_PATH = "../../mnt/";
const int RPT_TESTS = 3;
const int IGNORE_COUNT = 1;
const int NUM_FILES = 3;
struct stat buf;



/***
 * Moves through pathway path and removes all files found within
 *
 * @param path - pathway to unlink files from
 */
void clear_cache(std::string path){
    DIR *dr = opendir(path.c_str());
    struct dirent* de;
    std::string full_path;

    while ((de = readdir(dr)) != NULL){
        full_path = path + de->d_name;
        if (!stat(full_path.c_str(), &buf)){
            if (!S_ISDIR(buf.st_mode)){
//                std::cout << "...Removing:  " << full_path << std::endl;
                unlink(full_path.c_str());
            }
        }
    }

    closedir(dr);
}

/**
 * Open file RPT_TESTS # of times, track lowest elapsed time on open
 * Clear cache in between to ensure we call server
 * @param path
 */
void test_open(std::string path){
    struct timespec start, end;
    double res, best, first = 0, tot_elapsed = 0;
    // Repeat test multiple times
    for (int i = 0; i < RPT_TESTS; i++){
        // Clear cache
        clear_cache(CACHE_PATH);
        //Measure latency to fetch file
        get_time(&start);
        int fd = open(path.c_str(), O_RDONLY);
        close(fd);
        // Get results
        get_time(&end);
        res = difftimespec_ns(start, end);
        if (i==0) first = res;
        if (i > IGNORE_COUNT-1) tot_elapsed += res;
        if (i==0 || res < best){
            best = res;
        }
    }
    // Calculate throughput
    // Want Mb/s from (file_size*(RPT_TESTS-1))/ns
    stat(path.c_str(), &buf);
    double throughput = ((buf.st_size)*(RPT_TESTS-IGNORE_COUNT))/tot_elapsed;
    throughput = throughput*1000;
    std::cout << path << "," <<  buf.st_size << "," << (first/1e6)  << "," << (best/1e6) << "," << throughput << std::endl;
}
/**
 * Goal: Test transfer latency and max file transfers
 * 1) Generate files on server - handled by gen_files.cpp
 * 2) Clear Cache
 * 3) Measure latency to fetch file
 * 4) repeat steps 2 & 3 X number of times
 */


int main(){
    // For each test iteration
    bool run_list = true;

    std::list<std::string> run_files;
    std::string base_path = MOUNT_PATH;
    // eventually generates base_path/_i.txt
//    std::string gen_temp = base_path + "suite_range_proto_"; //Change "val" To whatever template files saved as "test_small_" for "test_small_i.txt"
//    std::string gen_temp = base_path + "suite_range_MB_"; //Change "val" To whatever template files saved as "test_small_" for "test_small_i.txt"
    std::string gen_temp = base_path + "suite_range_GB_"; //Change "val" To whatever template files saved as "test_small_" for "test_small_i.txt"

    int num_files;
    if (run_list){ // Build list to run
//        run_files.push_back(base_path + "suite_range_proto_1.txt");
//        run_files.push_back(base_path + "suite_range_proto_2.txt");
        run_files.push_back(base_path + "suite_range_GB_3.txt");
        run_files.push_back(base_path + "suite_range_GB_4.txt");
        run_files.push_back(base_path + "suite_range_GB_5.txt");
        run_files.push_back(base_path + "suite_range_GB_6.txt");
//        run_files.push_back(base_path + "suite_range_proto_4.txt");
        num_files = run_files.size();
    } else { // Generates run list as <base_path>i.txt for NUM_FILES times
        for (int i = 1; i <= NUM_FILES; i++){
            run_files.push_back(gen_temp + std::to_string(i) + ".txt");
        }
        num_files = NUM_FILES;
    }
    // Output header
    std::cout << "Name, Size, First Time (ms), Best Elapsed Time (ms), Throughput" << std::endl;
    // run tests
    for (int i = 1; i <= num_files; i++){
        test_open(run_files.front());
        run_files.pop_front();
    }
}