# Numstore

A Numerical Data Base

## Usage

### Create Dataset
```
$ ./numstore example.db create data_name F32
```

### Delete Dataset
```
$ ./numstore example.db delete data_name
```

### Write to Dataset
```
$ gen_data | ./numstore example.db write data_name 
$ gen_data | ./numstore example.db append data_name 
```

### Read from Dataset
```
$ ./numstore example.db read data_name > output.bin
$ ./numstore example.db read data_name 0:10 > output.bin
$ ./numstore example.db read data_name 0:10:2 > output.bin
$ ./numstore example.db read data_name 0::10 > output.bin
```
