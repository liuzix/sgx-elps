#include <iostream>
#include <vector> // vector class-template definition
#include <thread>
#include <chrono>
using namespace std;

#include "test.h"


Queue<int> *ReqQueue = new Queue<int>();
atomic<int> sum(0);

void insert(int val) {
	ReqQueue->push(val);
	this_thread::sleep_for(chrono::seconds(1));
	int tmp = 0;
	if (ReqQueue->take(tmp))
		sum += tmp;
}


int main()
{
	int sum_test = 0;
	vector<thread> threads;

	for (int i = 0; i < 50; i++) {
		threads.push_back(thread(insert,i));
		sum_test += i;
	}

	for (auto& th : threads)
		th.join();

	cout << sum << ' ' << sum_test << endl;

	return 1;
} // end main

