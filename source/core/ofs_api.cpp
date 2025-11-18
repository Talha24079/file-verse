#include "ofs_api.hpp"
#include <fstream>   
#include <iostream>  
#include <cstring>   
#include <sstream>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include <functional>

#include "../data_structures/free_space_bitmap.hpp"

using namespace std;

FSTreeNode* find_node_by_path(FSTreeNode* root, const string& path);
void collect_nodes(FSTreeNode* node, string current_path, vector<pair<string, FSTreeNode*>>& all_nodes);
void parse_path(const string& path, string& parent_path, string& child_name);

void save_file_system(OFSInstance* fs_instance) {
    if (!fs_instance) return;
    fstream omni_file(fs_instance->omni_path, ios::binary | ios::in | ios::out);
    if (!omni_file) return;

    vector<UserInfo> users = fs_instance->userTree.listAllUsers();
    size_t user_table_offset = sizeof(OMNIHeader);
    
    UserInfo empty_user = {};
    omni_file.seekp(user_table_offset);
    for(int i=0; i < fs_instance->config.max_users; ++i) {
         omni_file.write(reinterpret_cast<const char*>(&empty_user), sizeof(UserInfo));
    }
    
    omni_file.seekp(user_table_offset);
    for (const auto& user : users) {
        omni_file.write(reinterpret_cast<const char*>(&user), sizeof(UserInfo));
    }

    size_t fs_tree_offset = user_table_offset + (fs_instance->config.max_users * sizeof(UserInfo));
    
    vector<pair<string, FSTreeNode*>> all_nodes;
    collect_nodes(fs_instance->fsTree.root, "/", all_nodes);
    
    FileEntry empty_node = {};
    omni_file.seekp(fs_tree_offset);
    for(int i=0; i < fs_instance->config.max_files; ++i) {
         omni_file.write(reinterpret_cast<const char*>(&empty_node), sizeof(FileEntry));
    }

    omni_file.seekp(fs_tree_offset);
    for (const auto& item : all_nodes) {
        FSTreeNode* node = item.second;
        string full_path = item.first;

        FileEntry disk_entry = node->metadata;
        strncpy(disk_entry.name, full_path.c_str(), sizeof(disk_entry.name) - 1);
        
        if (!node->data_blocks.empty()) {
            disk_entry.inode = node->data_blocks[0]; 
        }
        omni_file.write(reinterpret_cast<const char*>(&disk_entry), sizeof(FileEntry));
    }

    size_t total_blocks = fs_instance->bitmap.size();
    size_t bitmap_size_bytes = (total_blocks + 7) / 8;
    size_t bitmap_size_aligned = (size_t)ceil((double)bitmap_size_bytes / fs_instance->config.block_size) * fs_instance->config.block_size;
    size_t bitmap_offset = fs_tree_offset + (fs_instance->config.max_files * sizeof(FileEntry));

    char* bitmap_data = new char[bitmap_size_aligned];
    memset(bitmap_data, 0, bitmap_size_aligned);
    
    size_t data_blocks_offset = bitmap_offset + bitmap_size_aligned;
    size_t data_blocks_start = (data_blocks_offset + fs_instance->config.block_size - 1) / fs_instance->config.block_size;
    
    for(size_t i = 0; i < data_blocks_start; ++i) {
        bitmap_data[i/8] |= (1 << (i%8));
    }
    
    for (const auto& item : all_nodes) {
        FSTreeNode* node = item.second;
        for (int block : node->data_blocks) {
            if (block < total_blocks) {
                bitmap_data[block/8] |= (1 << (block%8));
            }
        }
    }

    omni_file.seekp(bitmap_offset);
    omni_file.write(bitmap_data, bitmap_size_aligned);
    delete[] bitmap_data;

    omni_file.close();
    cout << "System state saved to .omni file." << endl;
}

