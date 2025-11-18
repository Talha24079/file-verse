#include "user_avl_tree.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

using namespace std;

Node* UserAVLTree::leftRotate(Node* x) 
{
    Node* y = x->right;
    Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;
    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;

    return y;
}

Node* UserAVLTree::rightRotate(Node* y) 
{
    Node* x = y->left;
    Node* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(getHeight(y->left), getHeight(y->right)) + 1;
    x->height = max(getHeight(x->left), getHeight(x->right)) + 1;

    return x;
}

int UserAVLTree::getHeight(Node* node) 
{
    return node ? node->height : 0;
}

int UserAVLTree::getBalance(Node* node) 
{
    return node ? getHeight(node->left) - getHeight(node->right) : 0;
}

Node* UserAVLTree::minValueNode(Node* node) 
{
    Node* current = node;
    while (current->left != nullptr)
        current = current->left;
    return current;
}

Node* UserAVLTree::insert(Node* node, const UserInfo& user) 
{
    if (!node)
        return new Node(user);

    int cmp = strcmp(user.username, node->user.username);
    if (cmp < 0) 
        node->left = insert(node->left, user);
    else if (cmp > 0)
        node->right = insert(node->right, user);
    else
        return node; 

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getBalance(node);

    if (balance > 1 && strcmp(user.username, node->left->user.username) < 0)
        return rightRotate(node);

    if (balance < -1 && strcmp(user.username, node->right->user.username) > 0)
        return leftRotate(node);

    if (balance > 1 && strcmp(user.username, node->left->user.username) > 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && strcmp(user.username, node->right->user.username) < 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void UserAVLTree::insert(const UserInfo& user) 
{
    root = insert(root, user);
}

UserInfo* UserAVLTree::find(const string& username) 
{
    Node* resultNode = find(root, username);
    return resultNode ? &resultNode->user : nullptr;
}

Node* UserAVLTree::find(Node* node, const string& username) 
{
    if (!node)
        return nullptr;

    int cmp = strcmp(username.c_str(), node->user.username);

    if (cmp == 0)
        return node;
    else if (cmp < 0)
        return find(node->left, username);
    else
        return find(node->right, username);
}

Node* UserAVLTree::remove(Node* node, const string& username) 
{
    if (!node)
        return node;

    int cmp = strcmp(username.c_str(), node->user.username);

    if (cmp < 0)
        node->left = remove(node->left, username);
    else if (cmp > 0)
        node->right = remove(node->right, username);
    else {
        if ((!node->left) || (!node->right)) {
            Node* temp = node->left ? node->left : node->right;

            if (!temp) {
                temp = node;
                node = nullptr;
            } else
                *node = *temp;

            delete temp;
        } else {
            Node* temp = minValueNode(node->right);
            node->user = temp->user;
            node->right = remove(node->right, temp->user.username);
        }
    }

    if (!node)
        return node;

    node->height = 1 + max(getHeight(node->left), getHeight(node->right));

    int balance = getBalance(node);

    if (balance > 1 && getBalance(node->left) >= 0)
        return rightRotate(node);

    if (balance > 1 && getBalance(node->left) < 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && getBalance(node->right) <= 0)
        return leftRotate(node);

    if (balance < -1 && getBalance(node->right) > 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void UserAVLTree::remove(const string& username) 
{
    root = remove(root, username);
}

void UserAVLTree::inOrder(Node* node, vector<UserInfo>& userList) 
{
    if (node) {
        inOrder(node->left, userList);
        userList.push_back(node->user);
        inOrder(node->right, userList);
    }
}

vector<UserInfo> UserAVLTree::listAllUsers() 
{
    vector<UserInfo> userList;
    inOrder(root, userList);
    return userList;
}