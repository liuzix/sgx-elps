#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

#include "queue.h"
#include "spin_lock.h"



namespace {
struct TestObj {
    int val;
    std::atomic_bool done = {false};

    TestObj() : val(0) {}
    TestObj(int x) : val(x) {}
};
static void producer(Queue<TestObj*>& q, int i, std::atomic<bool>& suicide) {
    char mem[sizeof(TestObj)];
    for (;;) {
        TestObj *obj = new ((void*)mem) TestObj(i);
        std::cout << "enqueue " << i << std::endl;
        q.push(obj);
        while (!obj->done.load() && !suicide.load()) {;}
        std::cout << i << "done" << std::endl;
        if (suicide.load())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
};

static void consumer(Queue<TestObj*>& q, std::atomic<bool>& suicide) {
    while (true) {
        TestObj *tmp = 0x0;
        q.take(tmp);
		if (tmp)
        	tmp->done.store(true);
        if (suicide.load())
            break;
    }
};

TEST(QueueTest, PushAndTake2) {
    Queue<TestObj*> IntQueue(20);
    std::vector<std::thread> threads;
    std::atomic<bool> sig = {false};

    for (int i = 0; i < 4; i++) {
        threads.push_back(std::thread(producer, std::ref(IntQueue),
                                        i, std::ref(sig)));
        threads.push_back(std::thread(consumer, std::ref(IntQueue), std::ref(sig)));
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    sig.store(true);
    for (auto& th : threads)
        th.join();
}
}
/*
namespace {

struct TestObj {
	int val;
	DPointer<TestObj> q_next;
	std::atomic_bool done = {false};

	TestObj() : val(0), q_next(nullptr) {}
	TestObj(int x) : val(x), q_next(nullptr) {} 
};

static void pushAndTake(Queue<TestObj>& q, TestObj& val, std::atomic<int>& sum) {
	q.push(&val);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	TestObj *tmp;
	if (q.take(tmp))
		sum += tmp->val;
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	if (q.take(tmp))
		sum += tmp->val;
};

TEST(QueueTest, PushAndTake) {
	Queue<TestObj> IntQueue;
	std::atomic<int> sum_g(0);
	
	int sum_true = 0;
	std::vector<std::thread> threads;
	TestObj tests[100];

	for (int i = 0; i < 100; i++) {
		tests[i].val = i;
		threads.push_back(std::thread(pushAndTake,
				std::ref(IntQueue), std::ref(tests[i]), std::ref(sum_g)));
		sum_true += i;
	}

	for (auto& th : threads)
		th.join();

	EXPECT_EQ(sum_true, sum_g);
}

static void producer(Queue<TestObj>& q, int i, std::atomic<bool>& suicide) {
	char mem[sizeof(TestObj)];
	for (;;) {
		TestObj *obj = new ((void*)mem) TestObj(i);
		std::cout << "enqueue " << i << std::endl;
		q.push(obj);
		while (!obj->done.load() && !suicide.load()) {;}
		std::cout << i << "done" << std::endl;
		if (suicide.load())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
};

static void consumer(Queue<TestObj>& q, std::atomic<bool>& suicide) {
	while (true) {
		TestObj *tmp;
		if (q.take(tmp)) {
			tmp->done.store(true);
//			std::cout << "dequeue " << tmp->val << std::endl;
		}
		if (suicide.load())
			break;
//		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
};

TEST(QueueTest, PushAndTake2) {
	Queue<TestObj> IntQueue;
	std::vector<std::thread> threads;
	std::atomic<bool> sig = {false};

	for (int i = 0; i < 4; i++) {
		threads.push_back(std::thread(producer, std::ref(IntQueue),
										i, std::ref(sig)));
		threads.push_back(std::thread(consumer, std::ref(IntQueue), std::ref(sig)));
	}
	
	std::this_thread::sleep_for(std::chrono::seconds(5));
	sig.store(true);
	for (auto& th : threads)
		th.join();
	EXPECT_EQ(IntQueue.getLen(), 0);
}

TEST(QueueTest, QueueLen) {
	Queue<TestObj> IntQueue;
	TestObj *tmp;
	TestObj tests[10];

	EXPECT_EQ(IntQueue.getLen(), 0);
	for (int i = 0; i < 10; i++)
		IntQueue.push(&tests[i]);
	EXPECT_EQ(IntQueue.getLen(), 10);

	for (int i = 0; i < 10; i++)
		IntQueue.take(tmp);
	
	EXPECT_EQ(IntQueue.getLen(), 0);	
}


TEST(QueueTest, IsIntrusive) {
	Queue<TestObj> TestQueue;

	TestObj *tmp;
	TestObj first(1);

	TestQueue.push(&first);
	TestQueue.take(tmp);

	EXPECT_EQ(tmp, &first);
}

}

namespace {

static void addNumWithSpinLock(int val, SpinLock& sl, int& sum) {
	sl.lock();
	sum += val;
	sl.unlock();
}

TEST(SpinLockTest, AddGlobalNum) {
	SpinLock sl;
	int sum = 0;
	int sum_true = 0;
	
	std::vector<std::thread> threads;

	for (int i = 0; i < 50; i++) {
		threads.push_back(std::thread(addNumWithSpinLock,
				i, std::ref(sl), std::ref(sum)));
		sum_true += i;
	}

	for (auto& th : threads)
		th.join();

	EXPECT_TRUE(sum_true == sum);
}

}
*/
int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

