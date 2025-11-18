#pragma once

#include "../include/odf_types.hpp"
#include "child_avl_tree.hpp"
#include <string>  
#include <vector>  

using namespace std;

struct FSTreeNode {
    FileEntry metadata;
    FSTreeNode* parent;

    ChildAVLTree children;

    vector<int> data_blocks;

    FSTreeNode(const FileEntry& meta, FSTreeNode* p) : 
        metadata(meta), parent(p) {
    }

    bool isDirectory() const 
    {
        return metadata.getType() == EntryType::DIRECTORY;
    }

    void addChild(FSTreeNode* childNode) 
    {
        if (isDirectory()) 
        {
            children.insert(childNode);
        }
    }

    FSTreeNode* findChild(const string& name)
    {
        if (isDirectory()) 
        {
            return children.find(name);
        }
        return nullptr;
    }

    void removeChild(const string& name)
    {
        if (isDirectory()) 
        {
            children.remove(name);
        }
    }

    vector<FSTreeNode*> listChildren()
    {
        if (isDirectory()) 
        {
            return children.listAll();
        }
        return {};
    }
};

class FileSystemTree {
private:
    uint32_t next_inode = 1;
public:
    FSTreeNode* root;

    FileSystemTree()
    {
        FileEntry rootMeta("/", EntryType::DIRECTORY, 0, 0755, "admin", 0, 0);
        root = new FSTreeNode(rootMeta, nullptr);
    }

    uint32_t generate_inode() {
        return next_inode++;
    }
};