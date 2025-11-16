#ifndef OFS_CORE_HPP
#define OFS_CORE_HPP

#include <iostream>
#include <string>
#include "odf_types.hpp"
using namespace std;

struct FSStats {
    uint64_t total_size;
    uint64_t used_space;
    uint64_t free_space;
    uint32_t total_files;
    uint32_t total_directories;
    uint32_t total_users;
    uint32_t active_sessions;
    double fragmentation;
};

int fs_format(const char* omni_path, const char* config_path);
int fs_init(const char* omni_path);

int verify_user(const char* username, const char* password);
int get_stats(FSStats* out);

#endif