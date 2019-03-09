#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

#include "queue.h"
#include "spin_lock.h"

namespace {

static void pushAndTake(Queue<int>& q, int val, std::atomic<int>& sum) {
	q.push(val);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	int tmp = 0;
	if (q.take(tmp))
		sum += tmp;
}

TEST(QueueTest, PushAndTake) {
	Queue<int> IntQueue;
	std::atomic<int> sum_g(0);
	
	int sum_true = 0;
	std::vector<std::thread> threads;

	for (int i = 0; i < 50; i++) {
		threads.push_back(std::thread(pushAndTake,
				std::ref(IntQueue), i, std::ref(sum_g)));
		sum_true += i;
	}

	for (auto& th : threads)
		th.join();

	EXPECT_EQ(sum_true, sum_g);
}

TEST(QueueTest, QueueLen) {
	Queue<int> IntQueue;
	int tmp = 0;

	EXPECT_EQ(IntQueue.getLen(), 0);
	for (int i = 0; i < 10; i++)
		IntQueue.push(i);
	EXPECT_EQ(IntQueue.getLen(), 10);

	for (int i = 0; i < 10; i++) {
		IntQueue.take(tmp);
		std::cout << tmp << std::endl;
	}
	EXPECT_EQ(IntQueue.getLen(), 0);	
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

