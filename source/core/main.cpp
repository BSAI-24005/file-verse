#include <iostream>
#include "ofs_core.hpp"
#include "server.hpp"
using namespace std;

int main() {
    fs_format("compiled/sample.omni", "compiled/default.uconf");
    fs_init("compiled/sample.omni");
    start_server("compiled/sample.omni", 8080);
    return 0;
}