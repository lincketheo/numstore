# Numstore

A way to store and stream native C-type formatted numbers

## Features:
1. Storing numbers in formats 
    - unsigned/signed ints (8, 16, 32, 64 bits)
    - floats (16, 32, 64 bits)
    - complex int / float pairs (2x8, 2x16, 2x32, 2x64 unsigned / signed ints, floats)
2. Streaming input/output data via the following mechanisms:
    - Unix Pipes
    - Sockets
    - Websockets

## Usage:

### Unix stream interface
```
$ numstore db create foo  
$ numstore db foo ds create bar 
$ numstore ds foo:bar read > out.bin
$ numstore ds foo:bar read 0:10:5 > out.bin
$ numstore ds foo:bar read 0::100 > out.bin
$ cat file | numstore ds foo:bar write 
$ cat file | numstore ds foo:bar write 1:10
$ cat file | numstore ds foo:bar append
```

#### Details:
Over writes indexes 1:10 of foo:bar with data in file. If len(foo:bar) < 10, then appends the data on the end
```
$ cat file | numstore ds foo:bar write 1:10
```


