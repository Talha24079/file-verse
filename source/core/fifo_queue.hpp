#pragma once
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include "../include/json.hpp"

using json = nlohmann::json;
using namespace std;

struct Request {
    int client_socket;
    json data;
};

class FifoQueue {
private:
    queue<Request> q;
    mutex mtx;
    condition_variable cv;

public:
    void push(Request req);
    Request pop();
};