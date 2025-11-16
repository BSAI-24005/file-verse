#ifndef MY_QUEUE_HPP
#define MY_QUEUE_HPP

#include <string>
using namespace std;

struct Request {
    int client_fd;
    string json;
};

class TSQueue {
private:
    void *impl;
public:
    TSQueue(int cap);
    ~TSQueue();

    void enqueue(const Request &r);
    Request dequeue();
    bool empty();
};

#endif