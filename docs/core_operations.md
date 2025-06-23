# Core Operations
The things you can _do_ in numstore

## Variables
Variables are collection of values of a set type. 

### Ordering
Although I don't _want_ you to think of variables as having any ordering 
(they're sets, not "arrays") I know you will. It's natural to think 
the ordering on variables is the ordering in which you inserted them into the array. 

1. By default, the "ordering" on a variable is it's insertion ordering. So if you 
interact with data in any way that requires some "index", it assumes the index 
is the insertion order:

```
create a u32;
append { "a" : 1 };
append { "a" : [10, 20, 1, 3] };
read a[0::2];
>> [1, 20, 3]
```

See how we never declared the ordering on "a". So when we read the variable "a",
it return values of a at index 0 - infinity with stride 2.

Another example:
```
create a u32;
append { "a" : 1 };
append { "a" : [10, 20, 1, 3] };
insert 1 { "a" : 5 }
read a[0::1];
>> [1, 5, 10, 20, 1, 3]
```

In this example, we inserted { "a" : 5 } at index 1. What ordering does index 1 refer to? 
The insertion ordering.

By the way, append is the same as insert, except it prepends an imaginary `-1` to the command. The 
general form of insert is:
```
insert START DATA
```

Without an ordering clause START is assumed the insertion ordering

2. Orderings are their own things. There are some special orderings:
```
create a struct { t f32, v u32 };
insert { "a" : { "t" : 1, "v" : 5 } };
insert { "a" : 
        [
          { "t": 3, "v": 7 },
          { "t": 1, "v": 5 },
          { "t": 6, "v": 2 },
          { "t": 4, "v": 9 }
        ]
    }
insert { "a" : [10, 20, 1, 3] };
read a[0::2];
>> [1, 20, 3]
```

Therefore, a key 
paradigm is that 

## Arrays
**append** data 
```
append { "a" : 1 }
```
**insert** data starting at an index
```
create a [10] u32;
insert 1 { "a" : 1 } # USes
insert idx 1 { "a" : 1 }
```

