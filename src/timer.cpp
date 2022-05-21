/***************************************
* Author: Todd Hayes-Birchler
* CS739 - Distributed Systems
* 3/5/22
***************************************/

#include "timer.h"

#include <sys/time.h>
#include <string>

void get_time(struct timespec* ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

std::string get_time_str(){
    struct timespec ts;
    get_time(&ts);
    long tmstr;
    tmstr = (long)ts.tv_nsec;
    return std::to_string(tmstr);
}

double difftimespec_ns(const struct timespec before, const struct timespec after)
{
    return ((double)after.tv_sec - (double)before.tv_sec) * (double)1000000000
         + ((double)after.tv_nsec - (double)before.tv_nsec);
}

double difftimespec_s(const struct timespec before, const struct timespec after)
{
    return ((double)after.tv_sec - (double)before.tv_sec);
}