# nsfslite-java

Java bindings for NumStore File System Lite (nsfslite).

## Overview

This is a Java wrapper for the nsfslite C library. The API closely mirrors the C API, using static methods that operate on opaque handles (represented as `long` values).

## Building

### Prerequisites

- Java 11 or higher
- Gradle
- CMake 3.16+
- JNI development headers

### Build Steps

```bash
cd java
./gradlew build
```

This will:
1. Build the native JNI library using CMake
2. Compile the Java sources
3. Package everything into a JAR file

The native library will be included in the JAR under `native/`.

## Usage

### Basic Example

```java
import com.numstore.nsfslite.Nsfslite;

public class Example {
    public static void main(String[] args) {
        // Open database
        long handle = Nsfslite.open("data.db", "data.wal");

        // Create a variable
        long varId = Nsfslite.newVar(handle, "myvar");

        // Insert data
        byte[] data = "Hello, World!".getBytes();
        Nsfslite.insert(handle, varId, 0, data);

        // Read data
        byte[] read = Nsfslite.read(handle, varId, 0, 13, 1);
        System.out.println(new String(read));  // Prints: Hello, World!

        // Get size
        long size = Nsfslite.fsize(handle, varId);
        System.out.println("Variable size: " + size);

        // Close database
        Nsfslite.close(handle);
    }
}
```

### Transaction Example

```java
long handle = Nsfslite.open("data.db", "data.wal");
long varId = Nsfslite.newVar(handle, "txn_test");

// Begin transaction
long txn = Nsfslite.beginTxn(handle);

// Perform operations (Note: transaction support in insert/write/remove
// operations is handled internally)

// Commit transaction
Nsfslite.commit(handle, txn);

Nsfslite.close(handle);
```

### Slice Operations

The read, write, and remove operations support Python-style slice syntax `[start:stop:step]`:

```java
long handle = Nsfslite.open("data.db", "data.wal");
long varId = Nsfslite.newVar(handle, "slices");

// Insert data
Nsfslite.insert(handle, varId, 0, "0123456789".getBytes());

// Read every other byte: [0:10:2]
byte[] everyOther = Nsfslite.read(handle, varId, 0, 10, 2);
System.out.println(new String(everyOther));  // Prints: 02468

// Write data
Nsfslite.write(handle, varId, 0, 5, 1, "HELLO".getBytes());

// Remove and return data
byte[] removed = Nsfslite.removeAndReturn(handle, varId, 0, 5, 1);
System.out.println(new String(removed));  // Prints: HELLO

Nsfslite.close(handle);
```

## API Reference

All methods are static and in the `com.numstore.nsfslite.Nsfslite` class:

### Lifecycle
- `long open(String dbPath, String walPath)` - Open database connection
- `void close(long handle)` - Close database connection

### Variable Management
- `long newVar(long handle, String name)` - Create new variable
- `long getId(long handle, String name)` - Get variable ID by name
- `void delete(long handle, String name)` - Delete variable
- `long fsize(long handle, long id)` - Get variable size

### Data Operations
- `void insert(long handle, long id, long offset, byte[] data)` - Insert data
- `byte[] read(long handle, long id, long start, long stop, long step)` - Read data
- `void write(long handle, long id, long start, long stop, long step, byte[] data)` - Write data
- `void remove(long handle, long id, long start, long stop, long step)` - Remove data
- `byte[] removeAndReturn(long handle, long id, long start, long stop, long step)` - Remove and return data

### Transactions
- `long beginTxn(long handle)` - Begin transaction
- `void commit(long handle, long txn)` - Commit transaction

## Comparison with C API

| C API | Java API |
|-------|----------|
| `nsfslite *nsfslite_open(...)` | `long Nsfslite.open(...)` |
| `nsfslite_close(n)` | `Nsfslite.close(handle)` |
| `nsfslite_new(n, name)` | `Nsfslite.newVar(handle, name)` |
| `nsfslite_get_id(n, name)` | `Nsfslite.getId(handle, name)` |
| `nsfslite_delete(n, name)` | `Nsfslite.delete(handle, name)` |
| `nsfslite_fsize(n, id)` | `Nsfslite.fsize(handle, id)` |
| `nsfslite_insert(n, id, ...)` | `Nsfslite.insert(handle, id, ...)` |
| `nsfslite_read(n, id, ...)` | `Nsfslite.read(handle, id, ...)` |
| `nsfslite_write(n, id, ...)` | `Nsfslite.write(handle, id, ...)` |
| `nsfslite_remove(n, id, ...)` | `Nsfslite.remove(handle, id, ...)` |
| `nsfslite_begin_txn(n)` | `Nsfslite.beginTxn(handle)` |
| `nsfslite_commit(n, tx)` | `Nsfslite.commit(handle, txn)` |

## License

Same as the main NumStore project.
