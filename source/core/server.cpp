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
#include <map>

#include "../../include/server.hpp"
#include "../../include/ofs_core.hpp"
#include "../../include/my_queue.hpp"
#include "../../include/json_util.hpp"

using namespace std;

string process_command(const string &raw_json);

static TSQueue *gqueue = NULL;
static bool g_running = true;

static void send_json(int client_fd, const string &json) {
    string msg = json + "\n";
    send(client_fd, msg.c_str(), msg.size(), 0);
}

static void worker_thread_func() {
    while (g_running) {
        Request r = gqueue->dequeue();

        map<string,string> obj = parse_json_simple(r.json);
        string cmd = obj.count("cmd") ? obj["cmd"] : "";
        string rid = obj.count("request_id") ? obj["request_id"] : "0";
        string response;

        if (cmd == "login" || cmd == "user_login") {
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
        } else if (cmd == "logout" || cmd == "user_logout") {
            response = string("{\"status\":\"success\",\"operation\":\"logout\",\"request_id\":\"") + rid + "\"}";
        } else if (cmd == "exit") {
            response = string("{\"status\":\"success\",\"operation\":\"exit\",\"request_id\":\"") + rid + "\"}";
        } else if (cmd == "file_read") {
            string path = obj.count("path") ? obj["path"] : "";
            response = string("{\"status\":\"error\",\"operation\":\"file_read\",\"request_id\":\"") + rid + "\",\"error_code\":-8,\"error_message\":\"file_read not implemented in core\"}";
        } else if (cmd == "file_create") {
            response = string("{\"status\":\"error\",\"operation\":\"file_create\",\"request_id\":\"") + rid + "\",\"error_code\":-8,\"error_message\":\"file_create not implemented in core\"}";
        } else if (cmd == "dir_list") {
            string path = obj.count("path") ? obj["path"] : "/";
            response = string("{\"status\":\"success\",\"operation\":\"dir_list\",\"request_id\":\"") + rid + "\",\"data\":{\"path\":\"" + path + "\",\"entries\":[]}}";
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

void http_server_thread() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cout << "[HTTP] socket() failed\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9001);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "[HTTP] bind() failed\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        cout << "[HTTP] listen failed\n";
        close(server_fd);
        return;
    }

    cout << "[HTTP] UI server running on port 9001\n";

    while (true) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
        if (client_fd < 0) {
            continue;
        }

        char buffer[8192];
        ssize_t n = recv(client_fd, buffer, sizeof(buffer)-1, 0);
        if (n <= 0) {
            close(client_fd);
            continue;
        }
        buffer[n] = 0;
        string req(buffer);

        size_t firstLineEnd = req.find("\r\n");
        string firstLine = (firstLineEnd != string::npos) ? req.substr(0, firstLineEnd) : "";
        bool isPost = (firstLine.find("POST ") == 0);
        size_t pos = req.find("\r\n\r\n");
        string body = "";
        if (pos != string::npos) {
            body = req.substr(pos + 4);
        }

        if (!isPost) {
            string notAllowed = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length:0\r\n\r\n";
            send(client_fd, notAllowed.c_str(), notAllowed.size(), 0);
            close(client_fd);
            continue;
        }

        string json_body = body;
        string resp_json;
        try {
            resp_json = process_command(json_body);
            Request r; r.client_fd = client_fd; r.json = json_body;
            gqueue->enqueue(r);
        } catch (...) {
            resp_json = "{\"status\":\"error\",\"error_message\":\"internal\"}";
        }

        if (resp_json.empty()) {
            resp_json = "{\"status\":\"success\",\"message\":\"queued\"}";
        }
        std::ostringstream http;
        http << "HTTP/1.1 200 OK\r\n";
        http << "Content-Type: application/json\r\n";
        http << "Access-Control-Allow-Origin: *\r\n";
        http << "Content-Length: " << resp_json.size() << "\r\n";
        http << "\r\n";
        http << resp_json;

        string out = http.str();
        send(client_fd, out.c_str(), out.size(), 0);
        close(client_fd);
    }

    close(server_fd);
}

string process_command(const string &raw_json) {
    map<string,string> obj = parse_json_simple(raw_json);
    string cmd = obj.count("cmd") ? obj["cmd"] : "";
    string rid = obj.count("request_id") ? obj["request_id"] : "0";
    string response;

    if (cmd == "login" || cmd == "user_login") {
        string u = obj.count("username") ? obj["username"] : "";
        string p = obj.count("password") ? obj["password"] : "";
        int ok = verify_user(u.c_str(), p.c_str());
        if (ok == 0) {
            response = string("{\"status\":\"success\",\"operation\":\"login\",\"request_id\":\"") + rid + "\",\"data\":{\"message\":\"logged_in\",\"session_id\":\"sess_" + u + "\"}}";
        } else {
            response = string("{\"status\":\"error\",\"operation\":\"login\",\"request_id\":\"") + rid + "\",\"error_code\":-2,\"error_message\":\"Invalid credentials\"}";
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
    } else if (cmd == "whoami") {
        string sid = obj.count("session_id") ? obj["session_id"] : "";
        if (sid == "") {
            response = string("{\"status\":\"error\",\"operation\":\"whoami\",\"request_id\":\"") + rid + "\",\"error_code\":-9,\"error_message\":\"no session\"}";
        } else {
            response = string("{\"status\":\"success\",\"operation\":\"whoami\",\"request_id\":\"") + rid + ("\",\"data\":{\"session_id\":\"") + sid + "\"}}";
        }
    } else if (cmd == "dir_list") {
        string path = obj.count("path") ? obj["path"] : "/";
        response = string("{\"status\":\"success\",\"operation\":\"dir_list\",\"request_id\":\"") + rid + "\",\"data\":{\"path\":\"" + path + "\",\"entries\":[]}}";
    } else {
        response = string("{\"status\":\"error\",\"operation\":\"unknown\",\"request_id\":\"") + rid + "\",\"error_message\":\"unknown command\"}";
    }
    return response;
}

void start_server(const char* omni_path, int port) {
    fs_init(omni_path);

    gqueue = new TSQueue(1000);
    thread worker(worker_thread_func);
    worker.detach();

    thread http(http_server_thread);
    http.detach();

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