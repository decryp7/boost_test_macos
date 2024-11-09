#include <iostream>
#include "common.h"

#include <chrono>
#include <thread>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/container/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio.hpp>

#include "notifier_pool.h"

int main(int argc, char *argv[]) {
	notifier_pool pool{};
	boost::asio::thread_pool t{};
	while (true) {
		for(int i = 0; i < 100; i++) {
			post(t, [&pool, &t]{
				const scoped_notifier s{pool.get_notifier()};
				auto id = s.get_id();
				post(t, [id] {
					std::cout << id << std::endl;
				});
			});
		}
	}

	// using namespace boost::interprocess;
	//
	// managed_shared_memory *shm = nullptr;
	//
	// if(argc == 1)	{
	// 	struct shm_remove
	// 	{
	// 		shm_remove() { shared_memory_object::remove(SHARED_MEMORY_NAME); }
	// 		~shm_remove(){ shared_memory_object::remove(SHARED_MEMORY_NAME); }
	// 	} remover;
	//
	// 	//std::cout << e.get_error_code() << std::endl;
	// 	std::cout << "Start Server..." << std::endl;
	// 	shm = new managed_shared_memory(open_or_create, SHARED_MEMORY_NAME, 100 * 1000 * 1024);
	// 	interprocess_mutex *new_req_mutex = shm->find_or_construct<interprocess_mutex>(NEW_REQ_MUTEX_NAME)();
	// 	interprocess_condition *new_req_event = shm->find_or_construct<interprocess_condition>(NEW_REQ_EVENT_NAME)();
	// 	interprocess_mutex *req_queue_mutex = shm->find_or_construct<interprocess_mutex>(REQ_MUTEX_NAME)();
	// 	void_allocator shm_alloc(shm->get_segment_manager());
	// 	segment_manager_t *segment_manager = shm->get_segment_manager();
	// 	request_ringbuffer *req_queue = shm->find_or_construct<request_ringbuffer>(REQ_NAME)();
	//
	// 	while (true) {
	// 		{
	// 			scoped_lock<interprocess_mutex> lock{ *new_req_mutex };
	// 			new_req_event->wait(lock);
	// 		}
	//
	// 		Request r{ shm_alloc };
	//
	// 		do
	// 		{
	// 			if (r.function == DATAACCESS_SIMPLE_FUNCTION)
	// 			{
	// 				//std::cout << "Received simple function request.\n";
	// 				//2. Get the payload.
	// 				Reply *rp = r.reply.get();
	// 				auto *p = static_cast<char_string*>(r.data.get());
	// 				//printf("param: %s\n", p->c_str());
	//
	// 				//4. Store the reply on shared memory and set the handle.
	// 				rp->data = shm->construct<int>(anonymous_instance)(0);
	//
	// 				//4. Store the reply on shared memory and set the handle.
	// 				scoped_lock<interprocess_mutex> lock(rp->ready_mutex);
	// 				//std::cout << "Send simple function reply.\n";
	// 				rp->ready.notify_all();
	// 			}
	//
	// 			std::cout << "Free memory: " << shm->get_free_memory() / 1000 / 1000 << "mb\n";
	//
	// 		}while([&]
	// 		{
	// 			bool has_request{ false };
	// 			{
	// 				scoped_lock<interprocess_mutex> req_lock{ *req_queue_mutex };
	// 				has_request = req_queue->pop(r);
	// 			}
	// 			return has_request;
	// 		}());
	// 	}
	// }
	// else
	// {
	// 	shm = new managed_shared_memory(open_only, SHARED_MEMORY_NAME);
	// 	std::cout << "Connecting to server...\n";
	// 	interprocess_mutex *new_req_mutex = shm->find<interprocess_mutex>(NEW_REQ_MUTEX_NAME).first;
	// 	interprocess_condition *new_req_event = shm->find<interprocess_condition>(NEW_REQ_EVENT_NAME).first;
	// 	interprocess_mutex *req_queue_mutex = shm->find<interprocess_mutex>(REQ_MUTEX_NAME).first;
	// 	request_ringbuffer *request_queue = shm->find<request_ringbuffer>(REQ_NAME).first;
	// 	void_allocator shm_alloc(shm->get_segment_manager());
	// 	segment_manager_t *segment_manager = shm->get_segment_manager();
	//
	// 	while (true) {
	// 		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//
	// 		{
	// 			//SimpleFunction
	// 			auto start = std::chrono::high_resolution_clock::now();
	// 			Reply *rp = shm->construct<Reply>(anonymous_instance)();
	// 			scoped_lock<interprocess_mutex> lock(rp->ready_mutex);
	//
	// 			//1. Create payload.
	// 			auto *p = shm->construct<char_string>(anonymous_instance)("Hello World!", segment_manager);
	//
	// 			//2. Create and send the request. Logically possible to send request on another thread but queue needs to change to multiple writer and reader.
	// 			//Possible to keep single writer/reader (spsc_queue)?
	// 			Request r{ DATAACCESS_SERVICE,
	// 						DATAACCESS_SIMPLE_FUNCTION,
	// 						p,
	// 						rp,
	// 						shm_alloc };
	// 			{
	// 				//std::cout << "Send SimpleFunction request.\n";
	// 				scoped_lock<interprocess_mutex> req_lock(*req_queue_mutex);
	// 				request_queue->push(r);
	// 				scoped_lock<interprocess_mutex> notify_lock(*new_req_mutex);
	// 				new_req_event->notify_all();
	// 			}
	//
	// 			//3. Wait for the reply and read the reply handle to get the result from shared memory.
	// 			cv_status status = rp->ready.wait_until(lock, std::chrono::system_clock::now() + std::chrono::seconds(5));
	//
	// 			int result = 1;
	// 			if (status == cv_status::timeout)
	// 			{
	// 				//std::cout << "Wait SimpleFunction request timeout.\n";
	// 				result = 2;
	// 			}
	// 			else
	// 			{
	// 				//std::cout << "Read SimpleFunction reply.\n";
	// 				//4. Get reply using handle from shared memory
	// 				result = *(static_cast<int*>(rp->data.get()));
	// 			}
	//
	// 			shm->destroy_ptr(rp);
	// 			shm->destroy_ptr(p);
	// 		}
	// 		std::cout << "Free memory: " << shm->get_free_memory() / 1000 / 1000 << "mb.\n";
	// 	}
	// }
}
