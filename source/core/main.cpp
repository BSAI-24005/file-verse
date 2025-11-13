#include<iostream>
#include "ofs_core.hpp"
using namespace std;

int main() {
    cout<<"Formatting filesystem..."<<endl;
    fs_format("compiled/sample.omni","compiled/default.uconf");
    cout<<"Initializing..."<<endl;
    fs_init("compiled/sample.omni");
    cout<<"Done!"<<endl;
    return 0;
}