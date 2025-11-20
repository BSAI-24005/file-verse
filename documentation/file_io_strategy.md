# File I/O Strategy

- Use fstream binary read/write.
- OMNIHeader at start (512 bytes) => user table => free map => content blocks.
- Structures are serialized by writing their bytes directly (struct layout fixed).
- On startup fs_init reads header and user table to populate runtime metadata.