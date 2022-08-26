#include "webserver.h"


int main() {

	// 隐式实例化对象
	WebServer server(9007, true, 0, true, 10, 10, false, 1);

	server.start();

	return 0;

}