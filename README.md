# Numstore Language Guide

## Release Version
VERSION 0.0.1-WIP
These docs are for feature posterity only. v0.0.1 is still a wip

Numstore is a simple compact, expressive language for recording contiguous "memory-shaped" array data.  

--------------------------------------------------------------------------------
## CREATE

Use the **create** command to define new variables and data structures.

A variable can be thought of as a set of data collection points. Each element of a variable is a data type, 
and the variable as a whole is one big growing array.

Variable types can be primitive or structured types. If you're familiar with C, the types you can express 
in numstore are pretty much the same, with some added features.

### Primitive Types
```
create a U32  
```
(This creates a vector of unsigned 32-bit integers.)

You can create variables with any primitive type from the following list:

- **Primitive Types:**
    - **Signed Integers:** 
        - i8
        - i16
        - i32
        - i64 
    - **Unsigned Integers:** 
        - u8
        - u16
        - u32
        - u64
    - **Floating Point:** 
        - f16
        - f32
        - f64
        - f128
    - **Floating Complex Numbers:** 
        - cf32
        - cf64
        - cf128
        - cf256
    - **Signed Complex Integers:** 
        - ci16
        - ci32
        - ci64
        - ci128
    - **Unsigned Complex Integers:** 
        - cu16
        - cu32
        - cu64
        - cu128
    - **Others:** 
        - bool
        - char
        - bit

You can also create composite variables:

- **Composite Types**
    - **Structs:**  
    ```
    create a struct { a u32, b i32, c bool }  
    ```
    - **Variable Length Arrays:**  
    ```
    create b [][][][]i32  
    ```
    - **Strict Length Arrays:**  
    ```
    create c [10][20][5]u32  
    ```
    - **Union:**  
    ``` 
    create d union { a i32, b bit }  
    ```

    - **Enum:**  
    ``` 
    create e enum { FOO, BAR }  
    ```

Composite variables can be nested as deeply as you like. The general format is `VNAME TYPE` where `VNAME` 
is the variable name and `TYPE` is any primitive or composite type. For example:
```
create a union { a struct { b enum { FOO, BAR }, c struct { d f32, e cf64 } }, f [][][]bit, g [10]i8 }
```

