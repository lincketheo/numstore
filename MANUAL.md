# 1.0 Numbstore Technical Manual

## 1.1.0 Features

### 1.1.1.0 Writing

#### 1.1.1.1.0 Sockets

1. Open up the socket for writing variables "a" "b" "c"
```
WRITE 5 [(a, b), c] { data }
WRITE 5 [(a, b), c, (d, e), f, g] { data }
```

- The invariant is that for this packet, there will be 5 a's, 5 b's, 5 c's
    - Therefore, only one instance of each variable is allowed
    - and only one depth layer is allowed
```
# Not allowed
WRITE 5 [(a, a, b), c] 
WRITE 5 [(a, b), c, b] 
WRITE 6 [([a, b], c), d] 
WRITE 7 [([a, b], c), d] 
```

2. Open up the same pattern, but push format first
```
SET [(a, b), c]
WRITE 5 {data}
WRITE 10 {data}
```

- Termination
    - You must specify the size every time 
    - If long variable sequence (cause redundant bytes), just use `SET`
    - Other considered options
        - Stop on Timeout
        - Magic sequence (use some probability to say it's < 0.00005% chance or something)
        - Magic sequence handshake

- Question:
    - Is this too restrictive and are there other combinations of variables that I haven't thought of?
