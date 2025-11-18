#pragma once

#include <string>
#include <vector>
#include <cstdint>
using namespace std; 

struct Config {
    uint64_t total_size;
    uint64_t header_size;
    uint64_t block_size;
    int max_files;
    int max_filename_length;

    int max_users;
    string admin_username;
    string admin_password;
    bool require_auth;

    int port;
    int max_connections;
    int queue_timeout;
};

bool parse_config(const string& config_path, Config& config);
