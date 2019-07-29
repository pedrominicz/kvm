#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  const int kvm_fd = open("/dev/kvm", O_RDWR);
  if(kvm_fd < 0) {
    perror("/dev/kvm");
    return 1;
  }

  const int api_version = ioctl(kvm_fd, KVM_GET_API_VERSION, 0);
  if(api_version < 0) {
    perror("KVM_GET_API_VERSION");
    return 1;
  }
  assert(api_version == KVM_API_VERSION);

  const int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0);
  if(vm_fd < 0) {
    perror("KVM_CREATE_VM");
    return 1;
  }

  // Define the physical address of a three-page region in the guest physical
  // address space. Required on Intel-based hosts because of a quirk in the
  // virtualization implementation.
  if(ioctl(vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
    perror("KVM_SET_TSS_ADDR");
    return 1;
  }

  const size_t memory_size = 0x10000;
  void* const memory = mmap(
      NULL,
      memory_size,
      PROT_READ | PROT_WRITE,
      // Zeroed mapping without swap.
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
      // Ignored because of `MAP_ANONYMOUS`.
      -1,
      // Ignored because of `MAP_ANONYMOUS`.
      0);
  if(memory == MAP_FAILED) {
    perror("memory");
    return 1;
  }

  madvise(memory, memory_size, MADV_MERGEABLE);

  struct kvm_userspace_memory_region memory_region = {0};
  memory_region.memory_size = memory_size;
  memory_region.userspace_addr = (uintptr_t)memory;
  if(ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &memory_region) < 0) {
    perror("KVM_SET_USER_MEMORY_REGION");
    return 1;
  }

  const int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0);
  if(vcpu_fd < 0) {
    perror("KVM_CREATE_VCPU");
    return 1;
  }

  int vcpu_mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if(vcpu_mmap_size <= 0) {
    perror("KVM_GET_VCPU_MMAP_SIZE");
    return 1;
  }

  struct kvm_run* const run = mmap(
      NULL,
      vcpu_mmap_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      vcpu_fd,
      0);
  if(run == MAP_FAILED) {
    perror("run");
    return 1;
  }

  struct kvm_sregs sregs;
  if(ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
    perror("KVM_GET_SREGS");
    return 1;
  }
  sregs.cs.selector = 0;
  sregs.cs.base = 0;
  if(ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) {
    perror("KVM_SET_SREGS");
    return 1;
  }

  struct kvm_regs regs = {0};
  regs.rflags = 2;
  regs.rip = 0;
  if(ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) {
    perror("KVM_SET_REGS");
    return 1;
  }

  printf("rax = %llx\n", regs.rax);

  const uint8_t code[] = {
    0xb0, 0x25, // mov al, 0x25
    0xcc, // int 3
    0xf4, // hlt
  };
  memcpy(memory, code, sizeof(code));

  if(ioctl(vcpu_fd, KVM_RUN, 0) < 0) {
    perror("KVM_RUN");
    return 1;
  }

  if(run->exit_reason != KVM_EXIT_HLT) {
    fprintf(stderr, "Got exit_reason %d, expected KVM_EXIT_HLT (%d).\n",
        run->exit_reason, KVM_EXIT_HLT);
    return 1;
  }

  if(ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) {
    perror("KVM_GET_REGS");
    return 1;
  }

  printf("rax = %llx\n", regs.rax);

  return 0;
}
