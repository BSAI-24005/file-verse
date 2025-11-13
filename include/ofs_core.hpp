#ifndef OFS_CORE_HPP
#define OFS_CORE_HPP

#include<iostream>
#include<string>
#include "odf_types.hpp"
using namespace std;

int fs_format(const char* omni_path, const char* config_path);
int fs_init(const char* omni_path);
int user_create(const char* username, const char* password);
int user_login(const char* username, const char* password);
int user_logout(const char* username);

#endif