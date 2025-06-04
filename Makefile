# Main Makefile for Secure Chat Project

.PHONY: all server client clean

all: server client

server:
	$(MAKE) -C Auth
	$(MAKE) -C MessageDB  
	$(MAKE) -C Socket_server

client:
	$(MAKE) -C Socket_client

clean:
	$(MAKE) -C Socket_server clean
	$(MAKE) -C Socket_client clean
	$(MAKE) -C MessageDB clean
	$(MAKE) -C Auth clean
	rm -f chat_history.txt users.db sessions.db

initial install:
	sudo apt update
	sudo apt-get install build-essential libssl-dev
	@echo "Initial dependencies installed. Now running 'make install' to build the project."

install: all
	@echo "Build completed successfully!"
	@echo "To run server: cd Socket_server && ./server"
	@echo "To run client: cd Socket_client && ./client"

help:
	@echo "Available targets:"
	@echo "  all     - Build both server and client"
	@echo "  server  - Build server only"
	@echo "  client  - Build client only"
	@echo "  clean   - Clean all build files and databases"
	@echo "  install - Build and show usage instructions"
	@echo "  help    - Show this help message" 