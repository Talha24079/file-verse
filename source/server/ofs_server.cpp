#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <sstream>
#include <fstream>
#include <time.h>
#include <stdlib.h> 
#include <arpa/inet.h>
#include <functional>

#include "ofs_server.hpp"
#include "../core/ofs_api.hpp"
#include "../include/json.hpp"

using json = nlohmann::json;
using namespace std;

OFSServer::OFSServer(int p) : port(p), server_fd(0), fs_instance(nullptr) {
    srand(time(NULL));
    initFileSystem();
    setupSocket();
}

OFSServer::~OFSServer() {
    if (fs_instance) {
        fs_shutdown(fs_instance);
    }
    if (server_fd) {
        close(server_fd);
    }
}

void OFSServer::initFileSystem() {
    const string config_path = "./default.uconf";
    const string omni_path = "./my_filesystem.omni";

    cout << "--- Initializing File System ---" << endl;
    
    ifstream f(omni_path);
    bool file_exists = f.good();
    f.close();

    if (!file_exists) {
        cout << "File not found. Formatting new file system..." << endl;
        int format_result = fs_format(omni_path, config_path);
        if (format_result != (int)OFSErrorCodes::SUCCESS) {
            cerr << "FATAL: Could not format file system. Exiting." << endl;
            exit(1);
        }
    }
    
    if (fs_init(&fs_instance, omni_path, config_path) != 0) {
        cerr << "FATAL: Could not initialize file system. Exiting." << endl;
        exit(1);
    }
    
    if (port == 0) {
        port = ((OFSInstance*)fs_instance)->config.port;
        if (port == 0) port = 8080;
    }
}

void OFSServer::setupSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "Failed to create socket" << endl;
        exit(1);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "setsockopt failed" << endl;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        cerr << "Failed to set server address" << endl;
        exit(1);
    }

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Failed to bind to port " << port << endl;
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        cerr << "Failed to listen on socket" << endl;
        exit(1);
    }

    cout << "--- Server is running and listening on port " << port << " ---" << endl;
}

void OFSServer::start() {
    thread processor_thread(&OFSServer::processorLoop, this);
    processor_thread.detach();

    listenLoop();
}

void OFSServer::listenLoop() {
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        if (client_socket < 0) {
            cerr << "Failed to accept connection" << endl;
            continue;
        }

        thread client_thread(&OFSServer::handleClientConnection, this, client_socket);
        client_thread.detach();
    }
}

void OFSServer::processorLoop() {
    cout << "Processor thread started." << endl;
    while (true) {
        Request req = request_queue.pop();
        cout << "Processing request for op: " << req.data.value("operation", "unknown") << endl;
        
        string response_str = processRequest(req.data);
        
        send(req.client_socket, response_str.c_str(), response_str.length(), 0);
        close(req.client_socket);
        cout << "Response sent, client disconnected." << endl;
    }
}

void OFSServer::handleClientConnection(int client_socket) {
    // Increase buffer size to handle larger requests
    char buffer[8192] = {0};
    string raw_request;
    
    while (recv(client_socket, buffer, sizeof(buffer) - 1, 0) > 0) {
        raw_request += buffer;
        if (raw_request.find('\n') != string::npos) {
            break; 
        }
        memset(buffer, 0, sizeof(buffer));
    }

    if (raw_request.empty()) {
        close(client_socket);
        return;
    }

    size_t json_start = raw_request.find('{');
    if (json_start == string::npos) {
        // Only print error if it's not a preflight check or empty ping
        if (raw_request.length() > 1) 
             cout << "Failed to find JSON body." << endl;
        close(client_socket);
        return;
    }

    string json_body = raw_request.substr(json_start);

    try {
        Request req;
        req.client_socket = client_socket;
        req.data = json::parse(json_body);
        request_queue.push(req);
        cout << "Request queued." << endl;
    } catch (const json::exception& e) {
        cout << "Failed to parse JSON: " << e.what() << endl;
        string err = "{\"status\":\"error\",\"error_message\":\"Invalid JSON\"}\n";
        send(client_socket, err.c_str(), err.length(), 0);
        close(client_socket);
    }
}

