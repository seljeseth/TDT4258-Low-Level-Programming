# Assignment 2 - Cache Simulator 

"Write a cache simulator which reads a trace of
memory references and reports 3 statistics (accesses, hits and hit rate). The cache configuration is
determined by parameters that are passed as command line arguments. The command-line parameters
are cache size, cache mapping (DM/FA), and cache organization (Unified/Split). "

Takes in a cache size, mapping type and the organization type. 

An example of the program

input : ```./cache_sim 128 dm uc```
 
 output: 
 
```c
  Cache Organization
  -----------------
  Size: 128 bytes
  Organization: Unified Cache
  Mapping: Direct Mapped
  Size of index: 1
  Size of tag: 25
  Block Offset: 6
  -----------------


  Cache Statistics
  -----------------
  Accesses: 1465707
  Hits:     941760
  Hit Rate: 0.6425
```
