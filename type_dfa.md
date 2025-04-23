# Type CFG

```
T -> p |
     struct { i T K } |
     union { i T K } |
     enum { i I } |
     V T |
     S T 

K -> \eps | , T K
V -> [] V | []
S -> [NUM] S | [NUM]
I -> \eps | , i I
```

```
T = TYPE 
K = KEY_VALUE_LIST
V = VARRAY_BRACKETS 
S = SARRAY_BRACKETS
I = ENUM_KEY_LIST
```
