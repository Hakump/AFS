/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#ifndef SHARED_LOCAL_H
#define SHARED_LOCAL_H

#include <string>


std::string convertToString(const char* a, int size);
void replace_all(std::string& str, std::string find, std::string replace);
int atomic_save(std::string full_path, std::string data);
int build_tmp_file(std::string cache_path, std::string tmp_path);
#endif