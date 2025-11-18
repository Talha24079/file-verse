#pragma once
#include <string>
#include <vector>
#include "../include/odf_types.hpp" 

using namespace std;

struct Node {
    UserInfo user;
    Node *left;
    Node *right;
    int height; 
    Node(const UserInfo& userInfo) : 
        user(userInfo), left(nullptr), right(nullptr), height(1) {}
};

class UserAVLTree {
private:
    Node *root;

    Node* insert(Node* node, const UserInfo& user);
    Node* find(Node* node, const string& username);
    Node* remove(Node* node, const string& username);
    void inOrder(Node* node, vector<UserInfo>& userList);
    int getHeight(Node* node);
    int getBalance(Node* node);
    Node* rightRotate(Node* y);
    Node* leftRotate(Node* x);
    Node* minValueNode(Node* node);

public:
    UserAVLTree() : root(nullptr) {}
    void insert(const UserInfo& user);
    UserInfo* find(const string& username);
    void remove(const string& username);
    vector<UserInfo> listAllUsers();
};