int fs_format(const string& omni_path, const string& config_path) {
    Config config;
    if (!parse_config(config_path, config)) {
        cerr << "Error: Could not parse config file." << endl;
        return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    ofstream omni_file(omni_path, ios::binary | ios::trunc);
    if (!omni_file) {
        cerr << "Error: Could not create .omni file at " << omni_path << endl;
        return (int)OFSErrorCodes::ERROR_IO_ERROR;
    }

    OMNIHeader header = {};
    strncpy(header.magic, "OMNIFS01", 8);
    header.total_size = config.total_size;
    header.header_size = sizeof(OMNIHeader);
    header.block_size = config.block_size;
    header.max_users = config.max_users;

    size_t total_blocks = config.total_size / config.block_size;
    size_t user_table_size = config.max_users * sizeof(UserInfo);
    size_t fs_tree_size = config.max_files * sizeof(FileEntry); 
    
    size_t bitmap_size_bits = total_blocks;
    size_t bitmap_size_bytes = (bitmap_size_bits + 7) / 8;
    size_t bitmap_size_aligned = (size_t)ceil((double)bitmap_size_bytes / config.block_size) * config.block_size;

    size_t user_table_offset = sizeof(OMNIHeader);
    size_t fs_tree_offset = user_table_offset + user_table_size;
    size_t bitmap_offset = fs_tree_offset + fs_tree_size;
    size_t data_blocks_offset = bitmap_offset + bitmap_size_aligned;
    size_t data_blocks_start_block = (data_blocks_offset + config.block_size - 1) / config.block_size;

    header.user_table_offset = user_table_offset;
    
    omni_file.seekp(0);
    omni_file.write(reinterpret_cast<const char*>(&header), sizeof(OMNIHeader));

    UserInfo empty_user = {};
    omni_file.seekp(user_table_offset);
    for(int i = 0; i < config.max_users; ++i) {
        omni_file.write(reinterpret_cast<const char*>(&empty_user), sizeof(UserInfo));
    }
    
    string admin_user = config.admin_username;
    string admin_pass = config.admin_password;
    UserInfo adminUser(admin_user, admin_pass, UserRole::ADMIN, 0);
    adminUser.is_active = 1;

    omni_file.seekp(user_table_offset);
    omni_file.write(reinterpret_cast<const char*>(&adminUser), sizeof(UserInfo));

    FileEntry empty_node = {};
    FileEntry root_node("/", EntryType::DIRECTORY, 0, 0755, "admin", 0, 0);
    
    omni_file.seekp(fs_tree_offset);
    omni_file.write(reinterpret_cast<const char*>(&root_node), sizeof(FileEntry));
    for(int i = 0; i < config.max_files - 1; ++i) {
        omni_file.write(reinterpret_cast<const char*>(&empty_node), sizeof(FileEntry));
    }

    char* bitmap_data = new char[bitmap_size_aligned];
    memset(bitmap_data, 0, bitmap_size_aligned);

    for(size_t i = 0; i < data_blocks_start_block; ++i) {
        size_t byte_index = i / 8;
        int bit_index = i % 8;
        bitmap_data[byte_index] |= (1 << bit_index);
    }

    omni_file.seekp(bitmap_offset);
    omni_file.write(bitmap_data, bitmap_size_aligned);
    delete[] bitmap_data;

    omni_file.seekp(config.total_size - 1);
    omni_file.write("\0", 1);
    
    omni_file.close();
    
    cout << "fs_format: Successfully created and formatted " << omni_path << endl;
    return (int)OFSErrorCodes::SUCCESS;
}

int fs_init(void** instance, const string& omni_path, const string& config_path) {
    Config config;
    if (!parse_config(config_path, config)) {
        return (int)OFSErrorCodes::ERROR_INVALID_CONFIG;
    }

    ifstream omni_file(omni_path, ios::binary);
    if (!omni_file) {
        return (int)OFSErrorCodes::ERROR_IO_ERROR;
    }

    OMNIHeader header;
    omni_file.read(reinterpret_cast<char*>(&header), sizeof(OMNIHeader));
    if (strncmp(header.magic, "OMNIFS01", 8) != 0) {
        return (int)OFSErrorCodes::ERROR_IO_ERROR;
    }
    
    OFSInstance* fs_instance = new OFSInstance();
    fs_instance->config = config;
    fs_instance->omni_path = omni_path;

    omni_file.seekg(header.user_table_offset);
    for(int i = 0; i < header.max_users; ++i) {
        UserInfo user;
        omni_file.read(reinterpret_cast<char*>(&user), sizeof(UserInfo));
        if (user.is_active) {
            fs_instance->userTree.insert(user);
        }
    }

    size_t fs_tree_offset = header.user_table_offset + (header.max_users * sizeof(UserInfo));
    omni_file.seekg(fs_tree_offset);
    
    for(int i = 0; i < config.max_files; ++i) {
        FileEntry entry;
        omni_file.read(reinterpret_cast<char*>(&entry), sizeof(FileEntry));
        
        if (entry.name[0] == '\0') continue;

        string full_path = entry.name;
        if (full_path == "/") continue; 

        string parent_path, node_name;
        parse_path(full_path, parent_path, node_name);

        FSTreeNode* parent = find_node_by_path(fs_instance->fsTree.root, parent_path);
        if (parent) {
            strncpy(entry.name, node_name.c_str(), sizeof(entry.name)-1);
            
            FSTreeNode* new_node = new FSTreeNode(entry, parent);
            
            if (entry.getType() == EntryType::FILE && entry.size > 0) {
                int start_block = entry.inode; 
                size_t blocks_needed = (entry.size + config.block_size - 1) / config.block_size;
                for(size_t b = 0; b < blocks_needed; ++b) {
                    new_node->data_blocks.push_back(start_block + b);
                }
            }
            parent->addChild(new_node);
        }
    }
    
    size_t total_blocks = config.total_size / config.block_size;
    size_t bitmap_size_bytes = (total_blocks + 7) / 8;
    size_t bitmap_size_aligned = (size_t)ceil((double)bitmap_size_bytes / config.block_size) * config.block_size;
    size_t bitmap_offset = fs_tree_offset + (config.max_files * sizeof(FileEntry));

    char* bitmap_data = new char[bitmap_size_aligned];
    omni_file.seekg(bitmap_offset);
    omni_file.read(bitmap_data, bitmap_size_aligned);

    fs_instance->bitmap.initialize(total_blocks);
    for(size_t i = 0; i < total_blocks; ++i) {
        size_t byte_index = i / 8;
        int bit_index = i % 8;
        if ((bitmap_data[byte_index] >> bit_index) & 1) {
            fs_instance->bitmap.setBlock(i);
        }
    }
    delete[] bitmap_data;
    
    omni_file.close();

    *instance = (void*)fs_instance;
    cout << "fs_init: Successfully loaded instance from " << omni_path << endl;
    return (int)OFSErrorCodes::SUCCESS;
}

void fs_shutdown(void* instance) {
    if (instance == nullptr) return;
    OFSInstance* fs_instance = (OFSInstance*)instance;
    save_file_system(fs_instance);
    delete fs_instance;
    cout << "fs_shutdown: Successfully saved and shut down." << endl;
}

FSTreeNode* find_node_by_path(FSTreeNode* root, const string& path) {
    if (path == "/" || path == "") return root;

    stringstream ss(path);
    string segment;
    FSTreeNode* current_node = root;

    while (getline(ss, segment, '/')) {
        if (segment.empty()) continue;
        current_node = current_node->findChild(segment);
        if (current_node == nullptr) return nullptr; 
    }
    return current_node;
}

void parse_path(const string& path, string& parent_path, string& child_name) {
    size_t last_slash = path.find_last_of('/');
    if (last_slash == string::npos || last_slash == 0) {
        parent_path = "/";
    } else {
        parent_path = path.substr(0, last_slash);
    }
    child_name = path.substr(last_slash + 1);
}

void collect_nodes(FSTreeNode* node, string current_path, vector<pair<string, FSTreeNode*>>& all_nodes) {
    if (!node) return;
    if (current_path != "" && current_path != "/") {
        all_nodes.push_back({current_path, node});
    }
    vector<FSTreeNode*> children = node->listChildren();
    for (auto child : children) {
        string child_path = (current_path == "/" ? "" : current_path) + "/" + child->metadata.name;
        collect_nodes(child, child_path, all_nodes);
    }
}

string generate_session_id() {
    long long part1 = rand();
    long long part2 = rand();
    return to_string(part1) + to_string(part2);
}

int user_create(void* instance, const char* username, const char* password, UserRole role) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    if (fs_instance->userTree.find(username) != nullptr) return (int)OFSErrorCodes::ERROR_FILE_EXISTS;

    UserInfo newUser(username, password, role, 0);
    newUser.is_active = 1;
    fs_instance->userTree.insert(newUser);
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int user_list(void* instance, UserInfo** users, int* count) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    vector<UserInfo> userVector = fs_instance->userTree.listAllUsers();
    *count = userVector.size();
    if (*count == 0) { *users = nullptr; return (int)OFSErrorCodes::SUCCESS; }

    *users = new UserInfo[*count];
    for (int i = 0; i < *count; ++i) (*users)[i] = userVector[i];
    return (int)OFSErrorCodes::SUCCESS;
}

