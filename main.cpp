#include <string>
#include <thread>
#include <chrono>
#include <ratio>
#include <getopt.h>
#include "elfloader.h"
#include "rv_memory.h"
#include "rv_cpu.h"
#include "rv_global.h"

void usage(const char *path)
{
    fprintf(stderr, "Usage: %s [-m memory_size] <target_executable> [arg 1] ... [argn n]\n", path);
}

int main(int argc, char *argv[])
{
    int opt = -1;
    unsigned long int convres = (unsigned long int)-1;
    rv_uint memory_size = 128_MiB;
    int ret_val = EXIT_SUCCESS;

    while((opt = getopt(argc, argv, "m:")) != -1) {
        switch (opt) {
        case 'm':
            convres = strtoul(optarg, nullptr, 10);
            if (convres > 512_MiB || errno == ERANGE) {
                fprintf(stderr, "[e] error: > 512 MiB is too much for DooM, sorry...\n");
                exit(EXIT_FAILURE);
            }
            memory_size = (rv_uint)convres;
            break;

        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "missing executable to emulate!\n");
        exit(EXIT_FAILURE);
    }

    elf_loader loader{std::string(argv[optind])};
    try {
        loader.load();
        rv_memory memory(memory_size);

        // first 64k are mapped RWX
        memory.protect_region(0, 0x10000, RV_MEMORY_RWX);

        rv_uint last_vaddr = 0;
        rv_uint last_vsize = 0;

        // this is wrong, there's no guarantee that the last segment comes last in memory
        // but for now it's ok...for newlib layout at least
        for (const auto& seg : loader.segments()) {
            last_vaddr = seg.virtual_address();
            last_vsize = seg.memory_size();
            memory.set_region(last_vaddr, loader.pointer_to<uint8_t>(seg), seg.file_size());
            memory.protect_region(last_vaddr, last_vsize, seg.protection());
        }

        // TODO: align to segment->alignment
        if (last_vsize % 0x1000 != 0)
            last_vsize = last_vsize + (0x1000 - (last_vsize%0x1000));

        rv_uint end_of_data = last_vaddr + last_vsize;

        // setup one guard page before the stack
        end_of_data += 0x1000;

        // setup memory protection for stack (rw)
        memory.protect_region(end_of_data, memory.stack_size(), RV_MEMORY_RW);

        // the stack starts right after the first guard page
        memory.set_stack(end_of_data + memory.stack_size());
        end_of_data += memory.stack_size();

        memory.prepare_environment(argc, argv, optind);

        // setup a second guard page after the stack
        end_of_data += 0x1000;

        // finally set the program break and map all the remaining ram
        memory.set_brk(end_of_data);
        memory.protect_region(end_of_data, memory.ram_end() - end_of_data, RV_MEMORY_RW);

        rv_cpu cpu(memory);
        cpu.reset(loader.entry_point());
#define PROFILEME
#ifdef PROFILEME
        // start profiling thread
        std::thread([&cpu]() {
            using namespace std::chrono_literals;
            uint64_t prev_cycle = cpu.cycle_count();

            for (;;) {
                std::this_thread::sleep_for(1s);
                uint64_t cur_cycle = cpu.cycle_count();
                uint64_t delta = cur_cycle - prev_cycle;
                prev_cycle = cur_cycle;
                fprintf(stderr, "[i] MIPS: %.2f\n", double(delta)/double(1e6));
            }
        }).detach();
#endif

        for (;;) {
            cpu.run(500000);
            if (cpu.emulation_exit())
                break;
        }
        fprintf(stderr, "[i] target exited with: %d\n", cpu.emulation_exit_status());
    }
    catch(const std::runtime_error& ex) {
        fprintf(stderr, "[e] error: %s", ex.what());
        ret_val = EXIT_FAILURE;

    }
    return ret_val;
}