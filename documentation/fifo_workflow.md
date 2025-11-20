# FIFO Workflow

- All client connections enqueue Request {client_fd, json}.
- Single worker thread dequeues and processes one request at a time.
- Worker responds by sending JSON over the client's socket.
- HTTP UI calls the HTTP bridge which returns JSON immediately (synchronous).