#include <gtest/gtest.h>
#include <sys/mman.h>
#include "../libOS/allocator.h"
#include <vector>
#include <thread>
#define HEAP_LENGTH 0x100000000ULL
Allocator *testAllocator = nullptr;

class MallocEnvironment : public ::testing::Environment
{
public:
    virtual void SetUp()
    {
        void *heap = mmap(NULL, HEAP_LENGTH, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        ASSERT_TRUE(heap != MAP_FAILED);
        testAllocator = new Allocator(HEAP_LENGTH, (vaddr)heap);
    }
    virtual void TearDown()
    {
        munmap(testAllocator, HEAP_LENGTH);
    }
};

void SimpleIntegrityTest() {
    std::vector<unsigned char *> testVect;
    for (int i = 0; i < 100000; i++) {
        unsigned char *test = (unsigned char *)testAllocator->malloc(i % 4096);
        memset(test, i % 256, i % 4096);
        testVect.push_back(test);
    }
    for (int i = 0; i < 100000; i++) {
        for (int j = 0; j < i % 4096; j++) {
            EXPECT_EQ(testVect[i][j], i % 256);
        }
        testAllocator->free((vaddr)testVect[i]);
    }
}

TEST(MallocTest, SimpleThread) {
    std::thread first (SimpleIntegrityTest);
    std::thread second (SimpleIntegrityTest);
    std::thread third (SimpleIntegrityTest);
    std::thread fourth (SimpleIntegrityTest);
    first.join();
    second.join();
    third.join();
    fourth.join();
}

TEST(MallocTest, SimpleMalloc) {
    int *testInt = (int *)testAllocator->malloc(sizeof(int));
    *testInt = 5;
    EXPECT_EQ(*testInt, 5);
    testAllocator->free((vaddr)testInt);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MallocEnvironment);
    return RUN_ALL_TESTS();
}

