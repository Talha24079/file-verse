#include "ofs_server.hpp"

using namespace std;

int main() {
    OFSServer server(8080);
    server.start();
    return 0;
}