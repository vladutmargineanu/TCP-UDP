# Protocoale de comunicatii:
# Makefile

CFLAGS = -Wall -g

all: server subscriber

# Compileaza subscriber.c
subscriber: subscriber.c

# Compileaza server.c
server: server.c

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza subscriberul
run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber
