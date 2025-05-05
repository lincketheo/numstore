<template>
  <div class="about">
    <h1>About Numstore</h1>

    <h2>Motivation</h2>
    <p>
      In 2025, machine learning continues to dominate our feeds. Yet the underlying infrastructure for managing raw,
      structured data remains fragmented. We have plain text formats like CSV and JSON for labels and annotations,
      and binary formats like JPEG and PNG for media — but there's no unified system for storing structured,
      contiguous numerical data.
    </p>
    <p>
      Numstore was created to fill that gap. Its core goal is to support "machine learning-shaped" data — that is,
      dense, typed, structured arrays — and make them easy to store, access, and share. Whether it's feature matrices,
      tensors, or time series data, Numstore provides a clean abstraction for handling it at scale.
    </p>
    <p>
      The original motivation came from digital signal processing work. In that domain, datasets are often just flat
      sequences of binary data stored in files. Feeding that into a database was awkward at best, and impossible at
      worst. Numstore bridges that gap, making contiguous data a first-class citizen in a database context.
    </p>

    <h2>What is Numstore?</h2>
    <p>
      Numstore is an ACID-compliant database designed specifically for array-like data. It allows users to store
      typed variables in a compact format, append new data efficiently, and relate variables to each other through
      named keys. It is particularly well-suited for workloads where numerical data is central — such as AI pipelines,
      sensor streams, and scientific data logging.
    </p>
    <p>
      Datasets in Numstore are stored as one or more variables — each a contiguous array of a specific data type
      (e.g., <code>u64</code>, <code>float32</code>, structs, or unions). These variables can be grouped under
      named keys, and the database handles allocation, indexing, and querying internally.
    </p>

    <h2>What Numstore is <em>Not</em></h2>
    <p>
      Numstore is not a general-purpose SQL database. It does not support joins, foreign keys, or arbitrarily typed
      rows. It is not a replacement for PostgreSQL or MongoDB.
    </p>
    <p>
      Numstore is not meant for human-facing forms or text-heavy applications. It is built for machines and structured
      numerical data, not unstructured blobs or documents.
    </p>

    <h2>Guiding Principles</h2>
    <ul>
      <li>Treat contiguous, structured data as a first-class citizen</li>
      <li>Design around append-heavy, read-efficient workloads</li>
      <li>Make types explicit and memory layouts deterministic</li>
      <li>Support low-latency protocols for ingestion and query</li>
      <li>Minimize abstraction overhead and maximize composability</li>
    </ul>

    <h2>Use Cases</h2>
    <ul>
      <li>Machine learning datasets (e.g., tensors, arrays, sequences)</li>
      <li>Sensor data collection and replay</li>
      <li>Scientific experiment logging</li>
      <li>Streaming numerical event storage</li>
      <li>Custom structured binary formats</li>
    </ul>

    <h2>How to Use Numstore</h2>
    <p>
      Users can define a dataset schema by naming variables and assigning them types. Once defined, the database handles
      all low-level details: memory management, paging, indexing, and persistence. Data can be appended incrementally,
      and queried by index or range.
    </p>
    <p>
      Numstore supports read/update/delete of individual elements, as well as full-scan access for analytics or export.
      It is extensible and can be embedded in larger applications or used as a standalone server over a TCP protocol.
    </p>

    <h2>Future Work</h2>
    <ul>
      <li>Streaming replication for distributed environments</li>
      <li>Compression and delta encoding</li>
      <li>WebAssembly bindings for in-browser use</li>
      <li>Data marketplace integrations for buy/sell workflows</li>
      <li>Query language and CLI tooling</li>
    </ul>

    <h2>Get Involved</h2>
    <p>
      Numstore is an evolving project. If you're interested in contributing, using it in your stack, or helping refine
      the architecture, reach out or follow the project on GitHub.
    </p>
  </div>
</template>

<script setup>
// No logic needed at the moment
</script>

<style scoped>
.about {
  font-family: sans-serif;
  max-width: 800px;
  margin: 2rem auto;
  padding: 0 1rem;
}

h1 {
  font-size: 2rem;
  margin-bottom: 1rem;
}

h2 {
  font-size: 1.5rem;
  margin-top: 2rem;
  margin-bottom: 0.5rem;
}

p {
  margin-bottom: 1rem;
  line-height: 1.5;
}

ul, ol {
  margin-left: 1.5rem;
  margin-bottom: 1rem;
}

code {
  font-family: monospace;
  background: #f0f0f0;
  padding: 0.1em 0.3em;
  border-radius: 3px;
}
</style>
