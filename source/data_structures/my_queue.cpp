#include "../../include/my_queue.hpp"
#include <mutex>
#include <condition_variable>

using namespace std;

struct TSQImpl {
    Request* arr;
    int capacity;
    int head;
    int tail;
    int count;
    std::mutex m;
    std::condition_variable cv;
    TSQImpl(int cap) {
        capacity = cap;
        arr = new Request[capacity];
        head = 0; tail = 0; count = 0;
    }
    ~TSQImpl() { delete[] arr; }
};

TSQueue::TSQueue(int cap) {
    impl = (void*) new TSQImpl(cap);
}

TSQueue::~TSQueue() {
    TSQImpl* p = (TSQImpl*)impl;
    delete p;
}

void TSQueue::enqueue(const Request &r) {
    TSQImpl* p = (TSQImpl*)impl;
    std::unique_lock<std::mutex> lock(p->m);
    while (p->count == p->capacity) {
        p->cv.wait(lock);
    }
    p->arr[p->tail] = r;
    p->tail = (p->tail + 1) % p->capacity;
    p->count++;
    p->cv.notify_all();
}

Request TSQueue::dequeue() {
    TSQImpl* p = (TSQImpl*)impl;
    std::unique_lock<std::mutex> lock(p->m);
    while (p->count == 0) {
        p->cv.wait(lock);
    }
    Request tmp = p->arr[p->head];
    p->head = (p->head + 1) % p->capacity;
    p->count--;
    p->cv.notify_all();
    return tmp;
}

bool TSQueue::empty() {
    TSQImpl* p = (TSQImpl*)impl;
    std::unique_lock<std::mutex> lock(p->m);
    return p->count == 0;
}