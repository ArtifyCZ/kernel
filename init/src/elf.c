#include "init/include/elf.h"

#include "libs/libsyscall/syscalls.h"
#include "string.h"
#include <stddef.h>

static uint32_t p_flags_to_vmm(Elf64_Word p_flags) {
    uint32_t flags = SYS_PROT_READ;
    if (p_flags & ELF_PF_W) {
        flags |= SYS_PROT_WRITE;
    } else if (p_flags & ELF_PF_X) {
        flags |= SYS_PROT_EXEC;
    }

    return flags;
}

int elf_load(void *data, uintptr_t *out_v_addr_entrypoint, uint64_t *out_proc_handle) {
    const Elf64_Ehdr *header = (Elf64_Ehdr *) data;
    if (!elf_arch_is_supported(header->e_machine)) {
        return -1;
    }

    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        // If ELF is not little endian
        return -1; // Big endian not supported
    }

    uint64_t proc_handle;
    if (sys_proc_create(0, &proc_handle)) {
        return -1; // Could not create a process container
    }

    Elf64_Phdr *phdrs = (Elf64_Phdr *) ((uintptr_t) data + header->e_phoff);

    for (uint16_t i = 0; i < header->e_phnum; i++) {
        Elf64_Phdr *phdr = &phdrs[i];
        if (phdr->p_type == PT_LOAD) {
            uint32_t flags = p_flags_to_vmm(phdr->p_flags);
            uintptr_t virt_start = phdr->p_vaddr;

            uintptr_t page_start = virt_start & ~0xFFFULL;

            void *memory_chunk;
            if (sys_proc_mmap(
                proc_handle,
                page_start,
                phdr->p_memsz,
                SYS_PROT_READ | SYS_PROT_WRITE,
                SYS_MMAP_FL_MIRROR,
                (uintptr_t *) &memory_chunk
            )) {
                return -1; // Failed to map memory into a children process
            }
            memset(memory_chunk, 0, phdr->p_memsz);

            uintptr_t offset_within_page = virt_start & 0xFFF;
            void *copy_target = (void*)((uintptr_t)memory_chunk + offset_within_page);
            memcpy(copy_target, (uint8_t*)data + phdr->p_offset, phdr->p_filesz);

            if (sys_proc_mprot(proc_handle, page_start, phdr->p_memsz, flags)) {
                return -1; // Failed to set protection flags of the prepared memory
            }
        }
    }

    *out_v_addr_entrypoint = header->e_entry;
    *out_proc_handle = proc_handle;
    return 0;
}

#define EM_AARCH64 0xB7
#define EM_X86_64 0x3E

bool elf_arch_is_supported(Elf64_Half arch) {
#if defined (__x86_64__)
    return arch == EM_X86_64;
#elif defined (__aarch64__)
    return arch == EM_AARCH64;
#endif
    return false;
}
