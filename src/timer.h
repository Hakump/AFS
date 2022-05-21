/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#ifndef MY_TIMER_H
#define MY_TIMER_H
#include <time.h>
#include <iostream>

const int one_ns = 1e9;
void get_time(struct timespec* ts);
double difftimespec_ns(const struct timespec before, const struct timespec after);
double difftimespec_s(const struct timespec before, const struct timespec after);
std::string get_time_str();
std::string get_temp_path(std::string userpath);
#endif