#include<iostream>
#include<string>
#include "odf_types.hpp"
using namespace std;

struct UserTableEntry {
    string username;
    UserInfo user;
    bool used;
};

class UserTable {
    UserTableEntry* table;
    int size;
public:
    UserTable(int n) {
        size = n;
        table = new UserTableEntry[size];
        for(int i=0;i<size;i++) table[i].used=false;
    }

    int hash(string key) {
        int sum=0;
        for(char c : key) sum += c;
        return sum % size;
    }

    void insert(string name, UserInfo u) {
        int h = hash(name);
        for(int i=0;i<size;i++) {
            int idx = (h+i)%size;
            if(!table[idx].used) {
                table[idx].used=true;
                table[idx].username=name;
                table[idx].user=u;
                break;
            }
        }
    }

    UserInfo* find(string name) {
        int h=hash(name);
        for(int i=0;i<size;i++) {
            int idx=(h+i)%size;
            if(table[idx].used && table[idx].username==name)
                return &table[idx].user;
        }
        return NULL;
    }
};
