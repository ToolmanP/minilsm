#include <memtable/memtable.h>
#include <iostream>

int main(){
    memtable::RBTree<int,int> rbtree;
    for(int i=0;i<100;i++){
        rbtree.insert(100-i,100+i);
    }
    for(auto &&p: rbtree.dump()){
        std::cout << p.first << " " << p.second.value_or(0) << std::endl;
    }

    return 0;
}