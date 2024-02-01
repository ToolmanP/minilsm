#ifndef __SSTABLE_H
#define __SSTABLE_H

#include <utils/bloomfilter.h>

#include <algorithm>
#include <queue>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sstable {
const std::string deleted = "~DELETED~";
enum Policy { TIERING = 0, LEVELING = 1 };
enum Order { PREV = 0, NEXT = 1 };
struct SSBlockHeader {
  uint64_t timestamp;
  uint64_t nr_keys;
  uint64_t minn;
  uint64_t maxx;
  bool checkBound(uint64_t key){
    return key >= minn && key <= maxx;
  }
};

const uint64_t BLOOMFILTER_SIZE = 10240;
const int SSBLOCK_RESERVED_SIZE = sizeof(SSBlockHeader) + BLOOMFILTER_SIZE;

class SSBlock {
private:
  SSBlockHeader header;
  std::unique_ptr<bloomfilter::BloomFilter<uint64_t>> filter;
  std::deque<std::pair<uint64_t, uint64_t>> index;
  const std::string filename;
  bool is_prepared;

  void prepare_from_block(
      const std::vector<std::pair<uint64_t, std::string>>
          &block);
  void prepare_from_file();
  std::string read(uint64_t offset, size_t size);

public:
  SSBlock(const std::string &filename);
  ~SSBlock();
  void flush(const std::vector<std::pair<uint64_t, std::string>>
                 &block);
  uint64_t timestamp() const;
  uint64_t min() const;
  uint64_t max() const;
  uint64_t top_key() const;
  std::pair<uint64_t,std::string> top();
  uint64_t size() const;
  void pop();
  const std::string &getFilename() const;
  std::string search(const uint64_t key);
};

class SSLevel {
private:
  std::string base;
  std::deque<std::unique_ptr<SSBlock>> blocks;
  Policy policy;
  size_t limit;

public:
  SSLevel(const std::string &base, const Policy &policy, const size_t &limit);
  void
  insertBlock(const std::vector<std::pair<uint64_t, std::string>>
                  &block);
  void insertBlocks(const std::vector<std::unique_ptr<SSBlock>> &blocks);
  std::string nextFile() const;
  size_t getLimit() const;
  size_t size() const;
  std::vector<std::unique_ptr<SSBlock>> select(Order order, uint64_t minn,
                                               uint64_t maxx);
  std::string search(const uint64_t key);
};

class SSTable {
private:
  std::string base;
  std::string conf;
  std::vector<std::unique_ptr<SSLevel>> levels;
  std::vector<std::pair<Policy, size_t>> parseConf();
  std::pair<uint64_t, uint64_t> rangeSelected(
      const std::vector<std::unique_ptr<sstable::SSBlock>> &selected) const;
  void compactBlocks(
      const std::vector<std::unique_ptr<sstable::SSBlock>> &selected,
      const std::unique_ptr<SSLevel> &level) const;
  void prepare_levels();

public:
  SSTable(const std::string &base, const std::string &conf);
  // ~SSTable();
  void flush(const std::vector<std::pair<uint64_t, std::string>>
                 &block);
  std::string search(const uint64_t key);
  void reset();
  void compact();
  // TODO: implement scan here....
};
}; // namespace sstable

#endif
