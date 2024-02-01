#include <kvstore.h>

void kvstore::KVStore::put(const uint64_t key, const std::string &s){
    this->mtable->insert(key,s);
    if(this->mtable->size() > MAX_CAPACITY)
      this->flush();
}
std::string kvstore::KVStore::get(const uint64_t key){
    auto ret =this->mtable->search(key);
    if(ret != ""){
        if(ret == deleted)
            return "";
        return ret;
    } else{
        ret = this->stable->search(key);
        if(ret == deleted)
            return "";
        else
            return ret;
    }
}

void kvstore::KVStore::reset(){
    this->mtable->reset();
    this->stable->reset();
}

bool kvstore::KVStore::del(const uint64_t key){
    if(this->get(key).empty())
        return false;
    else{
      this->mtable->remove(key);
      if(this->mtable->size() > MAX_CAPACITY)
        this->flush();
      return true;
    }
}
void kvstore::KVStore::flush(){
    this->stable->flush(this->mtable->dump());
}

void kvstore::KVStore::scan(const uint64_t key1, const uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list){
    (void) key1;
    (void) key2;
    (void) list;
}
