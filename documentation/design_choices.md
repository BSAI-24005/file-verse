# Design Choices

- Users: loaded into memory on fs_init, stored on-disk in fixed-size UserInfo table. For lookup we use linear scan (small max_users=50), but recommend hash table for scaling.
- Directory tree: metadata index (fixed-size entries) per file/dir. In-memory tree can be built lazily.
- Free space: simple bitmap stored after user table. Each block uses 1 byte in this student implementation; can be improved to bit-packed.
- File blocks: linked-list blocks; first 4 bytes = next block index; remainder is data.
- Encoding: byte-substitution mapping is reserved in header's reserved area for phase 2.