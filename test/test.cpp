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
	DPointer<TestObj> q_next;

	TestObj() : val(0), q_next(nullptr) {}
	TestObj(int x) : val(x), q_next(nullptr) {} 
};

static void pushAndTake(Queue<TestObj>& q, TestObj& val, std::atomic<int>& sum) {
	q.push(&val);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	TestObj *tmp;
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

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

