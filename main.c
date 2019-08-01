#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define die() \
  do {                                  \
    fprintf(stderr, "%s:%d:%s\n",       \
        __FILE__, __LINE__, __func__);  \
    exit(1);                            \
  } while(0)

struct kvm {
  int fd;
  int vm;
  int vcpu;

  struct kvm_userspace_memory_region memory_region;
  struct kvm_run* run;

  struct kvm_sregs sregs;
  struct kvm_regs regs;
};

struct ivt_entry {
  uint16_t offset;
  uint16_t segment;
};

struct kvm kvm;

struct ivt_entry ivt[256];

void kvm_init(void) {
  kvm.fd = open("/dev/kvm", O_RDWR);
  if(kvm.fd < 0) die();

  // Assert correct `KVM_API_VERSION`.
  const int api_version = ioctl(kvm.fd, KVM_GET_API_VERSION, 0);
  if(api_version < 0 || api_version != KVM_API_VERSION) die();

  kvm.vm = ioctl(kvm.fd, KVM_CREATE_VM, 0);
  if(kvm.vm < 0) die();

  kvm.vcpu = ioctl(kvm.vm, KVM_CREATE_VCPU, 0);
  if(kvm.vcpu < 0) die();

  const int vcpu_mmap_size = ioctl(kvm.fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if(vcpu_mmap_size <= 0) die();

  kvm.run = mmap(NULL,
      vcpu_mmap_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      kvm.vcpu,
      0);
  if(kvm.run == MAP_FAILED) die();

  const size_t memory_size = 0x100000; // 1MB.
  void* const memory = mmap(NULL,
      memory_size,
      PROT_READ | PROT_WRITE,
      // Zeroed mapping without swap.
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
      // Ignored because of `MAP_ANONYMOUS`.
      -1,
      // Ignored because of `MAP_ANONYMOUS`.
      0);
  if(memory == MAP_FAILED) die();

  kvm.memory_region.memory_size = memory_size;
  kvm.memory_region.userspace_addr = (uintptr_t)memory;
  if(ioctl(kvm.vm, KVM_SET_USER_MEMORY_REGION, &kvm.memory_region) < 0)
    die();
}

void kvm_handle_io(void) {
  if(kvm.run->io.direction == KVM_EXIT_IO_OUT
      && kvm.run->io.size == 1
      && kvm.run->io.port == 0x3f8
      && kvm.run->io.count == 1) {
    printf("out: %c\n", *((char*)kvm.run + kvm.run->io.data_offset));
  }
}

void kvm_print_regs(void) {
  if(ioctl(kvm.vcpu, KVM_GET_REGS, &kvm.regs) < 0) die();

  printf("\n    eax 0x%08llx    ecx 0x%08llx    edx 0x%08llx    ebx 0x%08llx\n"
      "    esp 0x%08llx    ebp 0x%08llx    esi 0x%08llx    edi 0x%08llx\n"
      "    eip 0x%08llx eflags 0x%08llx\n",
      kvm.regs.rax, kvm.regs.rcx, kvm.regs.rdx, kvm.regs.rbx,
      kvm.regs.rsp, kvm.regs.rbp, kvm.regs.rsi, kvm.regs.rdi,
      kvm.regs.rip, kvm.regs.rflags);
}

void kvm_run(void) {
  while(1) {
    if(ioctl(kvm.vcpu, KVM_RUN, 0) < 0) {
      perror("run");
      die();
    }

    switch(kvm.run->exit_reason) {
    case KVM_EXIT_IO:     kvm_handle_io();  break;
    case KVM_EXIT_DEBUG:  kvm_print_regs(); break;
    case KVM_EXIT_HLT:    return;
    default:
      printf("exit_reason: %d\n", kvm.run->exit_reason);
      die();
    }
  }
}

void load_bios(const char* filename) {
  const int fd = open(filename, O_RDONLY);
  if(fd < 0) die();

  uint8_t* memory = (uint8_t*)kvm.memory_region.userspace_addr;

  for(size_t i = 0; i < 256; ++i) {
    ivt[i].offset = 0x400;
    ivt[i].segment = 0;
  }

  memcpy(memory, ivt, sizeof(ivt));

  memory[0x400] = 0xcf; // iret

  memory += 0xf0000;

  ssize_t count;
  while((count = read(fd, memory, 512)) > 0)
    memory += count;
}

void load_binary(const char* filename) {
  const int fd = open(filename, O_RDONLY);
  if(fd < 0) die();

  uint8_t* buf = (uint8_t*)kvm.memory_region.userspace_addr + 0x7c00;

  ssize_t count;
  while((count = read(fd, buf, 512)) > 0)
    buf += count;
}

int main(void) {
  kvm_init();

  load_bios("bios/bios.bin");
  load_binary("tests/out.bin");

  if(ioctl(kvm.vcpu, KVM_GET_SREGS, &kvm.sregs) < 0) die();
  kvm.sregs.cs.selector = 0xf000;
  kvm.sregs.cs.base = 0xf000;
  if(ioctl(kvm.vcpu, KVM_SET_SREGS, &kvm.sregs) < 0) die();

  kvm.regs.rip = 0;
  if(ioctl(kvm.vcpu, KVM_SET_REGS, &kvm.regs) < 0) die();

  kvm_print_regs();

  kvm_run();

  kvm_print_regs();

  uint8_t* memory = (uint8_t*)kvm.memory_region.userspace_addr;

  for(size_t i = 0; i < 128; ++i) {
    if(i % 16 == 0) putchar('\n');
    printf("%02x ", memory[i]);
  }

  return 0;
}
