# A Simple HTTP Web Proxy
## 1. Overview
This project aims to implement a simple web proxy using HTTP 1.0. It only requires implementing GET
method.


## 2. Background
**HTTP**
The Hypertext Transfer Protocol or (HTTP) is the protocol used for communication on the web. That is, it
is the protocol which defines how your web browser requests resources from a web server and how the
server responds. For simplicity, in this project we will be dealing only with version 1.0 of the HTTP protocol,
defined in detail in RFC 1945. You may refer back to the RFC while completing this assignment, but our
instructions should be self-contained.
HTTP communications happen in the form of transactions. A transaction consists of a client sending a
request to a server and then reading the response. Request and response messages share a common basic
format:

&nbsp;• An initial line (a request or response line, as defined below)
&nbsp;• Zero or more header lines
&nbsp;• A blank line (CRLF)
&nbsp;• An optional message body.

The initial line and header lines are each followed by a "carriage-return line-feed (CRLF)" (\r\n) signifying
the end-of-line.

For most common HTTP transactions, the protocol boils down to a relatively simple series of steps
(important sections of RFC 1945 are in parenthesis):

1. A client creates a connection to the server.

2. The client issues a request by sending a line of text to the server. This request line consists of a
HTTP method (most often GET, but POST, PUT, and others are possible in real world), a request
URI (like a URL), and the protocol version that the client wants to use (HTTP/1.0). The message
body of the initial request is typically empty. (5.1-5.2, 8.1-8.3, 10, D.1)

3. The server sends a response message, with its initial line consisting of a status line, indicating if
the request was successful. The status line consists of the HTTP version (HTTP/1.0), a response
status code (a numerical value that indicates whether or not the request was completed
successfully), and a reason phrase, an English-language message providing description of the
status code. Just as with the request message, there can be as many or as few header fields in
the response as the server wants to return. Following the CRLF field separator, the message
body contains the data requested by the client in the event of a successful request. (6.1-6.2, 9.1-
9.5, 10)

4. Once the server has returned the response to the client, it closes the connection. It is fairly easy
to see this process in action without using a web browser. From a Unix/LINUX prompt, type:
```
telnet www.yahoo.com 80
```
This opens a TCP connection to the server at www.yahoo.com listening on port 80 ? the default
HTTP port. You should see something like this:
```
Trying 209.131.36.158...
Connected to www.yahoo.com (209.131.36.158).
Escape character is '^]'.
```
type the following:
```
GET / HTTP/1.0
Hostname:www.yahoo.com
Connection:close
```
and hit enter twice. You should see something like the following:
```
HTTP/1.0 200 OK
Date: Fri, 10 Nov 2006 20:31:19 GMT
Connection: close
Content-Type: text/html; charset=utf-8
<html><head>
<title>Yahoo!</title>
(More HTML follows)
 ```
There may be some additional pieces of header information as well-setting cookies, instructions to the
browser or proxy on caching behavior, etc. What you are seeing is exactly what your web browser sees
when it goes to the Yahoo home page: the HTTP status line, the header fields, and finally the HTTP
message body ? consisting of the HTML that your browser interprets to create a web page. You may notice
here that the server responds with HTTP 1.1 even though you requested 1.0. Some web servers refuse to
serve HTTP 1.0 content.

**HTTP Proxy**

Ordinarily, HTTP is a client-server protocol. The client (usually your web browser) communicates directly
with the server (the web server software). However, in some circumstances it may be useful to introduce
an intermediate entity called a proxy. Conceptually, the proxy sits between the client and the server. In
the simplest case, instead of sending requests directly to the server the client sends all its requests to the
proxy. The proxy then opens a connection to the server, and passes on the client's request. The proxy
receives the reply from the server, and then sends that reply back to the client. Notice that the proxy is
essentially acting like both a HTTP client (to the remote server) and a HTTP server (to the initial client).


## 3. Assignment
This project aims to implement a simple web proxy using HTTP 1.0. It only requires implementing GET method. Your task is to build a web proxy capable of accepting HTTP requests, forwarding requests to remote (origin) servers, and returning response data to a client.
Caching is not required in the proxy!
This web proxy can be implemented in either C or C++. The executable is called MyProxy that takes as its first argument a port to listen on. Don't use a hard-coded port number.

