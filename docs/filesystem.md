# Virtual File System & Storage Architecture

## 1. Virtual File System (VFS) Abstraction

The VFS decouples file operations from physical storage mediums (in-memory ramfs, devfs, FAT32 disk).

```
+---------------------------------------------------------+
|                  VFS Layer (VNode / Inode)              |
|        open(), close(), read(), write(), readdir()      |
+--------------------+--------------------+---------------+
|                    |                    |
v                    v                    v
+--------------+     +--------------+     +---------------+
|  RamFS       |     |  DevFS       |     |  FAT32 Driver |
|  (In-Memory) |     |  (/dev/null, |     |  (Disk Image) |
|              |     |   /dev/zero, |     |               |
|              |     |   /dev/serial)     |               |
+--------------+     +--------------+     +---------------+
                                                  |
                                                  v
                                          +---------------+
                                          | Block Device  |
                                          | (VirtIO-Blk / |
                                          |  ATA PIO)     |
                                          +---------------+
```

---

## 2. Inode & File Descriptor Architecture

### VNode Interface
```cpp
struct VNode {
    enum class Type { File, Directory, Device };
    Type type;
    size_t size;

    virtual ssize_t read(size_t offset, void* buf, size_t count) = 0;
    virtual ssize_t write(size_t offset, const void* buf, size_t count) = 0;
    virtual int readdir(size_t index, DirectoryEntry* entry) = 0;
    virtual VNode* lookup(const char* name) = 0;
};
```

### File Descriptors
Each process contains a `FileDescriptorTable` mapping integer descriptors (`0 = stdin`, `1 = stdout`, `2 = stderr`) to open `FileDescription` objects containing:
- Pointer to target `VNode`.
- Current read/write seek offset.
- Open flags (`O_RDONLY`, `O_WRONLY`, `O_CREAT`, `O_APPEND`).

---

## 3. Storage Drivers & Persistence

1. **Milestone 11**: In-memory VFS (RamFS) allows creating files, directories, and reading data without physical device dependencies.
2. **Milestone 12**: Block device abstraction and VirtIO-Block / ATA controller integration reading raw disk images.
