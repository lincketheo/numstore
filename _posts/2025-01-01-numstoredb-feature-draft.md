# Numstore DB 

When I originally set out to write numstore, I wasn't thinking of storing just bytes. I had the idea of storing numerical data in contiguous layout. You can see the status of this project in `libs/apps/nsdblite`.

Here's what I'm working towards:

## A Rich Type System

You can see my work on this in `nscompiler` and `nstypes`.

Types are like C types (I decided C actually has one of the best type systems around—it has product and sum types and primitive types, which is all you really need).
```
TYPE = STRUCT | UNION | SARRAY | PRIM | ENUM

STRUCT = struct {
    field_a TYPE,
    field_b TYPE,
    ...
}

UNION = union {
    variant_a TYPE,
    variant_b TYPE
}

SARRAY = [NUM] TYPE

ENUM = enum {
    VARIANT_A,
    VARIANT_B,
    ...
}
```

### Primitives (PRIM)

| Type Class       | Variants                           |
|------------------|------------------------------------|
| Signed Integer   | I8, I16, I32, I64                  |
| Unsigned Integer | U8, U16, U32, U64                  |
| Floating Point   | F16, F32, F64, F128                |
| Complex Integer  | CI16, CI32, CI64, CI128, CI256     |
| Complex Unsigned | CU16, CU32, CU64, CU128, CU256     |
| Complex Float    | CF32, CF64, CF128, CF256           |

## Server Protocol

I want to have a TCP/UDP server protocol to easily create open streams for reads/inserts etc.

The idea would be that you have a control port and local ports. When you open local ports for inserting (`insert VNAME:PORT OFFSET (NELEMS);`), you open that port and _any data written to that port gets written to the database_.

I think this is manageable. And when nelems is 0, the stream is infinite and you can close that port.

I would also like inter-type access patterns. So if I have a `foo struct { a i32, b u32 }` I can do `read [foo.b, foo.a]` as an access pattern that would reverse `foo`'s fields in the resulting output.
```
create VNAME TYPE;      
delete VNAME;
close VNAME:PORT;
insert VNAME:PORT OFFSET (NELEMS);
remove VNAME START:STOP:STRIDE;
read VNAME:PORT[ACCESSOR] START:STOP:STRIDE;
write VNAME:PORT[ACCESSOR] START:STOP:STRIDE;
take VNAME:PORT[ACCESSOR] START:STOP:STRIDE;
```

**insert**: Opens TCP socket to receive data for insertion. If infinite insert, socket closes when you call `close VNAME:port`, otherwise closes on termination of insert range.

**remove**: Removes data in specified range.

**read**: Opens TCP socket to transmit byte stream. If infinite read, socket closes when you call `close VNAME:port`, otherwise closes on termination of read range.

**write**: Opens TCP socket to receive packed bytes. If infinite write, socket closes when you call `close VNAME:port`, otherwise closes on termination of write range.

**take**: Removes data and transmits via TCP socket. If infinite take, socket closes when you call `close VNAME:port`, otherwise closes on termination of take range.

## CLI Protocol

I also want a similar protocol on the command line interface, just with stdin/stdout or other provided files as arguments:
```
./nstore "QUERY"
```

Examples:
```bash
./nstore "create a struct { x i32, y f64 }"
cat source | ./nstore "insert VNAME OFFSET"
cat source | ./nstore "write VNAME[ACCESSOR] START:STOP:STRIDE"
./nstore "read VNAME[ACCESSOR] START:STOP:STRIDE" > output
./nstore "remove VNAME START:STOP:STRIDE"
./nstore "take VNAME[ACCESSOR] START:STOP:STRIDE" > output
```

CLI uses stdin/stdout for data transfer similar to `cat` or other file operators. PORT specification is omitted.
