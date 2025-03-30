# 1.0 Numstore Technical Manual

## 1.1.0 Features

### 1.1.1.0 Creating Variables 
1. Variables can be shaped or unshaped 
```
create a U32 5 10
create <var name> <type> <shape...>
create a U32 unshaped
```
- Unshaped just means the variable doesn't have any uniform shape 
    - It's generally discourageed to use unshaped data. 
      Part of the power of numstore is the simplicity of contiguous data, but when 
      a variable is unshaped, the "contiguous simple format" narrative breaks and data 
      requires a special format (read unshaped format). Meaning it's harder to write 
      to the variable contiguously because now the writer needs to understand the nature 
      of the format for shape


2. Add properties to variables.
```
create a U32 unshaped { "windowed": 10, ...., "prop1" = foo }
```

Properties are a way to add plugins to numstore. Basically a property allows you to add a custom key value property to a variable on the way it behaves. 
For example, the `windowed` property asserts that the variable can only have maximum of 10 elements. Properties are json, so you can add as much complex logic as 
you want. All properties must have a default 

TODO - Docs on how to add property plugins

### 1.1.1.0 Writing

1. Open up the socket for writing variables "a" "b" "c"
```
write 5 [(a, b), c] bytes{ data }
write 5 [(a, b), c, (d, e), f, g] bytes{ data }
```

- The invariant is that for this packet, there will be 5 a's, 5 b's, 5 c's
    - Therefore, only one instance of each variable is allowed
    - and only one depth layer is allowed
```
# Not allowed
write 5 [(a, a, b), c] 
write 5 [(a, b), c, b] 
write 6 [([a, b], c), d] 
write 7 [([a, b], c), d] 
```

2. Open up the same pattern, but push format first
```
SET [(a, b), c]
write 5 { data }
write 10 { data }
```

### 1.1.1.0 Reading
1. Open up a socket for reading variables "a" "b" "c" 
```
read 5 [(a, b), c] 
read 5 [(a, b[1:9], c), f[0], g][5:-10]
```

- Notes 
    - When you read indexes, only data that is available will be read _for all variables_
    - Let's say f is a non uniform shape array, then only results for all variables will show up when f has elements in it



3. Group by 
Let's say you've written two times
```
write 5 [a, c] bytes{ data }
write 5 [b, c] bytes{ data }
```

`a` and `b` Don't have a relation, but they are transatively related to one another. You can read `a` and `b` by querying a transative relation 

```
read 5 [a, b] by c
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
