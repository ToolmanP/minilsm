#include "utils.h"

#include <filesystem>
#include <fstream>
#include <kvstore.h>
#include <sstable/sstable.h>
#include <unordered_set>


sstable::SSTable::SSTable(const std::string &base, const std::string &conf) {
  this->base = base;
  this->conf = conf;
  this->levels = std::vector<std::unique_ptr<SSLevel>>();
  this->prepare_levels();
}

std::vector<std::pair<sstable::Policy, size_t>> sstable::SSTable::parseConf() {
  std::vector<std::pair<Policy, size_t>> ret;
  std::ifstream ifile(this->conf);
  int id;
  size_t limit;
  std::string mode;
  while (ifile >> id >> limit >> mode) {
    if (mode == "Leveling")
      ret.push_back(std::make_pair(LEVELING, limit));
    else
      ret.push_back(std::make_pair(TIERING, limit));
  }
  return ret;
}

void sstable::SSTable::prepare_levels() {

  if (utils::dirExists(this->base))
    utils::mkdir(this->base.c_str());

  auto dirs = std::vector<std::string>();
  utils::scanDir(this->base, dirs);

  std::sort(dirs.begin(), dirs.end());

  auto config = this->parseConf();

  if (dirs.size() < config.size()) {
    for (auto i = dirs.size(); i < config.size(); i++) {
      utils::mkdir((this->base + "/level-" + std::to_string(i)).c_str());
      dirs.emplace_back(this->base + "/level-" + std::to_string(i));
    }
  } else
    std::sort(dirs.begin(), dirs.end());

  for (size_t i = 0; i < config.size(); i++) {
    auto policy = config[i].first;
    auto limit = config[i].second;
    this->levels.emplace_back(
        std::make_unique<SSLevel>(dirs[i], policy, limit));
  }
}

void sstable::SSTable::flush(
    const std::vector<std::pair<uint64_t, std::string>> &block) {
  this->levels[0]->insertBlock(block);
  if (this->levels[0]->getLimit() >= this->levels[0]->size())
    this->compact();
}

std::string sstable::SSTable::search(const uint64_t key) {
  for (auto &level : this->levels) {
    auto ret = level->search(key);
    if (ret != "")
      return ret;
  }
  return "";
}

void sstable::SSTable::reset() {
  utils::rmdir(this->base.c_str());
  this->levels.clear();
  this->prepare_levels();
}

std::pair<uint64_t, uint64_t> sstable::SSTable::rangeSelected(
    const std::vector<std::unique_ptr<sstable::SSBlock>> &selected) const {
  std::pair<uint64_t, uint64_t> ret{(size_t)-1, 0};
  for (auto const &b : selected) {
    if (b->max() > ret.second)
      ret.second = b->max();
    if (b->min() < ret.first)
      ret.first = b->min();
  }
  return ret;
}

void sstable::SSTable::compactBlocks(
    const std::vector<std::unique_ptr<sstable::SSBlock>> &selected,
    const std::unique_ptr<SSLevel> &level) const {

  using pq_node = std::tuple<size_t, uint64_t, uint64_t>;
  auto compare = [](pq_node &lhs, pq_node &rhs) {
    if (std::get<1>(lhs) == std::get<1>(rhs))
      return std::get<2>(lhs) < std::get<2>(rhs);
    return std::get<1>(lhs) > std::get<1>(rhs);
  };

  std::priority_queue<pq_node, std::vector<pq_node>, decltype(compare)> pq(
      compare); // <index, timestamp, key>
  std::unordered_set<uint64_t> record;
  std::vector<std::pair<uint64_t, std::string>> temp;
  std::vector<std::unique_ptr<SSBlock>> ret;
  size_t capacity = 0;

  for (size_t i = 0; i < selected.size(); i++) {
    pq.push(
        std::make_tuple(i, selected[i]->top_key(), selected[i]->timestamp()));
  }

  while (!pq.empty()) {
    auto node = pq.top();
    auto i = std::get<0>(node);
    auto key = std::get<1>(node);
    auto timestamp = std::get<2>(node);
    pq.pop();

    if (record.find(key) == record.end()) {
      auto kv = selected[i]->top();

      if(kv.second != deleted){
        capacity += 2 * sizeof(uint64_t) +  kv.second.size();
        temp.push_back(kv);
      }
        
      record.insert(key);
    }

    selected[i]->pop();

    if (selected[i]->size() != 0)
      pq.push(
          std::make_tuple(i, selected[i]->top_key(), selected[i]->timestamp()));

    if (capacity >= kvstore::MAX_CAPACITY) {
      level->insertBlock(temp);
      temp.clear();
      capacity = 0;
    }
  }
  if (!temp.empty())
    level->insertBlock(temp);

  for (auto const &b : selected)
    utils::rmfile(b->getFilename().c_str());
}

void sstable::SSTable::compact() {
  for (size_t i = 0; i < this->levels.size() - 1 &&
                     this->levels[i]->size() >= this->levels[i]->getLimit();
       i++) {
    auto selected = this->levels[i]->select(PREV, -1, -1);
    auto range = rangeSelected(selected);
    auto minn = range.first;
    auto maxx = range.second;
    auto selected_next = this->levels[i + 1]->select(NEXT, minn, maxx);
    selected.insert(selected.end(),
                    std::make_move_iterator(selected_next.begin()),
                    std::make_move_iterator(selected_next.end()));
    this->compactBlocks(selected, this->levels[i + 1]);
  }
}
