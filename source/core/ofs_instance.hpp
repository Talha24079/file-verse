#pragma once
#include "../data_structures/user_avl_tree.hpp"
#include "../data_structures/fs_tree.hpp"
#include "../data_structures/free_space_bitmap.hpp"
#include "config_parser.hpp"
#include <mutex>
#include <string>
#include <vector>
#include "../include/odf_types.hpp"

struct OFSInstance {
    std::string omni_path;
    Config config;
    UserAVLTree userTree;
    FileSystemTree fsTree;
    FreeSpaceBitmap bitmap;
    std::vector<SessionInfo> active_sessions;
    std::mutex session_mutex;
};