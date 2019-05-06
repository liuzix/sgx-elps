#include <vector>
#include <cstring>
#include <iostream>
#include "../libOS/slub.h"

using namespace std;

#define CHUNK_SIZE 64

int main () {
    SlubCache *slub = createUnsafeSlub(CHUNK_SIZE);
    vector<char *> bufs;

    for (int i = 0; i < 100000; i++) {
        cout << "allocating " << dec << CHUNK_SIZE << " bytes\n";
        char *buf = (char *)slub->allocate();
        cout << "buf = " << hex << (uint64_t)buf << endl;
        memset(buf, i % 25, CHUNK_SIZE);
        bufs.push_back(buf);
    }

    for (int i = 99999; i >= 0; i--) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            assert(bufs[i][j] == i % 25);
        }
        
        cout << "freeing\n" << endl;
        slub->free(bufs[i]);
    }

}
