# A Description of Each Library 

- apps: Applications of numstore - not applications, but just usecases that are more complex than just usecases 
    - nsdblite: A simple implementation of numstore and it's type system
    - nsfslite: A simple file system in a file - no type system, just variable names and data

- cursors: Cursors are logical crawlers of the database - they have special semantics for how they create and manipulate nodes
    - nsrptcursor: A R+tree cursor - starts with a root node and does insert, write, read and remove operations on a tree 
    - nsvcursor: A variable name cursor - stores variable names in pages and their associated types

- nscompiler: A compiler for numstore that generates queries and types using nstypes 

- nscore: Common code - numstore's "standard library"

- nspager: All the pager logic in numstore

- nstypes: Numstore's type system

- nsusecase: One off simple usecases that can be performed. For example, printing all the wal entries, or iterating through data pages etc.

# Dependency Graph 

nsdblite -> nscore, nspager 
nsfslite -> nscore, nspager, nsrptcursor, nsvcursor
nsrptcursor -> nspager 
nsvcursor -> nspager, nstypes
nscompiler -> nstypes
nscore -> 
nspager -> nscore
nstypes -> nscore
nsusecase -> nspager
