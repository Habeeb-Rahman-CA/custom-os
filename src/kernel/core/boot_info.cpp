#include <kernel/core/boot_info.h>
#include <kernel/core/logger.h>
#include <kernel/core/panic.h>
#include <kernel/drivers/serial.h>

namespace kernel::boot {

namespace {

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_req = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_req = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

BootInfo g_boot_info{};

[[maybe_unused]] const char* memmap_type_str(u64 type) {
    switch (type) {
        case LIMINE_MEMMAP_USABLE:
            return "Usable RAM";
        case LIMINE_MEMMAP_RESERVED:
            return "Reserved";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return "ACPI Reclaimable";
        case LIMINE_MEMMAP_ACPI_NVS:
            return "ACPI NVS";
        case LIMINE_MEMMAP_BAD_MEMORY:
            return "Bad Memory";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return "Bootloader Reclaimable";
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            return "Kernel and Modules";
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return "Framebuffer";
        default:
            return "Unknown Type";
    }
}

} // namespace

BootInfo& get_info() {
    return g_boot_info;
}

struct limine_memmap_response* get_raw_memmap() {
    return memmap_req.response;
}

void init() {
    // Verify Limine base revision support
    if (!LIMINE_BASE_REVISION_SUPPORTED) {
        panic("Limine base revision 3 is not supported by the bootloader!");
    }

    // Process Bootloader Info
    if (bootloader_info_req.response) {
        if (bootloader_info_req.response->name) {
            g_boot_info.bootloader_name = bootloader_info_req.response->name;
        }
        if (bootloader_info_req.response->version) {
            g_boot_info.bootloader_version = bootloader_info_req.response->version;
        }
    }

    // Process HHDM
    if (hhdm_req.response) {
        g_boot_info.hhdm_offset = hhdm_req.response->offset;
    } else {
        panic("Limine HHDM response missing!");
    }

    // Process Memory Map
    if (memmap_req.response && memmap_req.response->entries) {
        g_boot_info.mem_stats.entry_count = memmap_req.response->entry_count;
        for (u64 i = 0; i < memmap_req.response->entry_count; ++i) {
            auto* entry = memmap_req.response->entries[i];
            g_boot_info.mem_stats.total_bytes += entry->length;

            if (entry->type == LIMINE_MEMMAP_USABLE) {
                g_boot_info.mem_stats.usable_bytes += entry->length;
            } else if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
                       entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
                g_boot_info.mem_stats.reclaimable_bytes += entry->length;
            } else {
                g_boot_info.mem_stats.reserved_bytes += entry->length;
            }
        }
    } else {
        panic("Limine memory map response missing!");
    }

    // Process Framebuffer (optional in Milestone 1)
    if (framebuffer_req.response && framebuffer_req.response->framebuffer_count > 0) {
        auto* fb = framebuffer_req.response->framebuffers[0];
        g_boot_info.fb_info.address = reinterpret_cast<u64>(fb->address);
        g_boot_info.fb_info.width = fb->width;
        g_boot_info.fb_info.height = fb->height;
        g_boot_info.fb_info.pitch = fb->pitch;
        g_boot_info.fb_info.bpp = fb->bpp;
        g_boot_info.fb_info.available = true;
    }

    g_boot_info.valid = true;
}

void dump_to_log() {
    log::info("--- Bootloader & Hardware Summary ---");
    log::write_kv(log::Level::Info, "Bootloader Name", g_boot_info.bootloader_name);
    log::write_kv(log::Level::Info, "Bootloader Version", g_boot_info.bootloader_version);
    log::write_kv_u64_hex(log::Level::Info, "HHDM Direct Map Base", g_boot_info.hhdm_offset);

    log::write_kv_u64_dec(log::Level::Info, "Memory Map Entries", g_boot_info.mem_stats.entry_count);
    log::write_kv_u64_dec(log::Level::Info, "Total Physical RAM (MiB)", g_boot_info.mem_stats.total_bytes / (1024 * 1024));
    log::write_kv_u64_dec(log::Level::Info, "Usable Physical RAM (MiB)", g_boot_info.mem_stats.usable_bytes / (1024 * 1024));
    log::write_kv_u64_dec(log::Level::Info, "Reclaimable RAM (MiB)", g_boot_info.mem_stats.reclaimable_bytes / (1024 * 1024));
    log::write_kv_u64_dec(log::Level::Info, "Reserved RAM (MiB)", g_boot_info.mem_stats.reserved_bytes / (1024 * 1024));

    if (g_boot_info.fb_info.available) {
        log::write_kv_u64_dec(log::Level::Info, "Framebuffer Width", g_boot_info.fb_info.width);
        log::write_kv_u64_dec(log::Level::Info, "Framebuffer Height", g_boot_info.fb_info.height);
        log::write_kv_u64_dec(log::Level::Info, "Framebuffer BPP", g_boot_info.fb_info.bpp);
    } else {
        log::info("Framebuffer: None (serial console only)");
    }
    log::info("-------------------------------------");
}

} // namespace kernel::boot

