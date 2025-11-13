#include<iostream>
#include<string>
using namespace std;

struct DirNode {
    string name;
    bool isDir;
    DirNode* parent;
    DirNode* children[20];
    int childCount;

    DirNode(string n,bool d) {
        name=n;
        isDir=d;
        parent=NULL;
        childCount=0;
    }

    void addChild(DirNode* node) {
        if(childCount<20) {
            children[childCount++]=node;
            node->parent=this;
        }
    }

    DirNode* findChild(string n) {
        for(int i=0;i<childCount;i++) {
            if(children[i]->name==n)
                return children[i];
        }
        return NULL;
    }
};