# Numstore

A single file data storage solution for (key: string, value: contiguous c type data).

Consider three random variables "a" "b" and "c".
- "a" is a U32 (unsigned 32 bit int) with numpy-style shape (2,2)
- "b" is a F32 (32 bit floating point) with numpy-style shape (,)
- "c" is a CF64 (complex float - e.g. 64 bits - 2 x 32 bit floats) with numpy-style shape (,)

```
a: U32  (2, 2)  = [[[1, 2], [3, 4]], [[1, 1], [2, 2]], [[9, 9], [10, 11]]]
b: F32  (,)     = [  1.123,            2.1,              x               ]
c: CF64 (,)     = [  1.0 + 2i,         x,                2.1 + 3.123i    ]
```

***
- Notes:
    - `x` denotes not present. 
        - So for example, we took a measurement of variables a and b at index 1, but not c
    - This notation uses numpy style shaping _for the entries of the random variable_
        - So a is a collection of `(2, 2)` elements. Not a `(3, 2, 2)` array.
        - I did this to force you to think about `a, b and c` as "sets" instead 
          of vectors/ordered arrays.
***

## B+Trees
This database has two B+Trees, one for the variables - culminating in the data at the end
```
                                           [m]
                                          /   \
            [c,                    f, j]         [m, t, w]
           /                |        |  \         /   |   \
[(a->data), (b->data)] [(c->data).. ... ...    ...  ...  ...
```

And one for indexes of the contiguous data that is split into page sized chunks.
For this example, I'll consider the array `[9, 10, 11, 13, 15, 1, 0, 19]`.
Let's say our page size is 2 elements (Yeah, you're right, that's tiny).
So our array is represented as a bunch of pages with their starting index:
- `0 [9, 10]`
- `2 [11, 13]`
- `4 [15, 1]`
- `6 [0 19]`
- asd
We store `0, 2, 4, 6` in our B+Tree with the arrays linked to one another on the bottom layer:
```
      [2,           4]
    /         |         \
 [0]         [2]         [4,      6]
  |           |           |       |
[9, 10] -> [11, 13] -> [15, 1], [0, 19]
```
