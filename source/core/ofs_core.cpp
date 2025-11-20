#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstring>
#include <vector>
#include "../../include/ofs_core.hpp"
using namespace std;

int fs_format(const char* omni_path, const char* config_path) {
    OMNIHeader header;
    memset(&header, 0, sizeof(header));
    strncpy(header.magic, "OMNIFS01", 8);
    header.format_version = 0x00010000;
    header.total_size = 104857600ULL;
    header.header_size = 512;
    header.block_size = 4096;
    strncpy(header.student_id, "BSAI-24005", sizeof(header.student_id)-1);
    strncpy(header.submission_date, "2025-11-13", sizeof(header.submission_date)-1);
    header.config_timestamp = (uint64_t)time(NULL);
    header.user_table_offset = (uint32_t)header.header_size;
    header.max_users = 50;

    ofstream ofs(omni_path, ios::binary | ios::trunc);
    if (!ofs.is_open()) {
        cout << "Cannot create " << omni_path << "\n";
        return -1;
    }
    ofs.write((char*)&header, sizeof(header));
    UserInfo empty;
    memset(&empty, 0, sizeof(UserInfo));

    UserInfo admin;
    memset(&admin, 0, sizeof(UserInfo));
    strncpy(admin.username, "admin", sizeof(admin.username)-1);
    // password set to 7861 as requested
    strncpy(admin.password_hash, "7861", sizeof(admin.password_hash)-1);
    admin.role = UserRole::ADMIN;
    admin.created_time = (uint64_t)time(NULL);
    admin.last_login = 0;
    admin.is_active = 1;

    ofs.seekp(header.user_table_offset, ios::beg);
    ofs.write((char*)&admin, sizeof(UserInfo));
    for (uint32_t i = 1; i < header.max_users; ++i) {
        ofs.write((char*)&empty, sizeof(UserInfo));
    }

    uint64_t user_table_size = (uint64_t)header.max_users * sizeof(UserInfo);
    uint64_t remaining = header.total_size - header.header_size - user_table_size;
    uint32_t nblocks = (uint32_t)(remaining / header.block_size);
    vector<char> free_map(nblocks, 0);
    ofs.write(free_map.data(), free_map.size());

    ofs.seekp((std::streamoff)header.total_size - 1, ios::beg);
    char zero = 0;
    ofs.write(&zero, 1);
    ofs.close();

    cout << "[fs_format] created " << omni_path << " size=" << header.total_size << " blocks=" << nblocks << "\n";
    return 0;
}

int fs_init(const char* omni_path) {
    ifstream ifs(omni_path, ios::binary);
    if (!ifs.is_open()) {
        cout << "Cannot open " << omni_path << "\n";
        return -1;
    }
    OMNIHeader header;
    ifs.read((char*)&header, sizeof(header));
    if (strncmp(header.magic, "OMNIFS01", 8) != 0) {
        cout << "Invalid omni file\n";
        ifs.close();
        return -1;
    }
    ifs.seekg(header.user_table_offset, ios::beg);
    UserInfo u;
    ifs.read((char*)&u, sizeof(UserInfo));
    if (ifs) {
        cout << "[fs_init] loaded 1 users, blocks=" << (header.total_size / header.block_size) << "\n";
    }
    ifs.close();
    return 0;
}

int verify_user(const char* username, const char* password) {
    ifstream ifs("compiled/sample.omni", ios::binary);
    if (!ifs.is_open()) return -1;
    OMNIHeader hdr;
    ifs.read((char*)&hdr, sizeof(hdr));
    if (strncmp(hdr.magic, "OMNIFS01", 8) != 0) {
        ifs.close();
        return -1;
    }
    ifs.seekg(hdr.user_table_offset, ios::beg);
    for (uint32_t i = 0; i < hdr.max_users; ++i) {
        UserInfo u;
        ifs.read((char*)&u, sizeof(u));
        if (!ifs) break;
        if (u.is_active) {
            string uname(u.username);
            if (uname == string(username)) {
                string stored(u.password_hash);
                if (stored == string(password)) {
                    ifs.close();
                    return 0;
                } else {
                    ifs.close();
                    return -1;
                }
            }
        }
    }
    ifs.close();
    return -1;
}

int get_stats(FSStats* out) {
    if (!out) return -1;
    ifstream ifs("compiled/sample.omni", ios::binary);
    if (!ifs.is_open()) return -1;
    OMNIHeader hdr;
    ifs.read((char*)&hdr, sizeof(hdr));
    ifs.close();
    if (strncmp(hdr.magic, "OMNIFS01", 8) != 0) return -1;
    out->total_size = hdr.total_size;
    out->used_space = 0;
    out->free_space = hdr.total_size - hdr.header_size;
    out->total_files = 0;
    out->total_directories = 0;
    out->total_users = hdr.max_users;
    out->active_sessions = 0;
    out->fragmentation = 0.0;
    return 0;
}