#ifndef sommon_service_h_
#define sommon_service_h_

#include <memory>

class Service {
 public:
 	Service(int argc, char* argv[]);
 	~Service() { }

 	template<class Function, class... Args>
 	int run(Function&& f, Args&&... args);

 protected:
 	struct Impl;
 	std::unique_ptr<Impl> pimpl_;
};  // class Service


#if defined(WIN32)
#include "impl/service_win32.h"
#elif defined(UNIX)
#include "impl/service_posix.h"
#else
#error "Sorry, no implementation for this platform :("
#endif

#endif  // sommon_service_h_