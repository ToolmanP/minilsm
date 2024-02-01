#include "utils.h"

#include <sstable/sstable.h>

#include <chrono>
#include <iostream>
#include <fstream>

sstable::SSBlock::SSBlock(const std::string &filename):filename(filename){
  
  this->header = {};
  this->filter = std::make_unique<bloomfilter::BloomFilter<uint64_t>>(BLOOMFILTER_SIZE);
  this->index = {};
  this->is_prepared = false;
  if(utils::dirExists(filename) == true)
    this->prepare_from_file();
}


sstable::SSBlock::~SSBlock() { 
  this->filter.reset();
}

uint64_t sstable::SSBlock::timestamp() const{
  return this->header.timestamp;
}

const std::string & sstable::SSBlock::getFilename() const{
  return this->filename;
}

uint64_t sstable::SSBlock::min() const {
  return this->header.minn;
}

uint64_t sstable::SSBlock::max() const {
  return this->header.maxx;
}

void sstable::SSBlock::prepare_from_block(
    const std::vector<std::pair<uint64_t, std::string>> &block) {
  this->header.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  this->header.nr_keys = block.size();
  this->header.minn = block.begin()->first;
  this->header.maxx = (block.end()-1)->first;
  uint64_t offset = SSBLOCK_RESERVED_SIZE+block.size() * 2 * sizeof(uint64_t);

  for (const auto &p : block) {
    this->index.push_back(std::make_pair(p.first, offset));
    offset += p.second.size();
    this->filter->insert(p.first);
  }
  this->is_prepared = true;
}
void sstable::SSBlock::flush(
    const std::vector<std::pair<uint64_t, std::string>> &block) {
  
  
  std::ofstream ofile(this->filename,std::ios::binary | std::ios::out);
  this->prepare_from_block(block);

  ofile.write(reinterpret_cast<const char *>(&this->header),
              sizeof(this->header));
  ofile.write(reinterpret_cast<const char *>(this->filter->data),
              BLOOMFILTER_SIZE);
  for (const auto &p : this->index) {
    ofile.write(reinterpret_cast<const char *>(&p.first), sizeof(uint64_t));
    ofile.write(reinterpret_cast<const char *>(&p.second), sizeof(uint64_t));
  }

  for (const auto &p : block) {
      ofile.write(p.second.data(), p.second.length());
  }
  ofile.close();
}

std::string sstable::SSBlock::read(uint64_t offset,
                                                  size_t size){
  std::ifstream ifile(this->filename,std::ios::binary | std::ios::in);

  if (size == (size_t)-1) {
    ifile.seekg(0, std::ios::end);
    size = ifile.tellg();
    size -= offset;
  }

  std::string ret(size, 0);
  ifile.seekg(offset);
  ifile.read(&ret[0], size);
  ifile.close();
  return ret;
}

void sstable::SSBlock::prepare_from_file() {
  std::ifstream ifile(this->filename);
  ifile.read(reinterpret_cast<char *>(&this->header), sizeof(this->header));
  ifile.read(this->filter->data, BLOOMFILTER_SIZE);
  for (uint64_t i = 0; i < this->header.nr_keys; i++) {
    std::pair<uint64_t, uint64_t> p;
    ifile.read(reinterpret_cast<char *>(&p.first), sizeof(uint64_t));
    ifile.read(reinterpret_cast<char *>(&p.second), sizeof(uint64_t));
    this->index.push_back(p);
  }
  this->is_prepared = true;
}

std::string sstable::SSBlock::search(const uint64_t key) {

  if(this->header.checkBound(key) == false || this->filter->check(key) == false)
    return "";


  auto range = std::equal_range(
      this->index.begin(), this->index.end(), std::make_pair(key, 0),
      [](auto p1, auto p2) -> bool { return p1.first < p2.first; });

  if (range.first == range.second) {
    return "";
  } else {

    size_t size = (size_t)-1;
    uint64_t offset = (*range.first).second;

    if (range.second != this->index.end())
      size = (range.first + 1)->second - range.first->second;

    return this->read(offset,size);
  }
}

uint64_t sstable::SSBlock::top_key() const {
  return this->index.front().first;
}

uint64_t sstable::SSBlock::size() const {
  return this->index.size();
}

std::pair<uint64_t,std::string> sstable::SSBlock::top() {

  size_t size = (size_t)-1;
  auto key = this->index.front().first;
  auto offset = this->index.front().second;
  
  if(this->index.size() != 1)
    size = (this->index.begin()+1)->second - this->index.begin()->second;

  return std::make_pair(key,this->read(offset,size));
}

void sstable::SSBlock::pop(){
  this->index.pop_front();
}

