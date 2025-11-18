#pragma once

#include <string>
#include <vector>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../core/fifo_queue.hpp"

using namespace std;

class OFSServer {
public:
    OFSServer(int port);
    ~OFSServer();
    void start();

private:
    int port;
    int server_fd;
    void* fs_instance;
    FifoQueue request_queue;

    void initFileSystem();
    void setupSocket();
    void listenLoop();
    void processorLoop();
    void handleClientConnection(int client_socket);
    string processRequest(json req_data);
};