Variable names can't conflict with each other at the same layer, but notice how `a` has a struct field `a`. 
If it can be considered unique by full access path (e.g. `a` is unique from `a.a`, then it's valid).

### Scalars
Earlier, we created variables that you can _write_ and _read_ data to using the write and read commands. 
But oftentimes, we want our data to have properties. You can define _scalars_ on data types, by referencing 
them via a dot (`.`). Scalars can be any of the types we learned about above.

```
create a.fs F32 10.1;  
create b.z struct { a i32, b i64 } { "a": 5, "b": 10 }  
```

--------------------------------------------------------------------------------
## WRITE

Use **write** to append data to a variable. You can specify types explicitly or rely on the existing variable structure.

In general, _how_ you write depends on the input interface. In general, there are three ways of providing data for a write:
1. Provide it as type based syntax on the command line
```
create a i32
write 5 a 1 2 3 4 5

create b struct { a i32 }
write 2 b { a : 5, b : 10 }
```
2. Provide valid json for your query:
```
create a i32
write { "a" : [1, 2, 3, 4] }

create b struct { a i32 }
write { "b" : { "a" : 5, "b" : 10 } }
```
3. Write your data in binary contiguous format (useful for embedded systems)
```
create a i32
write 5 a BINARY DATA 

create b struct { a i32 }
write 5 b BINARY DATA
```

You can also _tie_ variables together. More on this in the relation section.
Let's create 3 representative variables:
```
create a i32 
create b u32
create c u32
```

1. Tangled syntax. You can tangle variables together using brackets (`[a, b]`), or tuples (`(a, b)`), or a nesting of tuples under brackets (`[a, b, (c, d), e, (f, g), h]`)
    - A bracket denotes sequential data.
    ```
    write 3 [a, b] 1, 2, 3, 4, 5, 6
    ```
    - This write operation wrote `[1, 2, 3]` to a and `[4, 5, 6]` to b
    ```
    write 4 (a, b) 1, 2, 3, 4, 5, 6
    ```
    - This write operation wrote `[1, 3, 5]` to a and `[2, 4, 6]` to b
    ```
    write 2 [a, (b, c)] 1, 2, 3, 4, 5, 6
    ```
    - This write operation wrote `[1, 2]` to a, `[3, 5]` to b and `[4, 6]` to c
    - Tangled syntax isn't meant to make things more complex. It just allows a little bit more flow control. 
      Let's say I have two sensors I want to record data with, sensor a and b, and I read sensor a first in 
      a block of 10 measurements and then b in a block of 10 measurements

You can also "prime your write" using the open command:
```
open write a [a, b, (c, d)]
write 1 BIN
write 2 BIN
write 1 BIN
write 5 BIN
write 1 BIN
close a
```
This is useful when you have a large query string and lots of mini writes, or want to just write 1 element at a time

--------------------------------------------------------------------------------
## READ

The **read** command extracts data from a variable over a given range, applying transformations if specified.
- **Example:**  
```
read 0:10 a // Reads elements [0, 10) of a
read 9:-1:5 b // Reads elements [9, len(b)) with a stride of 5
read 0 c // Reads element 0 of c
```
- You can

  CODE: read 0:10 [a.t * a[:0:-1], (a.t * idx(a), c[0:1] + b[0:10])]  
  (This reads a subset from variable a, applies a transformation by multiplying by its reversed slice and index, and then combines this with computed values from arrays c and b.)


--------------------------------------------------------------------------------
## Basic Array Operations

You can operate on arrays using any of the following basic array operations:
- **Arithmetic:** +, -, *, /, %, **
- **Comparisons:** >, <, >=, <=, ==, !=
- **Bitwise:** &, |, ^, ~

```
read 0:10 [a.t * idx(a), b[0:10] + c[0:1]]  
```

In general, if it feels right, you won't get an error. You can't for example, multiply two structs together
```
create a struct { i i32 }
create b f32
XXX read 0:10 [a * b]
```

Here are things you cannot do:
- **Invalid array operations**
    - You _can_ multiply strict arrays together, and variable length arrays as long as your query can broadcast shapes together.

When combining arrays, the operation applies to the intersection of valid indices.

--------------------------------------------------------------------------------
## Casting

You can operate on arrays using any of the following basic array operations:
- **Arithmetic:** +, -, *, /, %, **
- **Comparisons:** >, <, >=, <=, ==, !=
- **Bitwise:** &, |, ^, ~

```
read 0:10 [a.t * idx(a), b[0:10] + c[0:1]]  
```

In general, if it feels right, you won't get an error. You can't for example, multiply two structs together
```
create a struct { i i32 }
create b f32
XXX read 0:10 [a * b]
```

Here are things you cannot do:
- **Invalid array operations**
    - You _can_ multiply strict arrays together, and variable length arrays as long as your query can broadcast shapes together.

When combining arrays, the operation applies to the intersection of valid indices.

--------------------------------------------------------------------------------
## TAKE

The **take** command works like read, but it also removes the retrieved data from the variable.
- **Example:**  
  CODE: take 0:10 [a + b]  
  (This returns and removes the first ten elements of the sum of arrays a and b.)

--------------------------------------------------------------------------------
## DELETE

The **delete** command removes data entirely or within a specific slice.
- **Delete Entire Variable:**  
  CODE: delete a  
- **Delete a Slice:**  
  CODE: delete a[0:10]  
  (Removes elements from index 0 to 9.)

--------------------------------------------------------------------------------
## ERRORS

Numstore can report various errors. Some common error types include:
1. Variable doesn't exist.
2. Divide by zero.
3. Invalid cast.
4. Float underflow.
5. Float overflow.
6. Shape mismatch (array dimensions incompatible).
7. Invalid axis (when performing operations on arrays).
8. Index out of bounds.
9. Type errors (such as using a bit in a context expecting a float).
10. Broadcasting errors (when array sizes do not align).

--------------------------------------------------------------------------------
## ADDITIONAL EXAMPLES

- **Creating a Nested Struct with Arrays:**
  CODE: create person struct { name [50]char, age u8, scores []f32 }  
  (Defines a person structure with a fixed-size name, an age, and a variable-length array for scores.)

- **Mapping Complex Operations:**
  CODE: read 5:15 [ (a * 2) + idx(a), b / 2 ]  
  (Doubles values from a, adds the index, and reads half the value of array b.)

- **Conditional Updates:**
  CODE: write 3 [ status enum { ACTIVE, INACTIVE, SUSPENDED } ] ACTIVE  
  (Writes the ACTIVE state to the status variable at index 3.)

- **Combining Data from Multiple Arrays:**
  CODE: take 0:5 [ a + b - c ]  
  (Retrieves and removes five elements by combining arrays a, b, and subtracting c.)

--------------------------------------------------------------------------------

## Internal Implementation Details (data structures)
Numstore is fast. It's designed using concepts from database design and data systems like B+Trees and indexes.

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
    [c,      f,     j]         [m, t, w]
   /       |      |    \
[a,    b] [c, ...] ...    ...    ...  ...  ...
 |     |   |
Data Data Data
```

Where Data is pointer to the root of the following B+Tree as well as other variable meta data like shape, data type etc.

And one for indexes of the contiguous data that is split into page sized chunks.
For this example, I'll consider the array `[9, 10, 11, 13, 15, 1, 0, 19]`.
Let's say our page size is 2 elements (Yeah, you're right, that's tiny).
So our array is represented as a bunch of pages with their starting index:
- `0 [9, 10]`
- `2 [11, 13]`
- `4 [15, 1]`
- `6 [0 19]`

We store `0, 2, 4, 6` in our B+Tree with the arrays linked to one another on the bottom layer:
```
      [2,           4]
    /         |         \
 [0]         [2]         [4,      6]
  |           |           |       |
[9, 10] -> [11, 13] -> [15, 1], [0, 19]
```
