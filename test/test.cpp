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

	EXPECT_TRUE(sum_true == sum_g);
}

}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

