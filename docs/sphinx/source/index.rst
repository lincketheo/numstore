===================
Numstore Specification
===================

Numstore
========

A high-performance database for accessing contiguous byte streams by index using R+Tree spatial indexing. Sustained write speeds exceed 640 MB/s.

Core applications operate on raw byte streams without type system constraints.

nsdb
====

Type System
-----------

::

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

Primitives
^^^^^^^^^^

==================  ====================================
Type Class          Variants
==================  ====================================
Signed Integer      I8, I16, I32, I64
Unsigned Integer    U8, U16, U32, U64
Floating Point      F16, F32, F64, F128
Complex Integer     CI16, CI32, CI64, CI128, CI256
Complex Unsigned    CU16, CU32, CU64, CU128, CU256
Complex Float       CF32, CF64, CF128, CF256
==================  ====================================

Server Protocol
---------------

::

    create VNAME TYPE;
    delete VNAME;
    close VNAME:PORT;
    insert VNAME:PORT OFFSET (NELEMS);
    remove VNAME START:STOP:STRIDE;
    read VNAME:PORT[ACCESSOR] START:STOP:STRIDE;
    write VNAME:PORT[ACCESSOR] START:STOP:STRIDE;
    take VNAME:PORT[ACCESSOR] START:STOP:STRIDE;

**insert**: Opens TCP socket to receive data for insertion - If infinite insert, socket closes when you call close VNAME:port, otherwise closes on termination of insert range

**remove**: Removes data in specified range

**read**: Opens TCP socket to transmit byte stream - If infinite read, socket closes when you call close VNAME:port, otherwise closes on termination of read range

**write**: Opens TCP socket to receive packed bytes - If infinite write, socket closes when you call close VNAME:port, otherwise closes on termination of write range

**take**: Removes data and transmits via TCP socket - If infinite take, socket closes when you call close VNAME:port, otherwise closes on termination of take range

CLI Protocol
------------

::

    ./nstore "QUERY"

Examples::

    ./nstore "create a struct { x i32, y f64 }"
    cat source | ./nstore "insert VNAME OFFSET"
    cat source | ./nstore "write VNAME[ACCESSOR] START:STOP:STRIDE"
    ./nstore "read VNAME[ACCESSOR] START:STOP:STRIDE" > output
    ./nstore "remove VNAME START:STOP:STRIDE"
    ./nstore "take VNAME[ACCESSOR] START:STOP:STRIDE" > output

CLI uses stdin/stdout for data transfer. PORT specification omitted.
