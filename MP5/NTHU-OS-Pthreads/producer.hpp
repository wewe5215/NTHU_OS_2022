#include <pthread.h>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef PRODUCER_HPP
#define PRODUCER_HPP

class Producer : public Thread {
public:
	// constructor
	Producer(TSQueue<Item*>* input_queue, TSQueue<Item*>* worker_queue, Transformer* transfomrer);

	// destructor
	~Producer();

	virtual void start();
private:
	TSQueue<Item*>* input_queue;
	TSQueue<Item*>* worker_queue;

	Transformer* transformer;

	// the method for pthread to create a producer thread
	static void* process(void* arg);
};

Producer::Producer(TSQueue<Item*>* input_queue, TSQueue<Item*>* worker_queue, Transformer* transformer)
	: input_queue(input_queue), worker_queue(worker_queue), transformer(transformer) {
}

Producer::~Producer() {}

void Producer::start() {
	// TODO: starts a Producer thread
	pthread_create(&t, 0, Producer::process, (void*) this);
}

void* Producer::process(void* arg) {
	// TODO: implements the Producer's work
	Producer* producer = (Producer*)arg;
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

	while(1){
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
		Item* it = producer->input_queue->dequeue();
		it -> val = producer->transformer->producer_transform(it->opcode, it->val);
		producer->worker_queue->enqueue(it);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
	}

	return nullptr;
}

#endif // PRODUCER_HPP