**Listening**

When your proxy starts, the first thing that it will need to do is establish a socket that it can use to listen for incoming connections. Your proxy should listen on the port specified from the command line and wait for incoming client connections. Your proxy is a multi-threaded process. After each new client request is accepted, a worker thread handles the request. To avoid overwhelming your proxy, you should not create an excessive number of worker threads (for this project, 30 is a good number). A thread pool and producer/consumer synchronization can be helpful in this project.
Once a client has connected, the proxy should read data from the client and then check for a properly-formatted HTTP request. Specifically, you will parse the HTTP request to ensure that the proxy receives a request that contains a valid request line:
```
<METHOD> <URL> <HTTP VERSION>
```
In this project, client requests to the proxy must be in their absolute URI form (see RFC 1945, Section 5.1.2), e.g.,
GET http://www.washington.com/index.html HTTP/1.0
Web browsers will send absolute URI if properly configured to explicitly use a proxy (as opposed to a transparent on-path proxy). On the other form, your proxy should issue requests to the webserver properly specifying relative URLs, e.g.,
GET /index.html HTTP/1.0
Host: www.seattleu.edu
Connection: close
An invalid request from the client should be answered with an appropriate error code, i.e. 500 'Internal Error'. Similarly, if headers are not properly formatted for parsing, your proxy should also generate a type of 500 'Internal Error' message.

**Parsing the URL**
Once the proxy receives a valid HTTP request, it will need to parse the requested URL. The proxy needs at least three pieces of information: the requested host, port, and path. You will need to parse the absolute
URL specified in the given request line. If the hostname indicated in the absolute URL does not have a port
specified, you should use the default HTTP port 80.

**Getting Data from the Remote Server**
Once the proxy has parsed the URL, it can make a connection to the requested host (using the appropriate
remote port, or the default of 80 if none is specified) and send the HTTP request for the appropriate
resource. The proxy should always send the request in the relative URL + Host header format.
For instance:
A request from client:
```
GET http://www.cnn.com/ HTTP/1.0
```
Send to remote server:
```
GET / HTTP/1.0
Host: www.cnn.com
Connection: close
(Additional client specified headers, if any...)
```
Note that we always send HTTP/1.0 flags and a Connection: close header to the server, so that it will close
the connection after its response is fully transmitted, as opposed to keeping open a persistent connection
(as we learned in class). So while you should pass the client headers you receive on to the server, you
should make sure you replace any Connection header received from the client with one specifying close,
as shown. To add new headers or modify existing ones, do it when parsing the HTTP request.

**Returning Data to the Client**
After the response from the remote server is received, the proxy should send the response message as-is
to the client via the appropriate socket. To be strict, the proxy would be required to ensure a Connection:
close is present in the server's response to let the client decide if it should close its end of the connection
after receiving the response. However, checking this is not required in this assignment for the following
reasons. First, a well-behaving server would respond with a Connection: close anyway given that we
ensure that we sent the server a close token. Second, we configure Firefox to always send a Connection:
close by setting keepalive to false. Finally, we wanted to simplify the assignment so you would not have
to parse the server response.
The following summarizes how status replies should be sent from the proxy to the client:
1. For any error your proxy should return the status 500 'Internal Error'. This means for any
request method other than GET, your proxy should return the status 500 'Internal Error'.
Likewise, for any invalid, incorrectly formed headers or requests, your proxy should return the
status 500 'Internal Error'. For any error that your proxy has in processing a request such as
failed memory allocation or missing files, your proxy should also return the status 500 'Internal
Error'.
2. Your proxy should simply forward status replies from the remote server to the client. This means
most 1xx, 2xx, 3xx, 4xx, and 5xx status replies should go directly from the remote server to the
client through your proxy. Most often this should be the status 200 'OK'. However, it may also
be the status 404 'Not Found' from the remote server. (While you are debugging, make sure you
are getting valid 404 status replies from the remote server and not the result of poorly
forwarded requests from your proxy.)
