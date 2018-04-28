**Team members:** Hang Nguyen, Scott Little and Jason Esparza


**Contributions:**
- Hang Nguyen: parse the user request input, producer-consumer code, synchronization and a little bit of threading
- Scott Little:
- Jason Esparza: memory management, debugging, socket management, and refactoring

Each group member contributed equally as our team met regularly to discuss, contribute, and debug accordingly.


**The design decision:**
We created the buffer with the size 2048 for receiving the http response as a chunk
and sends that chunks to the client in the while-loop.
We decided to use semaphores in order to ensure concurrency of a maximum of 30 users.


**Strengths:**
- Fully multi threaded for CPU(s) utilization
	
- Multiple clients can use proxy at same time
	
- Clients can load multiple pages at the same time
	
- Efficient memory management

- Functionally decomposed sending for easier readability
	
- Uses producer/consumer method to efficiently send/receive messages rather than storing in buffers

- Loads both light and heavy pages within minimal time

- Correctly manages sockets connections, closes when not used anymore

- Limits number of threads to not overload system or use too many sockets

- Sends 500 INTERNAL ERROR when data is corrupted or not accessible
- Compiles with Werror, Wall and pedantic flags

- Proxy always stays running


**Weaknesses:**
- We set a big size for the size of the receiving request buffer so that it can receive all requests from the client (Statically allocated large buffer size, could use dynamic allocation with reallocation for memory management)
- Could functionally decompose more
- Make use of more detailed commenting for even easier understanding of program