# OStore File Format

The OStore File Format is based on a series of blocks. Each block is of the same size, so block address calculation is easy. Data is organized into objects. Where an object is a set of bytes. If these objects are larger than a single block, then they are stored in a sequence of blocks ("block sequence"). This sequence of blocks forms a doubly linked list of blocks.

Because each block is of the same size the file can be considered an on disk array of blocks. Therefore blocks can be addressed by an array index value, rather than an offset value.

## File Header

- File Type Identifier : 32 bits - "OSTR"

- OStore Version Number : unit32_t : 1

- Block Size : uint32_t : 128

- Number of Blocks in File

  

## Block Structure

- Block Start Identifier : 32 bits - "BLCK"
- Block File Index : uint32_t : 0.. n
- Object Identification Number : uint32_t : 0..n
- Object Block Sequence Number : uint32_t
- Next Object Block Sequence Number : uint32_t
- Last Object Block Sequence Number : uint32_t
- Data payload : uint8_t [... block size... ]

## Block Sequence Handling

An initial Block Sequence (Block 0) stores a table of objects. Objects are represented as a series of blocks, a "block sequence" if you will within the file. Each entry stores the initial header block, the tail block and the assigned ID of the block sequence.

The Object Handling data stores the number of blocks, then an entry per block:

- Number of Entries : uint32_t
- Then a sequence of::

  - ID : uint32_t
  - Head Block : uint32_t
  - Tail Block : uint32_t

There are a set of reserved ID values, these are used to store housekeeping information, they are as follows:

| ID   | Purpose                                              |
| ---- | ---------------------------------------------------- |
| 0    | Table of Objects                                     |
| 1    | Sequence of empty blocks which can be reused (Trash) |
|      |                                                      |



## Derived Internal Block Structures

### File Header

### Block Header



## Internal API

- object header
  - dirty flag : bool
  - ID
  - head block
  - tail block
  - total number of blocks in object

- readobjectheader(object header id)
- readobject(object header, offset, length)
- writeobject(object header, offset, length)
- writeobjectheader(object header)



### Ro