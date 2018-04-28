Team members: Hang Nguyen, Scott Little and Jason Esparza


Contributions:
- Hang Nguyen: parse the user request input, producer-consumer code, synchronization and a little bit of threading
- Scott Little:
- Jason Esparza: memory management, debugging, socket management, and refactoring

Each group member contributed equally as our team met regularly to discuss, contribute, and debug accordingly.


The design decision:
We created the buffer with the size 2048 for receiving the http response as a chunk
and sends that chunks to the client in the while-loop.
We decided to use semaphores in order to ensure concurrency of a maximum of 30 users.


Strengths:


Weaknesses: