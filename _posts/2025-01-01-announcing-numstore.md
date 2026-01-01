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

Right now, without a WAL, numstore achieves back-of-the-napkin **640+ MB/s sustained write speeds** using what I'm calling R+Tree data structures and ARIES WAL recovery.

## How the R+Tree Works

A R+Tree is a Rope data structure but instead of a binary tree, it's a B+tree variant. The key insight: rather than storing keys to data elements, **the keys are counts of elements in each subtree**. Text editors use ropes all the time for efficient data insertion in the middle of documents, this is just an extension into a the database world. 

This gives you O(log n) access to the n-th element without needing to maintain sorted keys. For insertions and deletions, you just update the counts as you rebalance—which you're doing anyway in a B+tree.

I'll say that again, **Numstore is like a file system where instead of copy write copy (O(n) mutations in the middle), it allows for O(log N) internal mutations. This is fundamentally a new shift in thinking for files. 

Want elements 1000 through 2000? The tree can efficiently:
- Locate element 1000 using count-based navigation
- Stream elements 1000-2000 sequentially from the leaf nodes
- Handle strided access (every k-th element) by skipping within and between leaf nodes

Once you access the nth element, writes and reads are (theoretically) as fast as writing / reading to a file.

Combined with the fact that leaf nodes store contiguous byte arrays, you get cache-friendly sequential access for range queries while maintaining logarithmic random access.

## How It Works

Numstore breaks down its operation into the following operation codes:
(Note convention is numbers that refer to a byte value are prefixed by `b`. Numbers that refer to units of values 

Housekeeping Operations:
**CREATE** - Initialize a new "variable" (e.g. a file)
**DELETE** - Remove a variable

Data Manipulation Operations:
**INSERT boffset** - Inserts bytes at byte offset `[boffset]`
**READ bsize bstart stride nelems** - Reads strided data from byte offset `[bstart]` end with elemenet stride `[stride]` where each element is `[bsize]` bytes.
**REMOVE bsize bstart stride nelems** - Removes strided data from an offset
**WRITE start:stride:end** - Writes strided data from a byte offset

## What's Next

I'm a solo developer working on this 2-4 hours a week and I have very little time - so its slow. Still I suspect v0.1.0 to be released feb 1st. It won't include concurrency support - just strict serial transactions with a global database lock, but all my algorithms are designed and the code works barring bugs.

If you're interested in staying in the loop, send me an email at lincketheo@gmail.com

---

*Numstore is open source and available on GitLab at gitlab.com/lincketheo/numstore. More details coming soon.*
