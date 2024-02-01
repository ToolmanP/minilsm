#ifndef __SKIPLIST_H

#include <random>

#include "engine.h"

namespace skl {
    class SkipList : public memtable_generic::MemTable {
    private:
        struct SkipNode {
            std::pair<uint64_t,std::string> elem;
            std::vector<SkipNode *> forward;
            SkipNode(const uint64_t &key, const std::string &value, uint64_t maxlevels) {
                this->elem = std::make_pair(key, value);
                this->forward = decltype(this->forward)(maxlevels, nullptr);
            }

            SkipNode(const uint64_t maxlevels) {
                this->forward = decltype(this->forward)(maxlevels, nullptr);
            }
        };

        const uint64_t maxlevels;
        uint64_t levels = 0;
        SkipNode *header;
        std::bernoulli_distribution exp;
        std::random_device rd{};
        std::mt19937_64 gen;

        uint64_t roll_dice() {
            uint64_t next_level = 1;
            while (next_level < maxlevels) {
                auto c = this->exp(this->gen);
                if (c == 0)
                    break;
                next_level += 1;
            }

            return next_level;
        }

        void insertUtil(const uint64_t &key, const std::string &value) {
            auto target = roll_dice();
            auto updates = decltype(header->forward)(this->maxlevels, nullptr);
            auto current = this->header;

            for (uint64_t i = this->levels - 1; i >= 0; i--) {
                while (current->forward[i] != nullptr && current->forward[i]->elem.first < key)
                    current = current->forward[i];
                updates[i] = current;
            }

            current = current->forward[0];

            if (current == nullptr || current->elem.first != key) {
                if (target > this->levels) {
                    for (uint64_t i = this->levels; i < target; i++)
                        updates[i] = header;
                    this->levels = target;
                }

                auto node = new SkipNode(key, value, this->maxlevels);
                this->nr_size += (sizeof(uint64_t) + sizeof(size_t));
                if (value != memtable_generic::deleted) {
                    this->nr_size += value.size();
                }

                for (uint64_t i = 0; i < target; i++) {
                    node->forward[i] = updates[i]->forward[i];
                    updates[i]->forward[i] = node;
                }

            } else {
                uint64_t prev_size = current->elem.second.size();
                uint64_t cur_size = value.size();
                this->nr_size += (cur_size - prev_size);
                current->elem.second = value;
            }
        }

        void dumpUtil(const SkipNode *node, std::vector<std::pair<uint64_t,std::string>> &block) {
            if (node == nullptr)
                return;
            dumpUtil(node->forward[0], block);
            block.push_back(node->elem);
            delete node;
        }

        void deleteUtil(const SkipNode *node) {
            if (node == nullptr)
                return;
            deleteUtil(node->forward[0]);
            delete node;
        }

    public:
        SkipList(const uint64_t maxlevels, const double p) : maxlevels(maxlevels) {
            this->gen = std::mt19937_64(rd());
            this->header = new SkipNode(maxlevels);
            this->exp = std::bernoulli_distribution(p);
        }

        ~SkipList() {
            this->reset();
            delete this->header;
        }

        void remove(const uint64_t &key) noexcept {
            this->insertUtil(key, memtable_generic::deleted);
        }

        void insert(const uint64_t &key, const std::string &value) noexcept {
            this->insertUtil(key, value);
        }

        std::string search(const uint64_t &key) const noexcept {
            auto current = this->header;
            for (uint64_t i = this->levels - 1; i >= 0; i--) {
                while (current->forward[i] != nullptr &&
                       current->forward[i]->elem.first <= key) {
                    current = current->forward[i];
                }
                if (current->elem.first == key)
                    return current->elem.second;
            }
            return "";
        }

        std::vector<std::pair<uint64_t, std::string>> dump() noexcept {
            auto ret = std::vector<std::pair<uint64_t, std::string>>();
            dumpUtil(this->header->forward[0], ret);
            this->nr_size = 0;
            return ret;
        }

        size_t size() const noexcept {
            return this->nr_size;
        }

        void reset() noexcept {
            deleteUtil(this->header->forward[0]);
            delete this->header;
            this->header = new SkipNode(maxlevels);
            this->nr_size = 0;
        }
    };
};

#endif
