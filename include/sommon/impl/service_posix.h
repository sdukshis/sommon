#ifndef sommon_impl_service_posix_h__
#define sommon_impl_service_posix_h__

#include <thread>
#include <atomic>
#include <utility>
#include <cstdint>
#include <cstdlib>

namespace sys {
#include <signal.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
} // namespace sys


struct Service::Impl {
 public:
 	Impl(int argc, char** argv):
 		argc_(argc), argv_(argv) { }

 	~Impl() { }

    template<class Function, class... Args>
 	int run(Function&& worker, Args&&... args) {
        sigset_t mask;
        int sfd;
        int efd;
        int epollfd;
        //struct signalfd_siginfo fdsi;
        ssize_t s;

        sys::sigemptyset(&mask);
        sys::sigaddset(&mask, SIGINT);
        sys::sigaddset(&mask, SIGQUIT);

        if (-1 == sys::sigprocmask(SIG_BLOCK, &mask, NULL)) {
            perror("sigprocmask");
        }

        sfd = sys::signalfd(-1, &mask, sys::SFD_NONBLOCK);
        if (-1 == sfd) {
            perror("signalfd");
        }

        efd = sys::eventfd(0, sys::EFD_NONBLOCK);
        if (-1 == efd) {
            perror("eventfd");
        }

        epollfd = sys::epoll_create1(0);
        if (-1 == epollfd) {
            perror("epoll_create");
        }

        struct sys::epoll_event sevent;
        sevent.data.fd = sfd;
        sevent.events = sys::EPOLLIN | sys::EPOLLET;
        s = sys::epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &sevent);
        if (-1 == s) {
            perror("epoll_ctl");
        }

        struct sys::epoll_event eevent;
        eevent.data.fd = efd;
        eevent.events = sys::EPOLLIN | sys::EPOLLET;
        s = sys::epoll_ctl(epollfd, EPOLL_CTL_ADD, efd, &eevent);
        if (-1 == s) {
            perror("epoll_ctl");
        }


        daemonize();

        int rv = 0;
        running_.store(true);
        std::thread
        worker_thread([](const std::atomic<bool>& running, int efd, Function&& worker, Args&&... args) {
            int64_t rv = 0;
            try {
                rv = worker(running, args...);
            } catch (...) {
                rv = 2;
            }

            write(efd, &rv, sizeof(rv));
        }, std::cref(running_), efd, std::ref(worker), std::ref(args)...);


        struct sys::epoll_event events[64];

        bool wait = true;
        int timeout = -1; // infinite

        while (wait) {
            int n = sys::epoll_wait(epollfd, events, 64, timeout);

            for (int i = 0; i < n; ++i) {
                if ((events[i].events & sys::EPOLLERR) ||
                    (events[i].events & sys::EPOLLHUP) ||
                    (!(events[i].events & sys::EPOLLIN))) {
                    fprintf(stderr, "epoll error");
                    continue;
                } else if (events[i].data.fd == sfd) {
                    std::cout << "Signal received\n";
                    running_.store(false);
                    wait = false;
                } else if (events[i].data.fd == efd) {
                    ssize_t rb = read(efd, &rv, sizeof(rv));
                    if (rb != sizeof(rv)) {
                        perror("read");
                    }
                    std::cout << "Event received\n";
                    wait = false;
                } 
            }
        }
        std::cout << "waiting for worker joined" << std::endl;

 		worker_thread.join();
        std::cout << "worker joined\n";
 		return rv;
 	}

 	void daemonize() { }


 private:
 	int argc_;
 	char** argv_;

 	std::atomic<bool> running_;
};  // class Service::Impl


Service::Service(int argc, char* argv[]):
	pimpl_(new Impl(argc, argv)) { }

template<class Function, class... Args>
int Service::run(Function&& f, Args&&... args) {
	return pimpl_->run(f, args...);
}

#endif  // sommon_impl_service_posix_h__