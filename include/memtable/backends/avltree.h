#ifndef __AVLTREE_H
#define __AVLTREE_H

#include <algorithm>
#include <optional>
#include <variant>
#include <vector>

#include "engine.h"

namespace avl {

    class AVLTree : public memtable_generic::MemTable{
    private:
        struct AVLNode {
            std::pair<uint64_t,std::string> elem;
            uint64_t height;
            AVLNode *left;
            AVLNode *right;

            AVLNode(const uint64_t &key, const std::string &value) {
                left = nullptr;
                right = nullptr;
                elem = std::make_pair(key, value);
                height = 1;
            }
        };

        AVLNode *root;

        uint64_t height(const AVLNode *node) noexcept {
            return node == nullptr ? 0 : node->height;
        }
        void update_height(AVLNode *root) noexcept {
            if (root != nullptr)
                root->height = std::max(height(root->left), height(root->right)) + 1;
        }
        uint64_t balance(const AVLNode *root) noexcept {
            return height(root->left) - height(root->right);
        }

        AVLNode *left_rotation(AVLNode *root) noexcept {
            AVLNode *right = root->right;
            root->right = right->left;
            right->left = root;
            update_height(root);
            update_height(right);
            return right;
        }

        AVLNode *right_rotation(AVLNode *root) noexcept {
            AVLNode *left = root->left;
            root->left = left->right;
            left->right = root;
            update_height(root);
            update_height(left);
            return left;
        }
        AVLNode *LLUtil(AVLNode *root) noexcept {
            return right_rotation(root);
        }

        AVLNode *LRUtil(AVLNode *root) noexcept {
            root->left = left_rotation(root->left);
            return right_rotation(root);
        }

        AVLNode *RLUtil(AVLNode *root) noexcept {
            root->right = right_rotation(root->right);
            return left_rotation(root);
        }

        AVLNode *RRUtil(AVLNode *root) noexcept {
            return left_rotation(root);
        }

        AVLNode *insertUtil(AVLNode *root, const uint64_t &key, const std::string &value) noexcept {

            if (root == nullptr) {
                this->nr_size += sizeof(uint64_t) +sizeof(size_t) + value.size();
                return new AVLNode(key, value);
            }

            if (key == root->elem.first){
                uint64_t prev_size = root->elem.second.size();
                uint64_t cur_size = value.size();
                this->nr_size += (cur_size - prev_size);
                root->elem.second = value;
            }

            else if (key < root->elem.first)
                root->left = insertUtil(root->left, key, value);
            else
                root->right = insertUtil(root->right, key, value);
            update_height(root);
            return adjust(root);
        }

        const AVLNode *searchUtil(const AVLNode *root, const uint64_t &key) const noexcept {
            if (root == nullptr || root->elem.first == key)
                return root;
            else if (key < root->elem.first)
                return searchUtil(root->left, key);
            else
                return searchUtil(root->right, key);
        }

        void dumpUtil(const AVLNode *root, std::vector<std::pair<uint64_t,std::string>> &block) noexcept {
            if (root == nullptr)
                return;
            dumpUtil(root->left, block);
            block.push_back(root->elem);
            dumpUtil(root->right, block);
            delete root;
        }

        AVLNode *adjust(AVLNode *root) noexcept {
            if (balance(root) >= 2) {
                if (balance(root->left) > 0)
                    return LLUtil(root);
                else
                    return LRUtil(root);
            } else if (balance(root) <= -2) {
                if (balance(root->right) > 0)
                    return RLUtil(root);
                else
                    return RRUtil(root);
            }

            return root;
        }

        void deleteUtil(const AVLNode *root){
            if(root == nullptr)
                return;
            deleteUtil(root->left);
            deleteUtil(root->right);
            delete root;
        }

    public:
        AVLTree() {
            this->root = nullptr;
        }

        ~AVLTree(){
            this->reset();
        }

        void remove(const uint64_t &key) noexcept {
            this->root = insertUtil(this->root, key, memtable_generic::deleted);
        }

        void insert(const uint64_t &key, const std::string &value) noexcept {
            this->root = insertUtil(this->root, key, value);
        }

        std::string search(const uint64_t &key) const noexcept {
            const AVLNode *node = searchUtil(this->root, key);
            if (node == nullptr)
                return "";
            else
                return node->elem.second;
        }

        std::vector<std::pair<uint64_t,std::string>> dump() noexcept {
            auto ret = std::vector<std::pair<uint64_t,std::string>>();
            dumpUtil(this->root, ret);
            this->nr_size = 0;
            this->root = nullptr;
            return ret;
        }

        size_t size() const noexcept {
            return this->nr_size;
        }

        void reset() noexcept {
            deleteUtil(this->root);
            this->root = nullptr;
            this->nr_size = 0;
        }
    };
};

#endif
