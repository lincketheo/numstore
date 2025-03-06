# Conventions 
## Size in Bytes vs Size in Elements 
Consider 
```
int* foo = malloc(10 * sizeof*foo);
void* bar = foo;
```

- `nelem` - Number of elements in foo 
- `capelem` - Number of elements foo can hold 
- `lenb` - Number of bytes in bar 
- `cap` - Number of bytes bar can hold
