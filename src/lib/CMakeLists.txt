set(SRC
	Poller.cpp
	PollerPool.cpp
	StreamClient.cpp
	StreamServer.cpp
)

add_library(rasheeq SHARED ${SRC})
