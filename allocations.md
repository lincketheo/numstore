# Allocations

# 1 Transaction 




## Where am I making allocations:
- Memory Pager
    - Allocates 2 big blocks of memory for both pages 
      and buffer for meta information on pages
      Lifetime: Server lifetime

- Scanner 
    - Has 1 dynamic string to read in variable length strings 
      from chars
      Lifetime: Lifetime of a transaction

- Parser 
    - Allocates a stack for a LL(1) parser to put non terminals on 
      top of
      Lifetime: Lifetime of a transaction

- Errors
    - Allocates strings for cause and evidence chains to be later 
      processed by the connection to feed to the output socket
      Lifetime: Short lived (created by program, consumed by connectors)

- Rptree (like a b+tree)
    - Has a internal buffer sized 1 page to be able to do useful 
      operations on pages
    - Inner node 
        - For tree resizing, there's an "internal node representation" 
          that feeds in keys and pages to be sent up the chain of 
          inner node pages and consumed from 
          Lifetime: Short lived
    - A dynamically allocated stack for inner node pages to be pushed 
      onto for tree depth
      Lifetime: Lifetime of one rptree

- VMem hash map 
    - Allocating strings while parsing internal nodes , an upfront hash map 
    - Allocating types for return values

- Server 
    - Allocates a pool of "connectors" (active connection contexts)
      Lifetime: Server lifetime
    

## The things I'm allocating:
- Error strings 
- Token Strings 
- Type requires allocations 
- Memory page pool 
- Connection pool 
- In memory tree node for tree reconstructions
- Tree stack for inner pages 
