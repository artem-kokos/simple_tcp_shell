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
It starts up on 4470 port by default. Now new connections can be established, for instance, with netcat:
```
$ nc <IP addr> 4470
```
Also, port can be specified as cmdline parameter:
```
$ ./tcpsh 24680
```
