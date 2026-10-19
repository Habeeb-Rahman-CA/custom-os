#include <kernel/filesystem/vfs.h>
#include <kernel/memory/heap.h>
#include <kernel/drivers/serial.h>
#include <kernel/core/logger.h>

extern "C" const kernel::u8 g_hello_elf_start[];
extern "C" const kernel::u8 g_hello_elf_end[];
extern "C" const kernel::u8 g_fault_elf_start[];
extern "C" const kernel::u8 g_fault_elf_end[];

namespace kernel::fs {

namespace {

VFSNode* g_root = nullptr;

struct FileHandle {
    VFSNode* node{nullptr};
    usize offset{0};
    bool in_use{false};
};

constexpr usize MAX_FDS = 64;
FileHandle g_fd_table[MAX_FDS]{};

int string_compare(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

usize string_length(const char* s) {
    usize len = 0;
    while (s[len]) len++;
    return len;
}

void string_copy(char* dst, const char* src, usize max_len) {
    usize i = 0;
    while (src[i] && i + 1 < max_len) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void memory_copy(void* dst, const void* src, usize n) {
    auto* d = reinterpret_cast<u8*>(dst);
    const auto* s = reinterpret_cast<const u8*>(src);
    for (usize i = 0; i < n; ++i) d[i] = s[i];
}

} // namespace

ssize_t VFSNode::read(usize off, void* buf, usize count) {
    if (off >= size) return 0; // EOF
    usize available = size - off;
    usize bytes_to_read = (count < available) ? count : available;
    memory_copy(buf, data + off, bytes_to_read);
    return static_cast<ssize_t>(bytes_to_read);
}

ssize_t VFSNode::write(usize off, const void* buf, usize count) {
    if (off + count > capacity) {
        usize new_cap = (off + count + 1024) & ~511ULL;
        data = reinterpret_cast<u8*>(memory::krealloc(data, new_cap));
        capacity = new_cap;
    }
    memory_copy(data + off, buf, count);
    if (off + count > size) {
        size = off + count;
    }
    return static_cast<ssize_t>(count);
}

VFSNode* VFSNode::find_child(const char* child_name) {
    VFSNode* curr = first_child;
    while (curr) {
        if (string_compare(curr->name, child_name) == 0) {
            return curr;
        }
        curr = curr->next_sibling;
    }
    return nullptr;
}

VFSNode* VFSNode::add_child(const char* child_name, NodeType child_type) {
    auto* child = new VFSNode();
    string_copy(child->name, child_name, sizeof(child->name));
    child->type = child_type;
    child->parent = this;
    child->next_sibling = first_child;
    first_child = child;
    return child;
}

void VFS::init() {
    g_root = new VFSNode();
    string_copy(g_root->name, "/", sizeof(g_root->name));
    g_root->type = NodeType::Directory;

    // Create standard directory tree
    VFSNode* bin_dir = g_root->add_child("bin", NodeType::Directory);
    g_root->add_child("dev", NodeType::Directory);
    g_root->add_child("etc", NodeType::Directory);

    // Create /hello.txt
    VFSNode* hello_txt = g_root->add_child("hello.txt", NodeType::File);
    const char msg[] = "Welcome to CustomOS!\nA freestanding educational x86-64 operating system.\n";
    hello_txt->write(0, msg, sizeof(msg) - 1);

    // Create /bin/hello.elf with embedded user executable
    usize elf_size = reinterpret_cast<u64>(g_hello_elf_end) - reinterpret_cast<u64>(g_hello_elf_start);
    if (elf_size > 0) {
        VFSNode* hello_elf = bin_dir->add_child("hello.elf", NodeType::File);
        hello_elf->write(0, g_hello_elf_start, elf_size);
    }

    // Create /bin/fault.elf for testing user exception fault isolation
    usize fault_size = reinterpret_cast<u64>(g_fault_elf_end) - reinterpret_cast<u64>(g_fault_elf_start);
    if (fault_size > 0) {
        VFSNode* fault_elf = bin_dir->add_child("fault.elf", NodeType::File);
        fault_elf->write(0, g_fault_elf_start, fault_size);
    }

    log::info("[VFS] Virtual File System initialized with in-memory root, /bin, and /hello.txt.");
}

VFSNode* VFS::get_root() {
    return g_root;
}

VFSNode* VFS::lookup(const char* path) {
    if (!path || !g_root) return nullptr;
    if (string_compare(path, "/") == 0) return g_root;

    VFSNode* curr = g_root;
    const char* p = path;
    if (*p == '/') p++;

    char component[64];
    while (*p) {
        usize i = 0;
        while (*p && *p != '/' && i + 1 < sizeof(component)) {
            component[i++] = *p++;
        }
        component[i] = '\0';
        if (*p == '/') p++;

        curr = curr->find_child(component);
        if (!curr) return nullptr;
    }
    return curr;
}

VFSNode* VFS::create_file(const char* path) {
    if (!path || *path != '/') return nullptr;

    // Separate parent dir and filename
    usize len = string_length(path);
    usize last_slash = 0;
    for (usize i = 0; i < len; ++i) {
        if (path[i] == '/') last_slash = i;
    }

    char parent_path[128];
    if (last_slash == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    } else {
        string_copy(parent_path, path, last_slash + 1);
        parent_path[last_slash] = '\0';
    }

    const char* filename = path + last_slash + 1;

    VFSNode* parent = lookup(parent_path);
    if (!parent || parent->type != NodeType::Directory) return nullptr;

    VFSNode* existing = parent->find_child(filename);
    if (existing) return existing;

    return parent->add_child(filename, NodeType::File);
}

int VFS::open(const char* path, int flags) {
    (void)flags;
    VFSNode* node = lookup(path);
    if (!node) return -1;

    for (usize i = 3; i < MAX_FDS; ++i) {
        if (!g_fd_table[i].in_use) {
            g_fd_table[i].node = node;
            g_fd_table[i].offset = 0;
            g_fd_table[i].in_use = true;
            return static_cast<int>(i);
        }
    }
    return -1;
}

ssize_t VFS::read(int fd, void* buf, usize count) {
    if (fd < 3 || fd >= static_cast<int>(MAX_FDS) || !g_fd_table[fd].in_use) {
        return -1;
    }
    FileHandle& fh = g_fd_table[fd];
    ssize_t bytes_read = fh.node->read(fh.offset, buf, count);
    if (bytes_read > 0) {
        fh.offset += bytes_read;
    }
    return bytes_read;
}

ssize_t VFS::write(int fd, const void* buf, usize count) {
    if (fd < 3 || fd >= static_cast<int>(MAX_FDS) || !g_fd_table[fd].in_use) {
        return -1;
    }
    FileHandle& fh = g_fd_table[fd];
    ssize_t bytes_written = fh.node->write(fh.offset, buf, count);
    if (bytes_written > 0) {
        fh.offset += bytes_written;
    }
    return bytes_written;
}

int VFS::close(int fd) {
    if (fd < 3 || fd >= static_cast<int>(MAX_FDS) || !g_fd_table[fd].in_use) {
        return -1;
    }
    g_fd_table[fd].in_use = false;
    g_fd_table[fd].node = nullptr;
    g_fd_table[fd].offset = 0;
    return 0;
}

void VFS::dump_tree(VFSNode* node, int indent) {
    if (!node) node = g_root;
    if (!node) return;

    for (int i = 0; i < indent; ++i) drivers::serial::write_string("  ");
    if (node->type == NodeType::Directory) {
        drivers::serial::write_string("[DIR]  ");
    } else {
        drivers::serial::write_string("[FILE] ");
    }
    drivers::serial::write_string(node->name);
    if (node->type == NodeType::File) {
        drivers::serial::write_string(" (");
        drivers::serial::write_u64_dec(node->size);
        drivers::serial::write_string(" bytes)");
    }
    drivers::serial::write_char('\n');

    VFSNode* child = node->first_child;
    while (child) {
        dump_tree(child, indent + 1);
        child = child->next_sibling;
    }
}

} // namespace kernel::fs