void free_user_list_memory(UserInfo* users) {
    if (users != nullptr) delete[] users;
}

int user_login(void* instance, const char* username, const char* password, SessionInfo** session) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    UserInfo* user = fs_instance->userTree.find(username);
    if (user == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;

    if (strcmp(user->password_hash, password) != 0) return (int)OFSErrorCodes::ERROR_PERMISSION_DENIED;

    string session_id = generate_session_id();
    SessionInfo new_session(session_id, *user, 0);
    
    lock_guard<mutex> lock(fs_instance->session_mutex);
    fs_instance->active_sessions.push_back(new_session);
    *session = new SessionInfo(new_session);
    
    return (int)OFSErrorCodes::SUCCESS;
}

int user_logout(void* instance, const char* session_id) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    lock_guard<mutex> lock(fs_instance->session_mutex);
    for (auto it = fs_instance->active_sessions.begin(); it != fs_instance->active_sessions.end(); ++it) {
        if (strcmp(it->session_id, session_id) == 0) {
            fs_instance->active_sessions.erase(it);
            cout << "user_logout: Session ended." << endl;
            return (int)OFSErrorCodes::SUCCESS;
        }
    }
    return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
}

int get_session_info(void* instance, const char* session_id, SessionInfo* info) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    lock_guard<mutex> lock(fs_instance->session_mutex);
    for (const auto& session : fs_instance->active_sessions) {
        if (strcmp(session.session_id, session_id) == 0) {
            *info = session;
            return (int)OFSErrorCodes::SUCCESS;
        }
    }
    return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
}

