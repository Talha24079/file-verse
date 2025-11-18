#include "config_parser.hpp"
#include <fstream>     
#include <sstream>     
#include <iostream>     
using namespace std;

bool parse_config(const string& config_path, Config& config) 
{
    ifstream config_file(config_path);
    if (!config_file.is_open()) {
        cerr << "Error: Could not open config file at " << config_path << endl;
        return false;
    }

    string line;
    while (getline(config_file, line)) 
    {
        size_t comment_pos = line.find('#');
        if (comment_pos != string::npos)
            line = line.substr(0, comment_pos);

        size_t separator_pos = line.find('=');
        if (separator_pos == string::npos)
            continue; 

        string key_str = line.substr(0, separator_pos);
        string val_str = line.substr(separator_pos + 1);

        string key, value;
        stringstream ss_key(key_str);
        ss_key >> key; 

        stringstream ss_val(val_str);
        ss_val >> value; 

        if (key.empty() || value.empty())
            continue; 
        
        if (value.length() >= 2 && value.front() == '"' && value.back() == '"') 
            value = value.substr(1, value.length() - 2);

        try {
            if (key == "total_size") config.total_size = stoull(value);
            else if (key == "header_size") config.header_size = stoull(value);
            else if (key == "block_size") config.block_size = stoull(value);
            else if (key == "max_files") config.max_files = stoi(value);
            else if (key == "max_filename_length") config.max_filename_length = stoi(value);
            else if (key == "max_users") config.max_users = stoi(value);
            else if (key == "admin_username") config.admin_username = value; 
            else if (key == "admin_password") config.admin_password = value; 
            else if (key == "require_auth") config.require_auth = (value == "true");
            else if (key == "port") config.port = stoi(value);
            else if (key == "max_connections") config.max_connections = stoi(value);
            else if (key == "queue_timeout") config.queue_timeout = stoi(value);
        } catch (const exception& e) {
            cerr << "Error parsing key '" << key << "' with value '" << value << "': " << e.what() << endl;
            return false;
        }
    }

    return true;
}