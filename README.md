# FSS- A Low Level "In-Memory" Cache File Storage Server 💾

## Introduction 📜

The project involves the development of an in-memory cache file storage server. It comprises two main components: the Server and the Clients. Clients can make requests to the Server, following a connection, to manage files accessible in memory.

## Server 🖥️

The Server is a single multithreaded process. Server configurations are extracted from config files located within the "Config_Files" directory. The chosen config file must be specified as an argument when starting the Server. The values inside the config file are parsed in the following order:

1) Number of Workers
2) Maximum Number of Files
3) Maximum Memory Size in Bytes
4) Socket Name to connect to

### Connection with Clients and Auxiliary Threads 🌐

The Server is designed to accept multiple connections from different clients. The Main Thread initializes socket connections and creates auxiliary threads:

- Thread Dispatcher: Responsible for creating the thread pool, accepting client connections, and signaling the "threadfunction" (using a condition variable) when a client connects to the server. This allows the threadfunction to handle client requests, fetching the client's ID from a concurrent queue.

- Thread Logger: Handles server-side disk writing. Given that disk writes are slow, the project separates the management and writing of the log file from normal client request execution. Whenever a server-side operation is completed, it sends a signal (via a condition variable) to the thread logger, unblocking it to continue execution by writing to the log file.

### Data Structures 🧱

Various data structures are used to store files within the Server. Here are some of the main ones:

- Cache:
  - Hash Table for file storage
  - Concurrent Queue for storing file names (for FIFO management during replacement)
  - Mutex for concurrent memory management
  - Variables for storing cache-related information (int)

- Mylock:
  - Mutex
  - Condition variable

- MyFile:
  - File content (void *)
  - File name (char *)
  - Mylock (lock and cv for each file)
  - Client ID holding the lock (int)
  - Concurrent List (llist *)

- Concurrent Queue:
  - Pointer to the first element
  - Mutex
  - Condition variable

- Concurrent List:
  - Pointer to the first element of the list

### Concurrency, Locking, and Unlocking Operations 🔒

To manage server concurrency, several measures have been taken, including the use of concurrent data structures (such as the queue and list), mutexes within the cache, and for each individual file. Locking and unlocking operations occur in the following order:

1) Lock on the cache
2) Lock on the file
3) Unlock on the cache
4) File processing
5) Unlock on the file

This approach allows the cache to be accessible as soon as possible and performs more precise locks. Client-side lock and unlock operations involve a short fixed wait and reattempt by the client if the resource they want to lock belongs to someone else. A client's lock is released once the client closes the file to prevent a single client from blocking the entire cache. A locked file is still subject to replacement to prevent the cache from filling up with locked files.

## Client 🖥️

The Client consists of a single mono-threaded process that makes requests to the Server by passing the arguments provided at execution. After completing explicit requests, it terminates the connection with the server using the "Close Connection" operation. When file operations are requested with a complete path, the client extracts the file name and uses it as an identifier within the server.

## Testing and Makefile 🧪

All three tests passed during local testing. To run each test, execute the following commands within the Source directory:

- `make test1`: Executes operations requested by clients, ensuring successful completion without memory leaks and valgrind errors.
- `make test2`: Creates replacement folders (if they do not exist) and writes 5 files into two different folders, selected in a FIFO manner.
- `make test3`: The test concludes after 30 seconds of client execution and succeeds, providing data consistent with the algorithm's execution times.

  
## Contact 📇

For questions or support, please feel free to contact me at gb8gb8**AT**gmail**DOT**com 

Feel free to explore the project, contribute, and adapt it to suit your data logging needs. Happy coding!
