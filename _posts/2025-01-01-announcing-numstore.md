# Announcing Numstore: A Database for the Machine Learning Era

I've been working on numstore in my free time for the past year, and I think it's time to talk about why it exists.

## The Problem

Consider these use cases:
- I want to access the i, j, k-th pixel of a list of images sized 256x256
- I want to pull slices of time-series sensor data across multiple dimensions
- I want to efficiently store and query multi-dimensional numerical arrays

A relational database wouldn't handle this very well. Sure, there are binary blobs, but those aren't treated as first-class citizens. You end up serializing/deserializing entire objects just to access a single element. It's clunky, slow, and fundamentally not designed for how machine learning workloads actually operate.

## Numpy and Data Locality

Think about how Python stores lists as a series of objects (PyObject's in C). That's fine for general-purpose programming, but terrible for numerical computation. Numpy was written because it just makes sense to store contiguous data in binary-packed streams.

**Numstore is to Postgres as Numpy is to Python arrays.** It's a step closer to the way machines actually process tightly-packed data.

Right now, without a WAL, numstore achieves back-of-the-napkin **640+ MB/s sustained write speeds** using what I'm calling R+Tree data structures and ARIES WAL recovery.

## How the R+Tree Works

A R+Tree is a Rope data structure but instead of a binary tree, it's a B+tree variant. The key insight: rather than storing keys to data elements, **the keys are counts of elements in each subtree**.

Here's why that matters:

In a traditional B+tree, if you want to find the 1000th element, you need to know its key value. In a R+Tree, each internal node stores how many elements are in its left and right subtrees. To find the 1000th element:

1. Start at root, check left subtree count
2. If left count ≥ 1000, go left
3. If left count < 1000, go right and look for element (1000 - left_count)
4. Recurse until you hit a leaf

This gives you O(log n) access to the n-th element without needing to maintain sorted keys. For insertions and deletions, you just update the counts as you rebalance—which you're doing anyway in a B+tree.

The real power shows up in range operations. Want elements 1000 through 2000? The tree can efficiently:
- Locate element 1000 using count-based navigation
- Stream elements 1000-2000 sequentially from the leaf nodes
- Handle strided access (every k-th element) by skipping within and between leaf nodes

Once you access the nth element, writes and reads are (theoretically) as fast as writing / reading to a file.

Combined with the fact that leaf nodes store contiguous byte arrays, you get cache-friendly sequential access for range queries while maintaining logarithmic random access.

## How It Works

Numstore operates using the following operation codes:

**CREATE** - Initialize a new data store

**DELETE** - Remove a data store

**INSERT offset** - Inserts data at `[offset]`

**READ start:stride:end** - Reads strided data from start to end with byte stride `[stride]`

**REMOVE start:stride:end** - Removes data from start to end with byte stride `[stride]`

**WRITE start:stride:end** - Writes data from start to end with byte stride `[stride]`

The strided access pattern is what makes this particularly useful for multi-dimensional arrays—you can efficiently pull slices without reading entire blocks.

## What's Next

I'm a solo developer working on this 2-4 hours a week and I have very little time - so its slow. Still I suspect v0.1.0 to be released feb 1st. It won't include concurrency support - just strict serial transactions with a global database lock, but all my algorithms are designed.

If you're interested in staying in the loop, send me an email at lincketheo@gmail.com

---

*Numstore is open source and available on GitLab at gitlab.com/lincketheo/numstore. More details coming soon.*
