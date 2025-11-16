#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <cstring>

#include "../../include/server.hpp"
#include "../../include/ofs_core.hpp"
#include "../../include/my_queue.hpp"
#include "../../include/json_util.hpp"

using namespace std;

static TSQueue *gqueue = NULL;
static bool g_running = true;

static void send_json(int client_fd, const string &json) {
    string msg = json + "\n";
    ssize_t n = send(client_fd, msg.c_str(), msg.size(), 0);
    (void)n;
}

static void worker_thread_func() {
    while (g_running) {
        Request r = gqueue->dequeue();
        map<string,string> obj = parse_json_simple(r.json);
        string cmd = obj.count("cmd") ? obj["cmd"] : "";
        string rid = obj.count("request_id") ? obj["request_id"] : "0";
        string response;

        if (cmd == "login") {
            string u = obj.count("username") ? obj["username"] : "";
            string p = obj.count("password") ? obj["password"] : "";
            int ok = verify_user(u.c_str(), p.c_str());
            if (ok == 0) {
                response = string("{\"status\":\"success\",\"operation\":\"login\",\"request_id\":\"") + rid + "\",\"data\":{\"message\":\"logged_in\",\"session_id\":\"sess_" + u + "\"}}";
            } else {
                response = string("{\"status\":\"error\",\"operation\":\"login\",\"request_id\":\"") + rid + "\",\"error_code\":-2,\"error_message\":\"Invalid credentials\"}";
            }
        } else if (cmd == "whoami") {
            string sid = obj.count("session_id") ? obj["session_id"] : "";
            if (sid == "") {
                response = string("{\"status\":\"error\",\"operation\":\"whoami\",\"request_id\":\"") + rid + "\",\"error_code\":-9,\"error_message\":\"no session\"}";
            } else {
                response = string("{\"status\":\"success\",\"operation\":\"whoami\",\"request_id\":\"") + rid + ("\",\"data\":{\"session_id\":\"") + sid + "\"}}";
            }
        } else if (cmd == "stats") {
            FSStats st;
            if (get_stats(&st) == 0) {
                ostringstream ss;
                ss << "{\"status\":\"success\",\"operation\":\"stats\",\"request_id\":\"" << rid << "\",\"data\":{\"total_size\":" << st.total_size << ",\"used_space\":" << st.used_space << ",\"free_space\":" << st.free_space << "}}";
                response = ss.str();
            } else {
                response = string("{\"status\":\"error\",\"operation\":\"stats\",\"request_id\":\"") + rid + "\",\"error_message\":\"cannot get stats\"}";
            }
        } else if (cmd == "logout") {
            response = string("{\"status\":\"success\",\"operation\":\"logout\",\"request_id\":\"") + rid + "\"}";
        } else if (cmd == "exit") {
            response = string("{\"status\":\"success\",\"operation\":\"exit\",\"request_id\":\"") + rid + "\"}";
        } else {
            response = string("{\"status\":\"error\",\"operation\":\"unknown\",\"request_id\":\"") + rid + "\",\"error_message\":\"unknown command\"}";
        }

        send_json(r.client_fd, response);
    }
}

static void client_reader(int client_fd) {
    const int BUF = 4096;
    char buffer[BUF];
    string partial = "";
    while (true) {
        ssize_t bytes = recv(client_fd, buffer, BUF-1, 0);
        if (bytes <= 0) {
            close(client_fd);
            return;
        }
        buffer[bytes] = 0;
        partial += string(buffer);
        size_t pos;
        while ((pos = partial.find('\n')) != string::npos) {
            string line = partial.substr(0, pos);
            partial.erase(0, pos+1);
            if (line.size() == 0) continue;
            Request req;
            req.client_fd = client_fd;
            req.json = line;
            gqueue->enqueue(req);
        }
    }
}

void start_server(const char* omni_path, int port) {
    fs_init(omni_path);

    gqueue = new TSQueue(1000);
    thread worker(worker_thread_func);
    worker.detach();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cout << "socket() failed\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "bind failed\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 20) < 0) {
        cout << "listen failed\n";
        close(server_fd);
        return;
    }

    cout << "OFS server listening on port " << port << "\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
        if (client_fd < 0) {
            cout << "accept failed\n";
            continue;
        }
        thread t(client_reader, client_fd);
        t.detach();
    }

    close(server_fd);
}