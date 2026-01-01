<p align="center">
  <img src="docs/web/src/assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# NumStore

![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)
![Version](https://img.shields.io/badge/version-0.1.0-brightgreen.svg)
![Build](https://img.shields.io/badge/build-passing-success.svg)
![C Standard](https://img.shields.io/badge/C-C11-blue.svg)

**NumStore** is a high-performance, persistent storage library designed for numerical and scientific computing workloads. Built in C with bindings for Python and Java, it provides efficient storage and retrieval of variable-length arrays with advanced features like strided access, transactions, and crash recovery.

---

## 🚀 Features

- **Persistent Storage**: Automatic persistence with write-ahead logging (WAL)
- **Flexible Access Patterns**: Support for strided reads/writes and offset-based operations
- **Transaction Support**: ACID-compliant transactions for data consistency
- **Crash Recovery**: Automatic recovery from unexpected failures
- **Zero-Copy Operations**: Efficient data handling with minimal memory overhead
- **Multi-Language Support**: Native C library with Python and Java bindings
- **Simple API**: Clean, intuitive interface for common operations

## 🛠 Quick Start

### Prerequisites

- C Compiler (GCC or Clang)
- CMake 3.10+
- Python 3.8+ (optional, for Python bindings)
- Java 8+ (optional, for Java bindings)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Numstore/numstore.git
cd numstore

# Build the project
make debug

# Run tests
make test
```

### Basic Usage (C)

```c
#include "nsfslite.h"

// Open database
nsfslite *db = nsfslite_open("data.db", "data.wal");

// Create a variable
int64_t var_id = nsfslite_new(db, "mydata");

// Insert data
int data[] = {1, 2, 3, 4, 5};
nsfslite_insert(db, var_id, NULL, data, 0, sizeof(int), 5);

// Read data back
int read_data[5];
nsfslite_read(db, var_id, read_data, sizeof(int),
              (struct nsfslite_stride){.bstart = 0, .stride = 1, .nelems = 5});

// Close database
nsfslite_close(db);
```

### Basic Usage (Python)

```python
import numstore.nsfslite as nsfs

# Open database
db = nsfs.open("data.db", "data.wal")

# Create and write data
var_id = nsfs.create_variable(db, "mydata")
nsfs.variable_insert(db, var_id, 0, bytes([1, 2, 3, 4, 5]))

# Read data back
data = nsfs.variable_read(db, var_id, 0, 5, 1)

# Close database
nsfs.close(db)
```

## 📚 Documentation

- **User Guide**: [https://numstore.io/docs](https://numstore.io/docs) *(coming soon)*
- **API Reference**: [https://numstore.io/api](https://numstore.io/api) *(coming soon)*
- **Examples**: See the `apps/examples/` directory for comprehensive examples

## 🎯 Use Cases

NumStore is designed for:

- **Scientific Computing**: Store and retrieve large numerical datasets efficiently
- **Data Analytics**: Persistent storage for analytical workloads
- **Machine Learning**: Dataset versioning and storage for ML pipelines
- **Time Series Data**: Efficient storage of sequential numerical data
- **Array Operations**: Fast access to multi-dimensional array data

## 🏗 Project Structure

```
numstore/
├── libs/              # Core libraries
│   ├── nscore/        # Core utilities and error handling
│   ├── nspager/       # Page-based storage engine
│   └── apps/          # Application libraries
│       ├── nsfslite/  # Lightweight file storage API
│       └── nsdblite/  # Database-like storage API
├── apps/              # Example applications and tools
│   └── examples/      # 15+ comprehensive examples
├── python/            # Python bindings
├── java/              # Java bindings
├── docs/              # Documentation
└── cmake/             # CMake build modules
```

## 🧪 Examples

NumStore includes 15 comprehensive examples demonstrating various features:

1. **Basic Persistence** - Simple data storage and recovery
2. **Crash Recovery** - WAL-based crash recovery
3. **Insert Operations** - Inserting data at arbitrary positions
4. **Strided Access** - Reading/writing with custom stride patterns
5. **Offset Operations** - Offset-based random access
6. **Remove Operations** - Deleting data ranges
7. **Transactions** - Atomic multi-operation transactions
8. **Mixed Operations** - Combining multiple operation types

Run examples:
```bash
# Build examples
make debug

# Run an example
./build/debug/apps/examples/nsfslite/example1_basic_persistence
```

## 💡 Advanced Features

### Strided Access

Access data with custom stride patterns for efficient multi-dimensional array handling:

```c
// Read every 5th element
struct nsfslite_stride stride = {
    .bstart = 0,
    .stride = 5,
    .nelems = 100
};
nsfslite_read(db, var_id, buffer, sizeof(int), stride);
```

### Transactions

Group multiple operations into atomic transactions:

```c
nsfslite_txn *txn = nsfslite_begin_txn(db);
nsfslite_insert(db, var_id, txn, data1, 0, sizeof(int), 100);
nsfslite_write(db, var_id, txn, data2, sizeof(int), stride);
nsfslite_commit(db, txn);
```

### Crash Recovery

Automatic recovery from crashes using write-ahead logging:

```c
// Database automatically recovers on open
nsfslite *db = nsfslite_open("data.db", "data.wal");
// All committed operations are preserved
```

## 🌱 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:

- Setting up your development environment
- Code style and testing requirements
- Submitting pull requests
- Reporting issues

## 📝 License

NumStore is licensed under the [Apache License 2.0](LICENSE).

```
Copyright 2025 Theo Lincke

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
```

## 🔗 Links

- **Website**: [https://numstore.io](https://numstore.io) *(coming soon)*
- **Documentation**: [https://numstore.io/docs](https://numstore.io/docs) *(coming soon)*
- **Issue Tracker**: [GitHub Issues](https://github.com/Numstore/numstore/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Numstore/numstore/discussions)

## 🏆 Status

NumStore is currently in **active development** (v0.1.0). The API is stabilizing but may change before the 1.0 release.

- ✅ Core storage engine
- ✅ Transaction support
- ✅ Crash recovery
- ✅ Python bindings
- ✅ Java bindings
- 🚧 Performance optimization
- 🚧 Comprehensive benchmarks
- 📅 Production-ready 1.0 release

---

**Built with ❤️ for the scientific computing community**
