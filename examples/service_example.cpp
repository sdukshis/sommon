#include <unistd.h>
#include <iostream>
#include <atomic>

#include <sommon/service.h>

using namespace std;


int worker(atomic<bool>& running) {
	cout << "Worker start\n";
	while (running) {
		cout << "Hello from worker thread\n";
		sleep(1);
	}
	cout << "Worker finishing\n";
	return 0;
}


int main(int argc, char* argv[]) {
	Service service(argc, argv);

	return service.run(worker);
}