#pragma once
#include <string>
#include "ofs_instance.hpp"
#include "../include/odf_types.hpp" 

using namespace std;

int fs_format(const string& omni_path, const string& config_path);
int fs_init(void** instance, const string& omni_path, const string& config_path);
void fs_shutdown(void* instance);

int user_create(void* admin_session, const char* username, const char* password, UserRole role);
int user_list(void* admin_session, UserInfo** users, int* count);
void free_user_list_memory(UserInfo* users);

int user_login(void* instance, const char* username, const char* password, SessionInfo** session);
int user_logout(void* instance, const char* session_id);
int get_session_info(void* instance, const char* session_id, SessionInfo* info);
void free_session(SessionInfo* session);
int user_delete(void* instance, const char* username);

int dir_create(void* session, const char* path);
int dir_list(void* session, const char* path, FileEntry** entries, int* count);
void free_dir_list_memory(FileEntry* entries);
int dir_delete(void* instance, const char* path);
int dir_exists(void* instance, const char* path);

int file_create(void* session, const char* path, const char* data, size_t size);
int file_delete(void* instance, const char* path);
int file_exists(void* instance, const char* path);

int file_read(void* instance, const char* path, char** buffer, size_t* size);
int file_edit(void* instance, const char* path, const char* data, size_t size, uint32_t index);
int file_truncate(void* instance, const char* path);
int file_rename(void* instance, const char* old_path, const char* new_path);

int get_metadata(void* instance, const char* path, FileMetadata* meta);
int set_permissions(void* instance, const char* path, uint32_t permissions);
int get_stats(void* instance, FSStats* stats);
void free_buffer(char* buffer);
const char* get_error_message(int error_code);