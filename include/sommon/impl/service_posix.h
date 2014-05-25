#ifndef sommon_impl_service_posix_h__
#define sommon_impl_service_posix_h__

#include <thread>
#include <atomic>
#include <csignal>
#include <pthread.h>

struct Service::Impl {
 public:
 	Impl(int argc, char** argv):
 		argc_(argc), argv_(argv) { }

 	~Impl() { }

 	template<class C>
 	int run(C worker) {
 		struct sigaction sigact;
 		sigact.sa_handler = signal_handler;
 		sigemptyset(&sigact.sa_mask);
 		sigaction(SIGINT, &sigact, NULL);
 	
 		daemonize();


 		int rv = 0;
 		std::thread
 		worker_thread([this, &rv, &worker]() {
 			sigset_t mask;
 			sigemptyset(&mask);
 			sigaddset(&mask, SIGINT);
 			sigaddset(&mask, SIGQUIT);

 			pthread_sigmask(SIG_BLOCK, &mask, NULL);
 			
 			running_.store(true);

 			try {
 				rv = worker(running_);
 				std::cout << "FINISH\n";
 			} catch (...) {
 				rv = 2;
 			}
 		});

 		worker_thread.join();

 		return rv;
 	}

 	void daemonize() { }

 	static void signal_handler(int signum) {
 		if (signum == SIGINT) {
 			running_.store(false);
 		}
 	}

 private:
 	int argc_;
 	char** argv_;

 	static std::atomic<bool> running_;
};  // class Service::Impl

std::atomic<bool> Service::Impl::running_(false);

Service::Service(int argc, char* argv[]):
	pimpl_(new Impl(argc, argv)) { }

template<class C>
int Service::run(C worker) {
	return pimpl_->run(worker);
}

#endif  // sommon_impl_service_posix_h__