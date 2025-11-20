#ifndef OFS_CORE_HPP
#define OFS_CORE_HPP

#include <iostream>
#include <string>
#include "odf_types.hpp"

int fs_format(const char* omni_path, const char* config_path);
int fs_init(const char* omni_path);

int verify_user(const char* username, const char* password);
int get_stats(FSStats* out);

#endif