#include<iostream>
using namespace std;

struct Request {
    string operation;
    string data;
};

class MyQueue {
    Request* arr;
    int front;
    int rear;
    int size;
    int capacity;
public:
    MyQueue(int cap) {
        capacity=cap;
        arr=new Request[capacity];
        front=0;
        rear=-1;
        size=0;
    }

    bool isEmpty() {
        return size==0;
    }

    bool isFull() {
        return size==capacity;
    }

    void enqueue(Request r) {
        if(isFull()) {
            cout<<"Queue full"<<endl;
            return;
        }
        rear=(rear+1)%capacity;
        arr[rear]=r;
        size++;
    }

    Request dequeue() {
        Request temp;
        if(isEmpty()) {
            cout<<"Queue empty"<<endl;
            return temp;
        }
        temp=arr[front];
        front=(front+1)%capacity;
        size--;
        return temp;
    }

    Request peek() {
        if(isEmpty()) {
            Request temp;
            cout<<"Queue empty"<<endl;
            return temp;
        }
        return arr[front];
    }
};