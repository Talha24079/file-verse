#include "fifo_queue.hpp"

void FifoQueue::push(Request req) {
    unique_lock<mutex> lock(mtx);
    q.push(req);
    lock.unlock();
    cv.notify_one();
}

Request FifoQueue::pop() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [this]{ return !q.empty(); });
    Request req = q.front();
    q.pop();
    return req;
}
