<p align="center">
  <img src="docs/web/src/assets/logo.png" alt="NumStore Logo" width="200"/>
</p>

# NumStore

![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)
![Version](https://img.shields.io/badge/version-0.0.1-brightgreen.svg)
![Build](https://img.shields.io/badge/build-passing-success.svg)

Numstore is a database for contiguous bytes. 

It's essentially a file system, but in a database format with log N mutations in the middle.

I got the idea of an "R+Tree" which is just a rope equivalent to a B+Tree that keeps track of the count of bytes each node stores. It's designed for:

- log N Insertions anywhere in the byte stream (compared to N insertions in traditional file systems) 
- log N deletions anywhere in the byte stream
- log N reads 

When I say log N, I really mean it takes log N to find the location. Once it's found the location, it holds onto the bottom layer and it's real time. Ideally as fast as just appending to the file.

It's written entirely in C with just the lemon parser as a dependency and some posix apis (with a wrapper - I haven't prioritized testing cross platform, but I designed it in a way to work cross platform. That feature will come soon).

---

## 🏆 TODO

NumStore is currently in **active development** (v0.0.1). The API is stabilizing but may change before the 1.0 release.

- ✅ Core storage engine and R+Tree rebalance algorithm / pager
- ✅ Basic Transaction support
- 🚧 Crash Recovery 
    - TODO:
        - Finish Checkpoints 
- 🚧 Concurrency
    - TODO:
        - Verify lock table works
        - Add latches everywhere they need to be 
        - Add thread pool library some places:
            - WAL should be double buffered
            - Checkpoints should run in a background thread 
        - The whole latch idea is rough and buggy. example1 fails sporadically due to a deadlock
- 🚧 Java / Python Bindings
- 🚧 Performance optimization
    - TODO:
        - Optimize the WAL 
            - Combine 1 fsync for multiple commits 
            - Write tombstones as small log records (they don't need to be a full page) (x2 speed up)
            - Make a page request tombstone list method to request a chain of tombstones (x2 speed up)
- 🚧 Cross platform support 
    - TODO:
        - The os layer is defined, just not fully disciplined. I feel confident this is a 1-2 week full time task
- 🚧 Benchmarks and plots against other databases and / or linux file system
Failing tests

[FAILURE ]: spx_latch_x_waits_for_multiple_s
[FAILURE ]: aries_checkpoint_with_active_transactions
[FAILURE ]: aries_crash_before_commit
[FAILURE ]: aries_crash_before_commit_multiple
[FAILURE ]: aries_rollback_with_crash_recovery
[FAILURE ]: aries_rollback_clr_not_undone

---

## 🚀 Features

- **Persistent Storage**: Automatic persistence with undo redo write-ahead logging (WAL) using ARIES
- **Flexible Access Patterns**: Support for strided reads/writes and offset-based operations
- **Transaction Support**: ACID transactions for data consistency
- **Crash Recovery**: Automatic recovery from unexpected failures
- **Multi-Language Support**: Native C library with Python and Java bindings

## 🛠 Quick Start

### Prerequisites

- C Compiler (GCC or Clang)
- CMake 3.10+

### Building from Source

```bash
# Clone the repository
git clone https://gitlab.com/lincketheo/numstore.git
cd numstore

make 

./build/debug/apps/test
```

## 🏗 Project Structure

```
numstore/
├── libs/              # Core libraries
│   ├── nscore/        # Core utilities and error handling
│   ├── nspager/       # Page-based storage engine
│   └── apps/          # Application libraries
│       ├── nsfslite/  # Lightweight file storage API
├── apps/              # Example applications and tools
│   └── examples/      # 15+ comprehensive examples
├── wrappers/          # Other language wrappers of numstore
│   └── python/        # python wrapper
│   └── java/          # Java wrapper
├── docs/              # Documentation
└── cmake/             # CMake build modules
```

Run examples:
```bash
# Build examples
make 

# Run an example
./build/debug/apps/examples/nsfslite/example1_basic_persistence
```

## 🌱 Contributing

I welcome contributions. This project is early, so to whoever is reading this, please just be considerate. 

## 📝 License

NumStore is licensed under the [Apache License 2.0](LICENSE).

```
Copyright 2025 Theo Lincke

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
```

