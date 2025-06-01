# Main Makefile for Secure Chat Project

.PHONY: all server client clean

all: server client

server:
	$(MAKE) -C Socket_server

client:
	$(MAKE) -C Socket_client

clean:
	$(MAKE) -C Socket_server clean
	$(MAKE) -C Socket_client clean
	$(MAKE) -C MessageDB clean
	rm -f chat_history.txt

install: all
	@echo "Build completed successfully!"
	@echo "To run server: cd Socket_server && ./server"
	@echo "To run client: cd Socket_client && ./client"

help:
	@echo "Available targets:"
	@echo "  all     - Build both server and client"
	@echo "  server  - Build server only"
	@echo "  client  - Build client only"
	@echo "  clean   - Clean all build files and chat history"
	@echo "  install - Build and show usage instructions"
	@echo "  help    - Show this help message" 