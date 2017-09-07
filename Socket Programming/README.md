# Socket Programming

Problem Description

Built a simple client-server system. 

The system that is developed is a simple key-value store. The client takes user input to get/put/delete keys and passes these requests over the server. The server maintains a data structure of key-value pairs in memory and returns a suitable reply to the server. When the request from the client is of the form "put key value", the server must store the key-value pair in its data structures, provided the key does not already exist. Put requests on an existing key must return an error message. When the client sends a request of the form "get key value", the value stored against the key must be returned. If the key is not found, a suitable error message must be returned. The client should be able to issue a delete request (on an existing key) as follows: "del key". For all requests that succeed, the server either returns the value (in case of get), or the string "OK" (in case of put/del). It is assumed that all keys and values are integers. When the user inputs "Bye", the server replies "Goodbye" and closes the client connection. Upon receiving this message from the server, the client program terminates.

Below is a sample execution of the client, when the server is running on the local host (hence the special loopback IP address 127.0.0.1).

>$ gcc client.c -o client  
>$ ./client 127.0.0.1 5000

>Connected to server

>Enter request: put 1 2

>Server replied: OK

>Enter request: put 1 3

>Server replied: Key already exists

>Enter request put 2 3

>Server replied: OK

>Enter request: get 1

>Server replied: 2

>Enter request: del 1

>Server replied: OK

>Enter request: get 1

>Server replied: Key not found

>Enter request: del 5

>Server replied: Key not found

>Enter request: Bye

>Server replied: Goodbye

>$

Note that the server is able to communicate with multiple active clients at a time. So the server is written using event-driven I/O system call epoll
When multiple clients talk to the server, the key-value pairs of all clients can be stored in a common data structure, and one client is able to see the data stored by the other clients.