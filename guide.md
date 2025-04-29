
# create
Request:
```
create VNAME TYPE
```

Response (json):
```
{ "status" : "SUCCESS" }
{ "status" : "ERROR" , "message" : "foo bar biz" }
```

# bcreate
Request:
```
bcreate VNAME TYPE
```

- Response (2 bytes):
    - Error (E)
    - Success (S)
```
E<ERROR TYPE>
S
```

# write
Request:
```
create a F32
write { "a" : 9.123 }
write { "a" : [1.1, 2.3, 3, 4] }

create b struct { a F32, c U32 }
write { "b" : { "a" : 1.4, "c" : 9 } }
write { "b" : [{ "a" : 1.4, "c" : 9 }, {"a" : 9.1, "c" : 10 }] }

write { "a" : [1, 2], "b" : [{ "a" : 1.4, "c" : 9 }, {"a" : 9.1, "c" : 10 }] }

create c union { a F32, c U32 }
write { "c" : { "a" : 1.4 } }
write { "c" : [{ "a" : 1.4, "c" : 9 }, {"a" : 9.1, "c" : 10 }] }
```

Response:
```
E<ERROR TYPE>
```

# bwrite
Request:
```
create a F32
bwrite a :DATA

create b struct { a F32, c U32 }
bwrite b :DATA

create c union

bwrite { "a" : [1, 2], "b" : [{ "a" : 1.4, "c" : 9 }, {"a" : 9.1, "c" : 10 }] }
```

Response:
```
E<ERROR TYPE>
```
