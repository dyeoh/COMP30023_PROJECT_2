# COMP30023_PROJECT_2
A server based proof of work solver for the Hashcash algorithm used in Bitcoin.


## Getting Started
 The program needs to be compiled before it can be used. Running the included makefile will result
 in an executable named server being created in the root directory of the project.
 
 

### Guide
Once the server is running, clients will be able to connect to the server and send Simple Stratum Text Protocol messages.
Every protocol message should be delimited with a carriage return and line feed (\r\n). The server will then reply to the
message based on the header and the corresponding payload. A detailed log will be generated and will overwrite the 
previous log each time the server is started up.

#### Starting up the Server
In order to run the server, you will first need to compile the program. The server executable takes in one argument, which
is the port number. Upon successfully binding to the port, a message will be displayed on the console.

Example:
```
>>./server 3000
Server started successfully on port 3000, listening for connections, a more detailed log file will be created in the directory.
```

#### Connecting and sending protocol messages to the server
Messages from clients should be delimited with a carriage return and line feed (\r\n). You can use a custom client to connect
to the client or you can use nc specifying the host address and destination port.

Example:
Assuming the server has already been started locally on port 3000
```
Client
>>nc 127.0.0.1
```

```
Server
>>./server 3000
Server started successfully on port 3000, listening for connections, a more detailed log file will be created in the directory.
Client 127.0.0.1 connected
```
There are seven implemented messages in the protocol which are :

1. Ping Message: PING
Upon receiving this the server will reply with a PONG

Example:
```
>>nc 127.0.0.1
PING
PONG
```

2. Pong Message: PONG
Upon receiving this the server will reply with an error message informing the client that a PONG message is reserved for the server

Example:
```
>>nc 127.0.0.1
PONG
ERRO: PONG reserved for server
```

3. Okay Message: OKAY
Upon receiving this, the server will reply with an error message informing the client that it is not okay to send OKAY

Example:
```
>>nc 127.0.0.1
OKAY
ERRO: Not okay to send OKAY
```

4. Error Message: ERRO
Upon receiving this, the server will reply with an error message informing the client that this message should not be sent
to the server

Example:
```
>>nc 127.0.0.1
ERRO
ERRO: Not okay to send ERRO
```

5. Solution Message: SOLN
 A solution message follows this format : SOLN difficulty:uint32 seed:BYTE[64] solution:uint64.
 
Upon receiving this, the server will check and if the concatenation of the seed and the solution produces a hash that satisfies
the target requirement which is derived from the difficulty value. If the proof of work is valid the server will reply with an
OKAY message, if not it will reply with an ERRO message with a corresponding payload.

Example:
```
>>nc 127.0.0.1
SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605
OKAY
SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 10000000232123a2
ERRO: Invalid/malformed solution
```

6. Work Message: WORK
A work message follows this format: WORK difficulty:uint32 seed:BYTE[64] start:uint64 worker/thread count:uint8

Upon receiving this, the server will add this work to a work queue. Once a proof-of-work solution is found it will reply with the SOLN message with the correct nonce.

Example:
```
>>nc 127.0.0.1
WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212399 01
SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212605
```

7. Abort Message: ABRT
Upon receiving this, the server will remove all the work that is queued up in the work queue and reply with an OKAY message.

Example:
```
>>nc 127.0.0.1
ABRT
OKAY
```

