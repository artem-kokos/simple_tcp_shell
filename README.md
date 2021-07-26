# Simple TCP Shell
Simple command line shell over TCP

### Build
```
$ gcc tcpsh.c -o tcpsh
```

### Usage
```
$ ./tcpsh
```
It starts up on 4470 port. Now new connections can be established, for instance, with netcat:
```
$ nc <IP addr> 4470
```
