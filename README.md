# TCP server-client secure chat app

## Overview

This is a TCP server-client chat application project, comprising one server and multiple distinct clients. The server operates on a local address and uses port 4443 (the rationale for defining this specific port number is detailed in the Software Design Document). One or multiple clients can be executed on the same local address as the server, but they will utilize different random ephemeral ports. Upon successful connection to the server, an authentication stage is required. If you already have a registered account, you can log in and begin chatting with other clients. The history of previous conversations will also be displayed. If you do not have an account, registration is obligatory. You will set your username and password during the registration process. Subsequently, you will have full authority to view chat history and send messages to other clients connected to the server.


## Specification

- This project is designed to be compatible with Ubuntu 22.04.
- The used programming language is C (GCC compiler).
- Multiuser, multithread is required, which means one or more user can have access to use chat app. 
- Authentication (registration, login) is required for user. The method of authentication is session-based. All sessions information is saved in sessions.db in folder server.
- This project is capable of saving and storing chat history. You can see chat history database (chat_history.txt) in folder server.
- When the server and clients are restart, all related information comprising chat history, session id, users is not disappeared.
- Based on using OpenSSL development libraries (`libssl-dev`), all data payloads when the other view in Wireshark are encrypted and the only known information about data packets are the type of protocol, headers, etc...
- Based on GNU Make, you can start and use this project more easily. The detailed command line actions are describes in below. 


## Dependency installation

In Ubuntu/Debian, run:
```sh
sudo apt update
sudo apt install build-essential libssl-dev
```
Or run:
```sh
make initial install
```

---

## Compile project

From the project root directory, run: 
```sh
make all
```
This command will automatically compile both the server and the client. 

If you want to compile each different parts of the project, run:
```sh
make -C Auth
make -C MessageDB
make -C Socket_server
make -C Socket_client
```

---

## Run server 

1. Ensure you have the cert.pem certificate file and key.pem private key in the Socket_server directory.
   - If not, run:
     ```sh
     openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
     ```
2. Run server:
   ```sh
   cd Socket_server
   ./server
   ```

---

## Chạy client

1. Open new terminal.
2. Run client:
   ```sh
   cd Socket_client
   ./client
   ```

---

## Notes

- The server listens on port 4443 by default.
- Chat history is saved in Socket_server/chat_history.txt.
- Ensure the server is running before starting the client.
- All communication is encrypted via SSL/TLS.

---

## Clean compiled files

```sh
make clean
```
Or clean each files:
```sh
make -C Socket_server clean
make -C Socket_client clean
make -C MessageDB clean
make -C Auth clean
```

---

## Contact 

If you encounter any errors or need assistance, please contact us via GitHub. 