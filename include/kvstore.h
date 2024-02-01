#ifndef __KVSTORE_H

#define __KVSTORE_H

#include <kvstore_api.h>

#include <memtable/memtable.h>
#include <sstable/sstable.h>

#include <memory>
#include <string>
namespace kvstore {
    const std::string deleted = "~DELETED~";
    const size_t MAX_CAPACITY = 2 * 1024 * 1024 - sstable::SSBLOCK_RESERVED_SIZE;

    class KVStore : KVStoreAPI {
    private:
        std::unique_ptr<memtable::MemTable> mtable;
        std::unique_ptr<sstable::SSTable> stable;
        
    public:
        KVStore(const std::string &dir,const std::string &conf = "../conf/default.conf"): KVStoreAPI(dir){
            this->mtable = std::make_unique<memtable::RBTree>();
            this->stable = std::make_unique<sstable::SSTable>(dir,conf);
        }
        ~KVStore(){
            this->mtable.reset();
            this->stable.reset();
        }

        void put(const uint64_t key, const std::string &s) override;
        std::string get(const uint64_t key) override;
        bool del(const uint64_t key) override;
        void reset() override;
        void scan(const uint64_t key1, const uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list) override;
        void flush();
    };
};

#endif
