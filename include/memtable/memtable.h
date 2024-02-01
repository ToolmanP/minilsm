#ifndef __MEMTABLE_H

#include "backends/avltree.h"
#include "backends/engine.h"
#include "backends/rbtree.h"
#include "backends/skiplist.h"

namespace memtable{
    enum MemTable_Backend_Type{
        MEMTABLE_USE_AVLTREE = 0,
        MEMTABLE_USE_RBTREE,
        MEMTABLE_USE_SKIPLIST
    };
    using memtable_generic::MemTable;
    using avl::AVLTree;
    using rb::RBTree;
    using skl::SkipList;
};

#endif
