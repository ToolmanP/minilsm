#include "utils.h"
#include <sstable/sstable.h>
#include <chrono>


sstable::SSLevel::SSLevel(const std::string &base, const Policy &policy, const size_t &limit) {
    this->base = base;
    this->policy = policy;
    this->limit = limit;
    this->blocks = std::deque<std::unique_ptr<SSBlock>>();

    auto blockfiles = std::vector<std::string>();
    utils::scanDir(this->base,blockfiles);
    
    for (const auto &blockfile : blockfiles) {
        if(blockfile.find("block") == 0)
          this->blocks.emplace_back(std::make_unique<SSBlock>(blockfile));
    }

    std::sort(this->blocks.begin(),this->blocks.end(),[](auto &a,auto &b){
        if(a->timestamp() == b->timestamp()){
          return a->min() < b->min();
        }
        return a->timestamp() < b->timestamp();
    });

}

std::string sstable::SSLevel::nextFile() const{
  return this->base + "/block-" + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count()) + ".sst"; 
}

void sstable::SSLevel::insertBlock(const std::vector<std::pair<uint64_t, std::string>> &block) {
    std::string blockfile = this->nextFile();
    auto newblock = std::make_unique<SSBlock>(blockfile);
    this->blocks.push_back(std::move(newblock));
    (*(this->blocks).rbegin())->flush(block);
}

size_t sstable::SSLevel::getLimit() const {
  return this->limit;
}

size_t sstable::SSLevel::size() const {
  return this->blocks.size();
}

std::string sstable::SSLevel::search(const uint64_t key) {
    for (auto block = this->blocks.rbegin(); block != this->blocks.rend(); block++) {
        auto ret = (*block)->search(key);
        if (ret != "")
            return ret;
    }
    return "";
}

std::vector<std::unique_ptr<sstable::SSBlock>> sstable::SSLevel::select(sstable::Order order,uint64_t minn, uint64_t maxx){
  std::vector<std::unique_ptr<sstable::SSBlock>> ret{};
  if(order == PREV){
    if(this->policy == TIERING){
      while(!this->blocks.empty()){
        ret.push_back(std::move(this->blocks.front()));
        this->blocks.pop_front();
      }
    }
    else{
      while(this->blocks.size() > this->limit){
        ret.push_back(std::move(this->blocks.front()));
        this->blocks.pop_front();
      } 
    }
  } else {
    if(this->policy == LEVELING){
      std::deque<std::unique_ptr<sstable::SSBlock>> next{};
      while(!this->blocks.empty()){
        auto block = std::move(this->blocks.front());
        if(block->min() >= minn || block->max() <= maxx)
          ret.push_back(std::move(block));
        else
          next.push_back(std::move(block));
        this->blocks.pop_front();
      }
      this->blocks = std::move(next);
    }
  }
  return ret;
}