void free_session(SessionInfo* session) {
    if (session != nullptr) delete session;
}

int user_delete(void* instance, const char* username) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    if (fs_instance->userTree.find(username) == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    fs_instance->userTree.remove(username);
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int dir_create(void* instance, const char* path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    string parent_path, name;
    parse_path(path, parent_path, name);

    FSTreeNode* parent = find_node_by_path(fs_instance->fsTree.root, parent_path);
    if (parent == nullptr || !parent->isDirectory()) return (int)OFSErrorCodes::ERROR_NOT_FOUND; 
    if (parent->findChild(name) != nullptr) return (int)OFSErrorCodes::ERROR_FILE_EXISTS;

    uint32_t parent_inode_dir = parent->metadata.inode;
    FileEntry meta(name, EntryType::DIRECTORY, 0, 0755, "admin", 0, parent_inode_dir);
    FSTreeNode* new_dir = new FSTreeNode(meta, parent);
    parent->addChild(new_dir);
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int dir_list(void* instance, const char* path, FileEntry** entries, int* count) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    FSTreeNode* dir = find_node_by_path(fs_instance->fsTree.root, path);
    if (dir == nullptr || !dir->isDirectory()) return (int)OFSErrorCodes::ERROR_NOT_FOUND;

    vector<FSTreeNode*> children = dir->listChildren();
    *count = children.size();
    if (*count == 0) { *entries = nullptr; return (int)OFSErrorCodes::SUCCESS; }

    *entries = new FileEntry[*count];
    for (int i = 0; i < *count; ++i) (*entries)[i] = children[i]->metadata;
    return (int)OFSErrorCodes::SUCCESS;
}

void free_dir_list_memory(FileEntry* entries) {
    if (entries != nullptr) delete[] entries;
}

int dir_delete(void* instance, const char* path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    if (string(path) == "/") return (int)OFSErrorCodes::ERROR_INVALID_OPERATION;

    string parent_path, name;
    parse_path(path, parent_path, name);

    FSTreeNode* parent = find_node_by_path(fs_instance->fsTree.root, parent_path);
    if (parent == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;

    FSTreeNode* node = parent->findChild(name);
    if (node == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    if (!node->isDirectory()) return (int)OFSErrorCodes::ERROR_INVALID_OPERATION;

    if (!node->listChildren().empty()) return (int)OFSErrorCodes::ERROR_DIRECTORY_NOT_EMPTY;

    parent->removeChild(name);
    delete node;
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int dir_exists(void* instance, const char* path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node != nullptr && node->isDirectory()) return (int)OFSErrorCodes::SUCCESS;
    return (int)OFSErrorCodes::ERROR_NOT_FOUND;
}

int file_create(void* instance, const char* path, const char* data, size_t size) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    
    string parent_path, name;
    parse_path(path, parent_path, name);

    FSTreeNode* parent = find_node_by_path(fs_instance->fsTree.root, parent_path);
    if (parent == nullptr || !parent->isDirectory()) return (int)OFSErrorCodes::ERROR_NOT_FOUND; 
    if (parent->findChild(name) != nullptr) return (int)OFSErrorCodes::ERROR_FILE_EXISTS;

    size_t blocks_needed = (size == 0) ? 1 : (size + fs_instance->config.block_size - 1) / fs_instance->config.block_size;
    int start_block = fs_instance->bitmap.findFreeBlocks(blocks_needed);
    if (start_block == -1) return (int)OFSErrorCodes::ERROR_NO_SPACE;

    fs_instance->bitmap.setBlocks(start_block, blocks_needed);

    uint32_t parent_inode_file = parent->metadata.inode;
    FileEntry meta(name, EntryType::FILE, size, 0644, "admin", start_block, parent_inode_file); 
    meta.inode = start_block; 
    FSTreeNode* new_file = new FSTreeNode(meta, parent);

    for (size_t i = 0; i < blocks_needed; ++i) new_file->data_blocks.push_back(start_block + i);
    parent->addChild(new_file);

    if (data != nullptr && size > 0) {
        const char* data_ptr = data;
        size_t bytes_left = size;

        fstream omni_file(fs_instance->omni_path, ios::binary | ios::in | ios::out);
        if (omni_file) {
            for (int block : new_file->data_blocks) {
                if (bytes_left == 0) break;
                size_t offset = (size_t)block * fs_instance->config.block_size;
                omni_file.seekp(offset);
                size_t to_write = min(bytes_left, (size_t)fs_instance->config.block_size);
                omni_file.write(data_ptr, to_write);
                data_ptr += to_write;
                bytes_left -= to_write;
            }
            omni_file.close();
        }
    }
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int file_delete(void* instance, const char* path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    
    string parent_path, name;
    parse_path(path, parent_path, name);

    FSTreeNode* parent = find_node_by_path(fs_instance->fsTree.root, parent_path);
    if (parent == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;

    FSTreeNode* node = parent->findChild(name);
    if (node == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    if (node->isDirectory()) return (int)OFSErrorCodes::ERROR_INVALID_OPERATION;

    for (int block : node->data_blocks) fs_instance->bitmap.freeBlock(block);
    parent->removeChild(name);
    delete node;
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int file_exists(void* instance, const char* path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node != nullptr && !node->isDirectory()) return (int)OFSErrorCodes::SUCCESS;
    return (int)OFSErrorCodes::ERROR_NOT_FOUND;
}

int file_read(void* instance, const char* path, char** buffer, size_t* size) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node == nullptr || node->isDirectory()) return (int)OFSErrorCodes::ERROR_NOT_FOUND;

    *size = node->metadata.size;
    if (*size == 0) { *buffer = nullptr; return (int)OFSErrorCodes::SUCCESS; }

    *buffer = new char[*size + 1]; 
    memset(*buffer, 0, *size + 1);
    char* data_ptr = *buffer;
    size_t bytes_left = *size;
    ifstream omni_file(fs_instance->omni_path, ios::binary);
    if (!omni_file) { delete[] *buffer; return (int)OFSErrorCodes::ERROR_IO_ERROR; }

    for (int block : node->data_blocks) {
        if (bytes_left == 0) break;
        size_t offset = (size_t)block * fs_instance->config.block_size;
        omni_file.seekg(offset);
        size_t to_read = min(bytes_left, (size_t)fs_instance->config.block_size);
        omni_file.read(data_ptr, to_read);
        data_ptr += to_read;
        bytes_left -= to_read;
    }
    omni_file.close();
    return (int)OFSErrorCodes::SUCCESS;
}

int file_edit(void* instance, const char* path, const char* data, size_t size, uint32_t index) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node == nullptr || node->isDirectory()) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    if (index != 0) return (int)OFSErrorCodes::ERROR_NOT_IMPLEMENTED;
    
    if (size > (node->data_blocks.size() * fs_instance->config.block_size)) return (int)OFSErrorCodes::ERROR_NO_SPACE;

    const char* data_ptr = data;
    size_t bytes_left = size;

    fstream omni_file(fs_instance->omni_path, ios::binary | ios::in | ios::out);
    if (!omni_file) return (int)OFSErrorCodes::ERROR_IO_ERROR;

    for (int block : node->data_blocks) {
        if (bytes_left == 0) break;
        size_t offset = (size_t)block * fs_instance->config.block_size;
        omni_file.seekp(offset);
        size_t to_write = min(bytes_left, (size_t)fs_instance->config.block_size);
        omni_file.write(data_ptr, to_write);
        data_ptr += to_write;
        bytes_left -= to_write;
    }
    omni_file.close();
    node->metadata.size = size;
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int file_truncate(void* instance, const char* path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node == nullptr || node->isDirectory()) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    
    node->metadata.size = 0;
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int file_rename(void* instance, const char* old_path, const char* new_path) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, old_path);
    if (node == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    
    string p_path, name;
    parse_path(new_path, p_path, name);
    
    FSTreeNode* new_parent = find_node_by_path(fs_instance->fsTree.root, p_path);
    if (new_parent == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;

    strncpy(node->metadata.name, name.c_str(), sizeof(node->metadata.name)-1);
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int get_metadata(void* instance, const char* path, FileMetadata* meta) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    meta->entry = node->metadata;
    meta->blocks_used = node->data_blocks.size();
    return (int)OFSErrorCodes::SUCCESS;
}

int set_permissions(void* instance, const char* path, uint32_t permissions) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;
    FSTreeNode* node = find_node_by_path(fs_instance->fsTree.root, path);
    if (node == nullptr) return (int)OFSErrorCodes::ERROR_NOT_FOUND;
    node->metadata.permissions = permissions;
    
    save_file_system(fs_instance);
    return (int)OFSErrorCodes::SUCCESS;
}

int get_stats(void* instance, FSStats* stats) {
    OFSInstance* fs_instance = (OFSInstance*)instance;
    if (fs_instance == nullptr) return (int)OFSErrorCodes::ERROR_INVALID_SESSION;

    stats->total_size = fs_instance->config.total_size;
    size_t total_files = 0;
    size_t total_dirs = 0;
    size_t used_space = 0;

    vector<FSTreeNode*> nodes_to_visit;
    nodes_to_visit.push_back(fs_instance->fsTree.root);

    while (!nodes_to_visit.empty()) {
        FSTreeNode* current = nodes_to_visit.back();
        nodes_to_visit.pop_back();
        if (current->isDirectory()) {
            total_dirs++;
            vector<FSTreeNode*> children = current->listChildren();
            for (FSTreeNode* child : children) nodes_to_visit.push_back(child);
        } else {
            total_files++;
            used_space += current->metadata.size;
        }
    }

    stats->total_files = total_files;
    stats->total_directories = total_dirs;
    stats->used_space = used_space;
    stats->free_space = stats->total_size - used_space;
    stats->total_users = fs_instance->userTree.listAllUsers().size();
    lock_guard<mutex> lock(fs_instance->session_mutex);
    stats->active_sessions = fs_instance->active_sessions.size();
    return (int)OFSErrorCodes::SUCCESS;
}

void free_buffer(char* buffer) {
    if (buffer) delete[] buffer;
}

const char* get_error_message(int error_code) {
    return "Error";
}