#include <vector>
#include <pthread.h>
#include <cstring>
#include <iostream>
#include "../libOS/slub.h"
#include "../libOS/panic.h"
#include "../libOS/slub.h"
#include "../libOS/mm/rbtree.h"

using namespace std;

#define CHUNK_SIZE 64

struct test_node {
    int key;
    mm::rbtree_node node;
};

mm::malloc_rbtree<test_node, &test_node::node, int, &test_node::key> tree;    

static void *worker (void *arg) {
    size_t base = (size_t)arg * 100000;

    for (int i = 99999; i >= 0; i--) {
        auto node = new (unsafe_slub_malloc(sizeof(test_node))) test_node;
        node->key = base + i;
        
        //cout << "inserting: " << base + i << endl;
        if (!tree.insert(node)) {
            __asm__("ud2");
        }
    }

    return nullptr;
    auto it = tree.begin();
    int prev = it->key;
    while (it) {
        //cout << it->key << endl;
        it = tree.next(it);
        if (prev >= it->key)
            __asm__("ud2");
    }
    
    for (int i = 0; i < 100000; i++) {
        it = tree.lookup(i);
        
        if (!it || it->key != i)
            __asm__("ud2");
                    
        tree.erase(it);
    }
}

int main() {
    pthread_t threads[8];

    for (size_t i = 0; i < 8; i++) {
        if (pthread_create(&threads[i], NULL, &worker, (void *)i)) {
            cout << "error!" << endl;
            exit(-1);
        }
    }

    for (int i = 0; i < 8; i++)
        pthread_join(threads[i], NULL);

    cout << "test ended!" << endl;
    return 0;
}