string OFSServer::processRequest(json req_data) {
    json response;
    string op = req_data.value("operation", "");
    string session_id = req_data.value("session_id", "");
    
    response["operation"] = op;
    response["request_id"] = req_data.value("request_id", "none");
    
    if (op.empty()) {
        response["status"] = "error";
        response["error_message"] = "Invalid operation";
        return response.dump() + "\n";
    }
    
    int result = (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
    
    try {
        if (op == "user_login") {
            string user = req_data["parameters"]["username"];
            string pass = req_data["parameters"]["password"];
            SessionInfo* new_session = nullptr;
            
            result = user_login(fs_instance, user.c_str(), pass.c_str(), &new_session);
            
            if (result == (int)OFSErrorCodes::SUCCESS) {
                response["data"]["session_id"] = new_session->session_id;
                
                if (new_session->user.role == UserRole::ADMIN) {
                    response["data"]["role"] = "admin";
                } else {
                    response["data"]["role"] = "normal";
                }
                
                free_session(new_session);
            }
        }
        else if (op == "user_logout") {
            result = user_logout(fs_instance, session_id.c_str());
        }
        else if (op == "get_error_message") {
            int code = req_data["parameters"].value("error_code", 0);
            response["data"] = get_error_message(code);
            result = (int)OFSErrorCodes::SUCCESS;
        }
        else {
            SessionInfo session_check;
            if (get_session_info(fs_instance, session_id.c_str(), &session_check) != 0) {
                result = (int)OFSErrorCodes::ERROR_INVALID_SESSION;
            } else {
                if (op == "user_create") {
                    string user = req_data["parameters"]["username"];
                    string pass = req_data["parameters"]["password"];
                    
                    // --- FIX 1: READ ROLE AS STRING FIRST ---
                    string role_str = req_data["parameters"].value("role", "normal");
                    UserRole role = (role_str == "admin") ? UserRole::ADMIN : UserRole::NORMAL;
                    
                    result = user_create(fs_instance, user.c_str(), pass.c_str(), role);
                }
                else if (op == "user_delete") {
                    string user = req_data["parameters"]["username"];
                    result = user_delete(fs_instance, user.c_str());
                }
                else if (op == "user_list") {
                    UserInfo* users = nullptr;
                    int count = 0;
                    result = user_list(fs_instance, &users, &count);
                    if (result == (int)OFSErrorCodes::SUCCESS) {
                        json user_list_json = json::array();
                        for(int i = 0; i < count; ++i) {
                            json user_json;
                            user_json["username"] = users[i].username;
                            user_json["role"] = (users[i].role == UserRole::ADMIN) ? "admin" : "normal";
                            user_json["is_active"] = users[i].is_active;
                            user_list_json.push_back(user_json);
                        }
                        response["data"] = user_list_json;
                        free_user_list_memory(users);
                    }
                }
                else if (op == "dir_create") {
                    string path = req_data["parameters"]["path"];
                    result = dir_create(fs_instance, path.c_str());
                }
                else if (op == "dir_delete") {
                    string path = req_data["parameters"]["path"];
                    result = dir_delete(fs_instance, path.c_str());
                }
                else if (op == "dir_exists") {
                    string path = req_data["parameters"]["path"];
                    result = dir_exists(fs_instance, path.c_str());
                }
                else if (op == "file_create") { 
                    string path = req_data["parameters"]["path"];
                    
                    // Robust size parsing
                    size_t size = 0;
                    if (req_data["parameters"]["size"].is_number()) {
                        size = req_data["parameters"]["size"];
                    } else if (req_data["parameters"]["size"].is_string()) {
                        try {
                            size = std::stoul(req_data["parameters"]["size"].get<std::string>());
                        } catch (...) { size = 0; }
                    }
                    
                    result = file_create(fs_instance, path.c_str(), nullptr, size);
                }
                else if (op == "file_delete") {
                    string path = req_data["parameters"]["path"];
                    result = file_delete(fs_instance, path.c_str());
                }
                else if (op == "file_exists") {
                    string path = req_data["parameters"]["path"];
                    result = file_exists(fs_instance, path.c_str());
                }
                else if (op == "dir_list") {
                    string path = req_data["parameters"]["path"];
                    FileEntry* entries = nullptr;
                    int count = 0;
                    result = dir_list(fs_instance, path.c_str(), &entries, &count);
                    
                    if (result == (int)OFSErrorCodes::SUCCESS) {
                        json entry_list = json::array();
                        for(int i = 0; i < count; ++i) {
                            json entry;
                            entry["name"] = entries[i].name;
                            entry["type"] = (entries[i].getType() == EntryType::DIRECTORY) ? "directory" : "file";
                            entry["size"] = (unsigned long)entries[i].size;
                            entry_list.push_back(entry);
                        }
                        response["data"] = entry_list;
                        free_dir_list_memory(entries);
                    }
                }
                else if (op == "get_stats") {
                    FSStats stats;
                    result = get_stats(fs_instance, &stats);
                    if (result == (int)OFSErrorCodes::SUCCESS) {
                        response["data"]["total_size"] = (unsigned long)stats.total_size;
                        response["data"]["used_space"] = (unsigned long)stats.used_space;
                        response["data"]["free_space"] = (unsigned long)stats.free_space;
                        response["data"]["total_files"] = stats.total_files;
                        response["data"]["total_directories"] = stats.total_directories;
                        response["data"]["total_users"] = stats.total_users;
                        response["data"]["active_sessions"] = stats.active_sessions;
                    }
                }
                else if (op == "file_truncate") {
                    string path = req_data["parameters"]["path"];
                    result = file_truncate(fs_instance, path.c_str());
                }
                else if (op == "file_rename") {
                    string old_path = req_data["parameters"]["old_path"];
                    string new_path = req_data["parameters"]["new_path"];
                    result = file_rename(fs_instance, old_path.c_str(), new_path.c_str());
                }
                else if (op == "get_metadata") {
                    string path = req_data["parameters"]["path"];
                    
                    // --- FIX 2: ZERO OUT MEMORY TO PREVENT UTF-8 CRASH ---
                    FileMetadata meta;
                    memset(&meta, 0, sizeof(FileMetadata));
                    
                    result = get_metadata(fs_instance, path.c_str(), &meta);
                    if (result == (int)OFSErrorCodes::SUCCESS) {
                        response["data"]["path"] = meta.path;
                        response["data"]["entry"]["name"] = meta.entry.name;
                        response["data"]["entry"]["size"] = (unsigned long)meta.entry.size;
                        response["data"]["entry"]["permissions"] = (unsigned int)meta.entry.permissions;
                        response["data"]["blocks_used"] = (unsigned long)meta.blocks_used;
                    }
                }
                else if (op == "set_permissions") {
                    string path = req_data["parameters"]["path"];
                    uint32_t perms = req_data["parameters"]["permissions"];
                    result = set_permissions(fs_instance, path.c_str(), perms);
                }
                else if (op == "file_edit") {
                    string path = req_data["parameters"]["path"];
                    string data = req_data["parameters"]["data"];
                    result = file_edit(fs_instance, path.c_str(), data.c_str(), data.length(), 0);
                }
                else if (op == "file_read") {
                    string path = req_data["parameters"]["path"];
                    char* buffer = nullptr;
                    size_t size = 0;
                    result = file_read(fs_instance, path.c_str(), &buffer, &size);
                    if (result == (int)OFSErrorCodes::SUCCESS) {
                        if (buffer != nullptr) {
                            response["data"]["content"] = string(buffer, size);
                            free_buffer(buffer);
                        } else {
                            response["data"]["content"] = "";
                        }
                    }
                }
            }
        }

        if (result == (int)OFSErrorCodes::SUCCESS) {
            response["status"] = "success";
        } else {
            response["status"] = "error";
            response["error_code"] = result;
        }
    
    } catch (const json::exception& e) {
        response["status"] = "error";
        response["error_message"] = "JSON parsing error: " + string(e.what());
    } catch (...) {
        response["status"] = "error";
        response["error_message"] = "Unknown server error";
    }
    
    return response.dump() + "\n";
}