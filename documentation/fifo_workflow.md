# FIFO Queue Workflow

This document details the concurrency model used to ensure data safety in the multi-user OFS server.

## 1. The Concurrency Problem
Since multiple users can connect via TCP simultaneously, race conditions could occur (e.g., two users trying to create the same file at the exact same time). To solve this, we treat the file system as a **critical section**.

## 2. The Solution: Producer-Consumer Model

### Components
* **Producers (Client Threads):** One thread per connected user.
* **Queue (`FifoQueue`):** A thread-safe queue protected by a `std::mutex` and `std::condition_variable`.
* **Consumer (Processor Thread):** A single dedicated thread that executes file operations.

### The Workflow

1.  **Connection:** When a client connects, the main server loop spawns a dedicated **Client Thread**.
2.  **Ingestion:** The Client Thread reads the JSON request from the socket.
3.  **Queuing:** The Client Thread pushes the request (containing the socket ID and JSON data) into the **FifoQueue**. It *does not* execute the logic.
4.  **Processing:** The **Processor Thread**, which is always waiting for the queue to be non-empty, wakes up and pops the request.
5.  **Execution:** The Processor Thread calls the appropriate API function (e.g., `file_create`). Since this is the *only* thread allowed to touch the file system data structures, no locks are needed within the data structures themselves.
6.  **Response:** The Processor Thread sends the JSON response back to the specific client socket and closes the connection.

This ensures that all operations are strictly serialized (First-In, First-Out), guaranteeing data consistency.