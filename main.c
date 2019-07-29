#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

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

struct kvm kvm;

void kvm_init(void) {
  kvm.fd = open("/dev/kvm", O_RDWR);
  if(kvm.fd < 0) die();

  // Assert correct `KVM_API_VERSION`.
  const int api_version = ioctl(kvm.fd, KVM_GET_API_VERSION, 0);
  if(api_version < 0) die();
  if(api_version != KVM_API_VERSION) die();

  kvm.vm = ioctl(kvm.fd, KVM_CREATE_VM, 0);
  if(kvm.vm < 0) die();

  kvm.vcpu = ioctl(kvm.vm, KVM_CREATE_VCPU, 0);
  if(kvm.vcpu < 0) die();

  const int vcpu_mmap_size = ioctl(kvm.fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if(vcpu_mmap_size <= 0) die();

  kvm.run = mmap(
      NULL,
      vcpu_mmap_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      kvm.vcpu,
      0);
  if(kvm.run == MAP_FAILED) die();
}

void kvm_run(void) {
  while(1) {
    if(ioctl(kvm.vcpu, KVM_RUN, 0) < 0) die();

    switch(kvm.run->exit_reason) {
    case KVM_EXIT_HLT: return;
    default: die();
    }
  }
}

void kvm_print_regs(void) {
  if(ioctl(kvm.vcpu, KVM_GET_REGS, &kvm.regs) < 0) die();

  printf("    eax 0x%08llx    ecx 0x%08llx    edx 0x%08llx    ebx 0x%08llx\n"
      "    esp 0x%08llx    ebp 0x%08llx    esi 0x%08llx    edi 0x%08llx\n"
      "    eip 0x%08llx eflags 0x%08llx\n",
      kvm.regs.rax, kvm.regs.rcx, kvm.regs.rdx, kvm.regs.rbx,
      kvm.regs.rsp, kvm.regs.rbp, kvm.regs.rsi, kvm.regs.rdi,
      kvm.regs.rip, kvm.regs.rflags);
}

int main(void) {
  kvm_init();

  const int test_fd = open("tests/hlt.bin", O_RDONLY);
  if(test_fd < 0) die();

  const size_t memory_size = 0x10000;
  uint8_t* const memory = mmap(
      NULL,
      memory_size,
      PROT_READ | PROT_WRITE,
      // Zeroed mapping without swap.
      MAP_PRIVATE | MAP_NORESERVE,
      test_fd,
      0);
  if(memory == MAP_FAILED) die();

  madvise(memory, memory_size, MADV_MERGEABLE);

  kvm.memory_region.memory_size = memory_size;
  kvm.memory_region.userspace_addr = (uintptr_t)memory;
  if(ioctl(kvm.vm, KVM_SET_USER_MEMORY_REGION, &kvm.memory_region) < 0)
    die();

  if(ioctl(kvm.vcpu, KVM_GET_SREGS, &kvm.sregs) < 0) die();
  kvm.sregs.cs.selector = 0;
  kvm.sregs.cs.base = 0;
  if(ioctl(kvm.vcpu, KVM_SET_SREGS, &kvm.sregs) < 0) die();

  kvm.regs.rip = 0;
  if(ioctl(kvm.vcpu, KVM_SET_REGS, &kvm.regs) < 0) die();

  kvm_print_regs();

  kvm_run();

  kvm_print_regs();

  return 0;
}
