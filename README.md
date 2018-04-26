Team members:
Hang Nguyen
Scott Little
Jason Esparza


Contributions:
- Hang Nguyen: parse the requested URL, producer-consumer code, a little bit of threading
- Scott Little:
- Jason Esparza:


The design decision:
We created the buffer with the size 2048 for receiving the http response as a chunk
and sends that chunks to the client in the while-loop.


Strengths:


Weaknesses: