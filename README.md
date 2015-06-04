Read some information from HTTP header in C
=======

Webinfo connects via BSD socket to server on port 80 via HTTP 1.1 and get
required information from HTTP header.


Installation:
--------------
    $ git clone https://github.com/tomasvalek/Webinfo.git

Requirement:
    gcc

Using:
-------------
./webinfo [-l][-s][-m][-t] URL     
-l: show object size     
-s: show identifier of server     
-m: show the latest modification     
-t: show type of content     

If require item of header is not available, item show:N/A.

Using valgrind:
--------------
    valgrind --tool=memcheck --leak-check=full --leak-resolution=high ./webinfo -lstm http://www.google.com
