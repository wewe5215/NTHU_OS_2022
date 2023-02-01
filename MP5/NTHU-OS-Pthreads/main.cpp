#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

int main(int argc, char** argv) {
	assert(argc == 4);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// TODO: implements main function
	TSQueue<Item*>* input_queue = new TSQueue<Item*>(READER_QUEUE_SIZE);
	TSQueue<Item*>* worker_queue = new TSQueue<Item*>(WORKER_QUEUE_SIZE);
	TSQueue<Item*>* output_queue = new TSQueue<Item*>(WRITER_QUEUE_SIZE);

	Transformer* transformer = new Transformer;
	Reader* reader = new Reader(n, input_file_name, input_queue);
	Writer* writer = new Writer(n, output_file_name, output_queue);

	Producer* p1 = new Producer(input_queue, worker_queue, transformer);
	Producer* p2 = new Producer(input_queue, worker_queue, transformer);
	Producer* p3 = new Producer(input_queue, worker_queue, transformer);
	Producer* p4 = new Producer(input_queue, worker_queue, transformer);

	ConsumerController* consumer_ctrler = new ConsumerController
										(worker_queue, output_queue, transformer, 
										CONSUMER_CONTROLLER_CHECK_PERIOD, 
										(WORKER_QUEUE_SIZE * CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE / 100),
										(WORKER_QUEUE_SIZE * CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE / 100));

	reader->start();
	writer->start();

	p1->start();
	p2->start();
	p3->start();
	p4->start();
	consumer_ctrler->start();

	reader->join();
	writer->join();

	delete input_queue;
	delete worker_queue;
	delete output_queue;
	delete reader;
	delete writer;
	delete transformer;
	delete p1;
	delete p2;
	delete p3;
	delete p4;
	delete consumer_ctrler;

	return 0;
}
