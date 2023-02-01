#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();

private:
	std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {}

void ConsumerController::start() {
	// TODO: starts a ConsumerController thread
	pthread_create(&t, 0, ConsumerController::process, (void*) this);
}

void* ConsumerController::process(void* arg) {
	// TODO: implements the ConsumerController's work
	//usleep
	//https://blog.csdn.net/wangquan1992/article/details/103612109
	//chrono
	//https://www.pluralsight.com/blog/software-development/how-to-measure-execution-time-intervals-in-c--
	ConsumerController* consumer_ctrler = (ConsumerController*)arg;
	while(1){
		int worker_queue_size = consumer_ctrler->worker_queue->get_size();
		int high_thres = consumer_ctrler->high_threshold;
		int low_thres = consumer_ctrler->low_threshold;
		if(worker_queue_size > high_thres){
			Consumer* new_consumer = new Consumer(consumer_ctrler->worker_queue, 
												consumer_ctrler->writer_queue,
												consumer_ctrler->transformer);
			new_consumer->start();
			consumer_ctrler->consumers.push_back(new_consumer);
			printf("Scaling up consumers from %d to %d\n", consumer_ctrler->consumers.size() - 1, consumer_ctrler->consumers.size());
		}
		else if((worker_queue_size < low_thres) && (consumer_ctrler->consumers.size() > 1)){
			consumer_ctrler->consumers.back()->cancel();
			consumer_ctrler->consumers.pop_back();
			printf("Scaling down consumers from %d to %d\n", consumer_ctrler->consumers.size() + 1, consumer_ctrler->consumers.size());
		}

		usleep(consumer_ctrler->check_period);
	}
	return nullptr;
}

#endif // CONSUMER_CONTROLLER_HPP
