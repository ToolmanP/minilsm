#ifndef __RBTREE_H

#include <cassert>
#include <optional>
#include <type_traits>
#include <vector>

#include "engine.h"

namespace rb
{
    class RBTree : public memtable_generic::MemTable
    {
    private:
        enum RBColor
        {
            BLACK,
            RED
        };

        struct RBNode
        {
            enum RBColor color;
            std::pair<uint64_t, std::string> elem;
            uint64_t height;
            RBNode *left, *right;

            RBNode(const uint64_t &key, const std::string &value, enum RBColor color) noexcept
            {
                this->elem = std::make_pair(key, value);
                this->color = color;
                this->left = nullptr;
                this->right = nullptr;
                this->height = 1;
            }
        };

        RBNode *root;
        enum RBColor color(const RBNode *node)
        {
            return node == nullptr ? BLACK : node->color;
        }

        void flip(RBNode *node) noexcept
        {
            node->color = (node->color == BLACK) ? RED : BLACK;
        }

        uint64_t height(const RBNode *node) noexcept
        {
            return node == nullptr ? 0 : node->height;
        }

        void update_height(RBNode *node) noexcept
        {
            node->height = std::max(height(node->left), height(node->right)) + 1;
        }

        RBNode *left_rotation(RBNode *root) noexcept
        {
            RBNode *right = root->right;
            root->right = right->left;
            right->left = root;
            update_height(root);
            update_height(right);
            return right;
        }

        RBNode *right_rotation(RBNode *root) noexcept
        {
            RBNode *left = root->left;
            root->left = left->right;
            left->right = root;
            update_height(root);
            update_height(left);
            return left;
        }

        RBNode *LLUtil(RBNode *root) noexcept
        {
            root = right_rotation(root);
            flip(root);
            flip(root->right);
            return root;
        }

        RBNode *LRUtil(RBNode *root) noexcept
        {
            root->left = left_rotation(root->left);
            root = right_rotation(root);
            flip(root);
            flip(root->right);
            return root;
        }

        RBNode *RLUtil(RBNode *root) noexcept
        {
            root->right = right_rotation(root->right);
            root = left_rotation(root);
            flip(root);
            flip(root->left);
            return root;
        }

        RBNode *RRUtil(RBNode *root) noexcept
        {
            root = left_rotation(root);
            flip(root);
            flip(root->left);
            return root;
        }

        const RBNode *searchUtil(const RBNode *root, const uint64_t &key) const noexcept
        {
            if (root == nullptr || root->elem.first == key)
                return root;
            else if (key < root->elem.first)
                return searchUtil(root->left, key);
            else
                return searchUtil(root->right, key);
        }

        RBNode *adjust(RBNode *root) noexcept
        {
            enum RBColor lc = color(root->left);
            enum RBColor rc = color(root->right);

            if (lc == RED && rc == BLACK)
            {
                if (color(root->left->left) == RED)
                    return LLUtil(root);
                else if (color(root->left->right) == RED)
                    return LRUtil(root);
            }
            else if (lc == BLACK && rc == RED)
            {
                if (color(root->right->left) == RED)
                    return RLUtil(root);
                else if (color(root->right->right) == RED)
                    return RRUtil(root);
            }
            else if (lc == BLACK && rc == BLACK)
            {
                return root;
            }
            else
            {
                flip(root);
                flip(root->left);
                flip(root->right);
                return root;
            }
            return root;
        }

        RBNode *insertUtil(RBNode *root, const uint64_t &key, const std::string &value)
        {

            if (root == nullptr)
            {
                enum RBColor color = RED;

                if (this->nr_size == 0)
                    color = BLACK;

                this->nr_size += (sizeof(uint64_t) + sizeof(size_t));
                if (value != memtable_generic::deleted)
                {
                    this->nr_size += value.size();
                }

                return new RBNode(key, value, color);
            }

            if (key == root->elem.first)
            {
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

        void dumpUtil(const RBNode *root, std::vector<std::pair<uint64_t, std::string>> &block) noexcept
        {
            if (root == nullptr)
                return;
            dumpUtil(root->left, block);
            block.push_back(root->elem);
            dumpUtil(root->right, block);
            delete root;
        }

        void deleteUtil(const RBNode *root)
        {
            if (root == nullptr)
                return;
            deleteUtil(root->left);
            deleteUtil(root->right);
            delete root;
        }

    public:
        RBTree()
        {
            this->root = nullptr;
            this->nr_size = 0;
        }

        ~RBTree()
        {
            this->reset();
        }

        void insert(const uint64_t &key, const std::string &value) noexcept
        {
            this->root = insertUtil(this->root, key, value);
        }

        void remove(const uint64_t &key) noexcept
        {
            this->root = insertUtil(this->root, key, memtable_generic::deleted);
        }

        std::string search(const uint64_t &key) const noexcept
        {
            const RBNode *node = this->searchUtil(this->root, key);
            if (node == nullptr)
                return "";
            else
                return node->elem.second;
        }

        std::vector<std::pair<uint64_t,std::string>> dump() noexcept
        {
            auto ret = std::vector<std::pair<uint64_t,std::string>>();
            dumpUtil(this->root, ret);
            this->nr_size = 0;
            this->root = nullptr;
            return ret;
        }

        size_t size() const noexcept
        {
            return this->nr_size;
        }

        void reset() noexcept
        {
            deleteUtil(this->root);
            this->root = nullptr;
            this->nr_size = 0;
        }
    };
};
#endif
