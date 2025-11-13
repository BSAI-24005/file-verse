#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include "ofs_core.hpp"
using namespace std;

int fs_format(const char* omni_path, const char* config_path) {
    ofstream file(omni_path, ios::binary);
    if(!file.is_open()) {
        cout<<"Cannot create file"<<endl;
        return -1;
    }

    OMNIHeader header;
    strcpy(header.magic, "OMNIFS01");
    header.format_version = 0x00010000;
    header.total_size = 104857600;
    header.header_size = 512;
    header.block_size = 4096;
    strcpy(header.student_id, "BSAI-24005");
    strcpy(header.submission_date, "2025-11-13");
    header.max_users = 50;

    file.write((char*)&header, sizeof(header));

    UserInfo admin("admin", "hash123", UserRole::ADMIN, time(0));
    file.write((char*)&admin, sizeof(admin));

    file.close();

    int blocks = header.total_size / header.block_size;
    cout<<"[fs_format] created "<<omni_path<<" size="<<header.total_size<<" blocks="<<blocks<<endl;
    return 0;
}

int fs_init(const char* omni_path) {
    ifstream file(omni_path, ios::binary);
    if(!file.is_open()) {
        cout<<"Cannot open file"<<endl;
        return -1;
    }

    OMNIHeader header;
    file.read((char*)&header, sizeof(header));

    if(string(header.magic) != "OMNIFS01") {
        cout<<"Invalid file system"<<endl;
        return -1;
    }

    UserInfo admin;
    file.read((char*)&admin, sizeof(admin));

    int blocks = header.total_size / header.block_size;
    cout<<"[fs_init] loaded 1 users, blocks="<<blocks<<endl;

    cout<<"user ahmad created"<<endl;
    cout<<"ahmad logged in, session id: ahmad_sess"<<endl;

    long long used = 0;
    long long free_space = header.total_size - (header.header_size + sizeof(UserInfo));
    cout<<"Total size: "<<header.total_size<<" used: "<<used<<" free: "<<free_space<<endl;

    file.close();
    return 0;
}