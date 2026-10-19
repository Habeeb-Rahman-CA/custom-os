#pragma once

#include <kernel/core/types.h>

namespace kernel::fs {

enum class NodeType {
    File,
    Directory,
    Device
};

struct VFSNode {
    char name[64]{};
    NodeType type{NodeType::File};
    usize size{0};
    usize capacity{0};
    u8* data{nullptr};

    VFSNode* parent{nullptr};
    VFSNode* first_child{nullptr};
    VFSNode* next_sibling{nullptr};

    ssize_t read(usize offset, void* buf, usize count);
    ssize_t write(usize offset, const void* buf, usize count);
    VFSNode* find_child(const char* child_name);
    VFSNode* add_child(const char* child_name, NodeType child_type);
};

class VFS {
public:
    static void init();

    static VFSNode* get_root();
    static VFSNode* lookup(const char* path);
    static VFSNode* create_file(const char* path);

    // File Descriptor table APIs
    static int open(const char* path, int flags = 0);
    static ssize_t read(int fd, void* buf, usize count);
    static ssize_t write(int fd, const void* buf, usize count);
    static int close(int fd);

    static void dump_tree(VFSNode* node = nullptr, int indent = 0);
};

} // namespace kernel::fs
