


Script / Schema
```
CREATE DATASET match CF64;
CREATE DATASET signal CF64;
```

Bash - Writes data to match
```
$ ./program1 | ./numstore write match
$ ./program2 | ./numstore write signal
```

```
CREATE TRANSFORM(vec(CF64, l), vec(CF64, k)) -> vec(corl, F32, U64, 
);
```

caf_transform.c
```
int caf_corl(complex float* left, size_t llen, complex float* right, size_t rlen) {
    float f0, f1, df;
    float corls[rows][cols]; 

    // compute caf

    for(int r = 0; r < rows; ++r) {
        for(int c = 0; c < cols; ++c) {
            emit("caf_transform", corls[r][c], f0 + r*df, c);
        }
    }
}
```

```
SELECT * from caf_corl (match, signal);
```
