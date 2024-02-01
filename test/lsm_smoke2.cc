// Testing whether compaction is performed correctly.

#include <kvstore.h>

int main(){
  kvstore::KVStore store("/tmp/lsm_data","/tmp/default.conf");
  for(int i=0;i<20;i++){
    store.put(i,"a");
    store.flush();
  }
}
