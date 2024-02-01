#ifndef __KVMEM_H
#define __KVMEM_H

#include <optional>
#include <vector>

namespace memtable_generic {
    const std::string deleted = "~DELETED~";
    class MemTable {
    protected:
      size_t nr_size = 0;
    public:
        virtual ~MemTable(){}
        virtual size_t size() const noexcept = 0;
        virtual void remove(const uint64_t &key) noexcept = 0;
        virtual void insert(const uint64_t &key, const std::string &value) noexcept = 0;
        virtual std::string search(const uint64_t &key) const noexcept = 0;
        virtual std::vector<std::pair<uint64_t,std::string>> dump() noexcept = 0;
        virtual void reset() noexcept = 0;
    };
};

#endif
