# Contributing to NumStore

Thank you for your interest in contributing to NumStore! This guide will help you get started.

## Before You Start

### Opening Issues

Before working on a feature or bug fix, please:
- Check if an issue already exists for your idea
- Open a new issue to discuss your proposed changes
- Wait for feedback from maintainers before starting major work

### Reporting Security Vulnerabilities

**Do not open public issues for security vulnerabilities.** Instead, please report them privately to the maintainers.

### Code of Conduct

All contributors are expected to follow professional conduct standards. Be respectful, constructive, and collaborative.

## Finding Issues to Work On

### Good First Issues

New contributors should look for issues tagged with `good first issue`. These are specifically chosen to be approachable for those new to the codebase.

### Help Wanted

Issues tagged with `help wanted` are ready for contribution and have been approved by maintainers.

## Setting Up Your Development Environment

### Required Tools

- **C Compiler**: GCC or Clang
- **CMake**: Version 3.10 or higher
- **Git**: For version control
- **Python 3.8+**: (optional) For Python bindings
- **Java 8+**: (optional) For Java bindings

### Building the Project

```bash
# Clone the repository
git clone https://github.com/Numstore/numstore.git
cd numstore

# Build in debug mode
make debug

# Or build in release mode
make release

# Run tests
make test
```

### IDE Setup

The project works well with:
- **VSCode**: Install C/C++ extension
- **CLion**: Works out of the box with CMake
- **Vim/Neovim**: Use compile_commands.json for LSP support

## Making Your Change

### Code Guidelines

#### Testing
- Add tests for all new functionality
- Ensure existing tests pass: `make test`
- Test examples still compile and run correctly

#### Code Style
- Follow existing code style (use `make format` to auto-format)
- Use meaningful variable and function names
- Keep functions focused and reasonably sized

#### Documentation
- Add documentation headers to all public functions
- Update example files if adding new features
- Include usage examples where appropriate

#### Error Handling
- Use goto-based cleanup pattern for error handling
- Always check return values
- Provide meaningful error messages

#### Copyright Headers
All new `.c` and `.h` files must include the Apache 2.0 license header:

```c
/*
 * Copyright (c) 2025 Theo Lincke
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
```

### Commit Messages

Write clear, descriptive commit messages:

```
Brief summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain what changed and why, not how (the diff shows how).

- Use bullet points for multiple changes
- Reference issue numbers: Fixes #123
```

### Testing Your Changes

Before submitting:
1. Run `make clean && make debug` to ensure clean build
2. Run `make test` to verify all tests pass
3. Test relevant examples manually
4. Check for memory leaks with valgrind if appropriate

## Submitting Your Change

### Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make your changes following the guidelines above
4. Test thoroughly
5. Commit with clear messages
6. Push to your fork
7. Open a Pull Request with:
   - Clear description of changes
   - Reference to related issues
   - Summary of testing performed

### Code Review

- All PRs require review from maintainers
- Be responsive to feedback
- Make requested changes in new commits (don't force-push)
- Once approved, maintainers will merge your PR

## Useful Resources

### Project Structure

```
numstore/
├── libs/           # Core libraries
│   ├── nscore/     # Core utilities
│   ├── nspager/    # Pager implementation
│   └── apps/       # Application libraries (nsfslite, etc.)
├── apps/           # Example applications and tools
├── python/         # Python bindings
├── java/           # Java bindings
├── docs/           # Documentation
└── cmake/          # CMake modules
```

### Building Specific Targets

```bash
# Build only debug libraries
make debug

# Build with tests enabled
make debug-ntests

# Run specific test suites
make nstypes-tests
make nspager-tests

# Format all code
make format

# Clean everything
make clean
```

### Memory Leak Detection

```bash
# Run tests with valgrind
make valgrind-tests
```

### Getting Help

- Open a discussion on GitHub
- Ask questions in existing issues
- Review documentation in `docs/`

## License

By contributing to NumStore, you agree that your contributions will be licensed under the Apache License 2.0.
