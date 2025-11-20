#ifndef MY_QUEUE_HPP
#define MY_QUEUE_HPP

#include <string>
#include <condition_variable>
#include <mutex>

struct Request {
    int client_fd;
    std::string json;
};

class TSQueue {
    void* impl;
public:
    TSQueue(int cap = 1000);
    ~TSQueue();

    void enqueue(const Request &r);
    Request dequeue();
    bool empty();
};

#endif