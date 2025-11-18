#include "child_avl_tree.hpp"
#include "fs_tree.hpp" 
#include <algorithm>   
#include <cstring>    
#include <iostream>

using namespace std;

void ChildAVLTree::insert(FSTreeNode* fsNode) 
{
    if (fsNode == nullptr)
     return;
    root = insert(root, fsNode);
}

FSTreeNode* ChildAVLTree::find(const string& name)
{
    ChildAVLNode* result = find(root, name);
    if (result == nullptr) 
    {
        return nullptr; 
    }
    return result->fsNode; 
}

void ChildAVLTree::remove(const string& name) 
{
    root = remove(root, name);
}

vector<FSTreeNode*> ChildAVLTree::listAll() 
{
    vector<FSTreeNode*> nodes;
    inOrder(root, nodes);
    return nodes;
}

int ChildAVLTree::getHeight(ChildAVLNode* node) 
{
    if (node == nullptr)
        return 0;
    return node->height;
}

int ChildAVLTree::getBalance(ChildAVLNode* node) 
{
    if (node == nullptr)
        return 0;
    return getHeight(node->left) - getHeight(node->right);
}


ChildAVLNode* ChildAVLTree::rightRotate(ChildAVLNode* y) 
{
    ChildAVLNode* x = y->left;
    ChildAVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = 1 + max(getHeight(y->left), getHeight(y->right));
    x->height = 1 + max(getHeight(x->left), getHeight(x->right));

    return x; 
}

ChildAVLNode* ChildAVLTree::leftRotate(ChildAVLNode* x) {
    ChildAVLNode* y = x->right;
    ChildAVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = 1 + max(getHeight(x->left), getHeight(x->right));
    y->height = 1 + max(getHeight(y->left), getHeight(y->right));

    return y; 
}

ChildAVLNode* ChildAVLTree::insert(ChildAVLNode* node, FSTreeNode* fsNode) {

    if (node == nullptr) 
    {
        return new ChildAVLNode(fsNode);
    }

    int cmp = strcmp(fsNode->metadata.name, node->fsNode->metadata.name);

    if (cmp < 0)
        node->left = insert(node->left, fsNode);
    else if (cmp > 0)
        node->right = insert(node->right, fsNode);
    else {
        cerr << "Error: Node " << fsNode->metadata.name << " already exists in this directory." << endl;
        return node;
    }

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getBalance(node);

    if (balance > 1 && strcmp(fsNode->metadata.name, node->left->fsNode->metadata.name) < 0)
        return rightRotate(node);

    if (balance < -1 && strcmp(fsNode->metadata.name, node->right->fsNode->metadata.name) > 0)
        return leftRotate(node);

    if (balance > 1 && strcmp(fsNode->metadata.name, node->left->fsNode->metadata.name) > 0) 
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && strcmp(fsNode->metadata.name, node->right->fsNode->metadata.name) < 0) 
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

ChildAVLNode* ChildAVLTree::find(ChildAVLNode* node, const string& name) 
{
    if (node == nullptr) {
        return nullptr;
    }

    int cmp = strcmp(name.c_str(), node->fsNode->metadata.name);

    if (cmp == 0)
        return node;
    if (cmp < 0)
        return find(node->left, name);
    else
        return find(node->right, name);
}

ChildAVLNode* ChildAVLTree::minValueNode(ChildAVLNode* node) 
{
    ChildAVLNode* current = node;
    while (current->left != nullptr)
        current = current->left;
    return current;
}

ChildAVLNode* ChildAVLTree::remove(ChildAVLNode* node, const string& name) 
{
    if (node == nullptr) return node;

    int cmp = strcmp(name.c_str(), node->fsNode->metadata.name);

    if (cmp < 0)
        node->left = remove(node->left, name);
    else if (cmp > 0)
        node->right = remove(node->right, name);
    else 
    {

        if (node->left == nullptr || node->right == nullptr) {
            ChildAVLNode* temp = node->left ? node->left : node->right;
            if (temp == nullptr) 
            {
                temp = node;
                node = nullptr;
            } 
            else 
            {
                *node = *temp; 
            }
            delete temp;
        } 
        else 
        {
            ChildAVLNode* temp = minValueNode(node->right);
            node->fsNode = temp->fsNode;
            node->right = remove(node->right, temp->fsNode->metadata.name);
        }
    }

    if (node == nullptr)
        return node;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getBalance(node);

    if (balance > 1 && getBalance(node->left) >= 0)
        return rightRotate(node);
    if (balance > 1 && getBalance(node->left) < 0) 
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && getBalance(node->right) <= 0)
        return leftRotate(node);
    if (balance < -1 && getBalance(node->right) > 0) 
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void ChildAVLTree::inOrder(ChildAVLNode* node, vector<FSTreeNode*>& nodes) 
{
    if (node == nullptr)
        return;
    
    inOrder(node->left, nodes);
    nodes.push_back(node->fsNode);
    inOrder(node->right, nodes);
}