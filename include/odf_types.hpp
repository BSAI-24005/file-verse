#ifndef ODF_TYPES_HPP
#define ODF_TYPES_HPP

#include <cstdint>
#include <string>
#include <cstring>
using namespace std;

enum class UserRole : uint32_t {
    NORMAL = 0,
    ADMIN = 1
};

enum class OFSErrorCodes : int32_t {
    SUCCESS = 0,
    ERROR_NOT_FOUND = -1,
    ERROR_PERMISSION_DENIED = -2
};

struct OMNIHeader {
    char magic[8];
    uint32_t format_version;
    uint64_t total_size;
    uint64_t header_size;
    uint64_t block_size;
    char student_id[32];
    char submission_date[16];
    char config_hash[64];
    uint64_t config_timestamp;
    uint32_t user_table_offset;
    uint32_t max_users;
    uint32_t file_state_storage_offset;
    uint32_t change_log_offset;
    uint8_t reserved[328];

    OMNIHeader() {
        memset(this, 0, sizeof(OMNIHeader));
    }
};

struct UserInfo {
    char username[32];
    char password_hash[64];
    UserRole role;
    uint64_t created_time;
    uint64_t last_login;
    uint8_t is_active;
    uint8_t reserved[23];

    UserInfo() {
        memset(this, 0, sizeof(UserInfo));
    }

    UserInfo(string u, string pass, UserRole r, uint64_t time) {
        strncpy(username, u.c_str(), 31);
        strncpy(password_hash, pass.c_str(), 63);
        role = r;
        created_time = time;
        last_login = 0;
        is_active = 1;
        memset(reserved, 0, 23);
    }
};

#endif