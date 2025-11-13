#include<iostream>
using namespace std;

class FreeMap {
    bool* used;
    int total;
public:
    FreeMap(int n) {
        total=n;
        used=new bool[n];
        for(int i=0;i<n;i++) used[i]=false;
    }

    int allocate() {
        for(int i=0;i<total;i++) {
            if(!used[i]) {
                used[i]=true;
                return i;
            }
        }
        return -1;
    }

    void freeBlock(int idx) {
        if(idx>=0 && idx<total) used[idx]=false;
    }

    int getFreeCount() {
        int cnt=0;
        for(int i=0;i<total;i++) if(!used[i]) cnt++;
        return cnt;
    }
};