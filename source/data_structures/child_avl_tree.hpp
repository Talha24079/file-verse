#pragma once
#include <string>
#include <vector>
#include "../include/odf_types.hpp"

using namespace std;

struct FSTreeNode;

struct ChildAVLNode {
    FSTreeNode* fsNode; 
    ChildAVLNode *left;
    ChildAVLNode *right;
    int height;

    ChildAVLNode(FSTreeNode* node) : 
        fsNode(node), left(nullptr), right(nullptr), height(1) {}
};

class ChildAVLTree {
private:
    ChildAVLNode *root;

    ChildAVLNode* insert(ChildAVLNode* node, FSTreeNode* fsNode);
    ChildAVLNode* find(ChildAVLNode* node, const string& name);
    ChildAVLNode* remove(ChildAVLNode* node, const string& name);
    void inOrder(ChildAVLNode* node, vector<FSTreeNode*>& nodes);
    
    int getHeight(ChildAVLNode* node);
    int getBalance(ChildAVLNode* node);
    ChildAVLNode* rightRotate(ChildAVLNode* y);
    ChildAVLNode* leftRotate(ChildAVLNode* x);
    ChildAVLNode* minValueNode(ChildAVLNode* node);

public:
    ChildAVLTree() : root(nullptr) {}

    void insert(FSTreeNode* fsNode);
    FSTreeNode* find(const string& name);
    void remove(const string& name);
    vector<FSTreeNode*> listAll();
};