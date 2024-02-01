#include <kvstore.h>
#include <iostream>

void add(){
    kvstore::KVStore store("/tmp/lsm_data");
    store.put(123,"hello_world");
    store.put(234,"this is life");
    store.flush();
}
void check(){
    kvstore::KVStore store("/tmp/lsm_data");
    std::cout << store.get(123) << std::endl;
    std::cout << store.get(234) << std::endl;
    std::cout << store.del(123) << std::endl;
    std::cout << store.get(123) << std::endl;
    std::cout << store.get(345) << std::endl;
}

int main(){
    add();
    check();
    return 0;
}
