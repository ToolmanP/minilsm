#include <memtable/memtable.h>
#include <iostream>

int main(){
    memtable::AVLTree<int,int> avltree;
    for(int i=0;i<100;i++){
        avltree.insert(100-i,100+i);
    }

    for(auto &&p: avltree.dump()){
        std::cout << p.first << " " << p.second.value_or(0) << std::endl;
    }
    avltree.reset();
    std::cout << avltree.length() << std::endl;
    return 0;
}
