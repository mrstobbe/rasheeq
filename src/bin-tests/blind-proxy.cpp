
#include <Rasheeq.h>

#include <string>
#include <iostream>


int main(int argc, char** argv) {
	R::PollerPool pool;
	pool.createPoller();
	R::StreamServer server(&pool);
};

