# Announcing Numstore: A Database for the Machine Learning Era

I've been working on numstore in my free time for the past year, and I think it's time to talk about why it exists.

## The Problem

Consider these use cases:
- I want to access the i, j, k-th pixel of a list of images sized 256x256
- I want to pull slices of time-series sensor data across multiple dimensions
- I want to efficiently store and query multi-dimensional numerical arrays

A relational database wouldn't handle this very well. There are binary blobs, but those aren't treated as first-class citizens. You end up serializing/deserializing entire objects just to access a single element. It's clunky, slow, and fundamentally not designed for how machine learning workloads actually operate.

## Numpy and Data Locality

Think about how Python stores lists as a series of objects (PyObject's in C). That's fine for general-purpose programming, but terrible for numerical computation. Numpy was written because it just makes sense to store contiguous data in binary-packed streams.

**Numstore is to Postgres as Numpy is to Python arrays.** It's a step closer to the way machines actually process tightly-packed data.

## How the R+Tree Works

A R+Tree is a Rope data structure but instead of a binary tree, it's a B+tree variant. The key insight: rather than storing keys to data elements, **the keys are counts of elements in each subtree**. Text editors use ropes all the time for efficient data insertion in the middle of documents—this is just an extension into the database world.

This gives you O(log n) access to the n-th element without needing to maintain sorted keys. For insertions and deletions, you just update the counts as you rebalance—which you're doing anyway in a B+tree.

I'll say that again: **Numstore is like a file system where instead of copy-on-write (O(n) mutations in the middle), it allows for O(log n) internal mutations.** This is fundamentally a new way of thinking about files.

Want elements 1000 through 2000? The tree can efficiently:
- Locate element 1000 using count-based navigation
- Stream elements 1000-2000 sequentially from the leaf nodes
- Handle strided access (every k-th element) by skipping within and between leaf nodes

Once you access the nth element, writes and reads are (theoretically) as fast as writing/reading to a file minus one rebalance step at the end. Numstore is optimized for batch insertions, so inserting many pages at the bottom layer is first class. Same with deleting and reading. The idea is that once you've seeked, it's like you've opened up a TCP connection and can just write until you close (at which point the tree rebalances itself).

Combined with the fact that leaf nodes store contiguous byte arrays, you get cache-friendly sequential access for range queries while maintaining logarithmic random access.

## How It Works

Numstore breaks down its operations into the following operation codes:
(Note: convention is numbers that refer to a byte value are prefixed by `b`. Numbers that refer to units of values are not.)

Housekeeping Operations:
- **CREATE** - Initialize a new "variable" (e.g. a file)
- **DELETE** - Remove a variable

Data Manipulation Operations:
- **INSERT boffset** - Inserts bytes at byte offset `[boffset]`
- **READ bsize bstart stride nelems** - Reads strided data from byte offset `[bstart]` with element stride `[stride]` where each element is `[bsize]` bytes
- **REMOVE bsize bstart stride nelems** - Removes strided data from an offset
- **WRITE bsize bstart stride nelems** - Writes strided data from a byte offset

## Progress

I really wanted to wait to release numstore when it was feature complete, but I also want to gain interest before its full release. Numstore isn't "ready for production" yet, but it's close. The few areas I need to fix up before it works are:

1. I'm ironing out the database checkpoint strategy to avoid an infinite write-ahead log
2. I'm fixing up a few bugs coming mostly from my first attempt at making it concurrent
3. I'm writing tons and tons of tests

Of course I have a few things I'd like to do like:

1. Speed up my WAL - without a WAL I get speeds of ~640 MB/s. With it, it drops to 0.8 MB/s. I have many ideas on how to speed up the WAL and I'd like to make the WAL optional.
2. Add concurrency support. I designed the logical lock table extensibly, and think it would be cool to lock ranges of variables rather than each variable itself. That idea hasn't been implemented yet, but a basic hierarchical lock mechanism does exist. This needs lots of work though and concurrency tests are hard to write.
3. Lots more tests in different language bindings. I'd like to write a test harness in Python because the C test harnesses I wrote were bug-prone and unwieldy.

But those will be released in v0.2.0.

## What's Next

I'm a solo developer working on this only a few hours a week in my free time and I have very little time—so it's slow. Still, I expect v0.1.0 to be released February 1st. It won't include concurrency support—just strict serial transactions with a global database lock—but all my algorithms are designed and the code works barring bugs.

---

*Numstore is open source and available on GitLab at gitlab.com/lincketheo/numstore. More details coming soon.*
