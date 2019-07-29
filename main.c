#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct kvm {
  int fd;
  int vm;
  int vcpu;
};

struct kvm kvm;

void die(const char* format, ...) __attribute__((format(printf, 1, 2)));

void die(const char* format, ...) {
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  if(format[strlen(format) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);
  } else {
    fputc('\n', stderr);
  }

  exit(1);
}

void kvm_init(void) {
  kvm.fd = open("/dev/kvm", O_RDWR);
  if(kvm.fd < 0) die("/dev/kvm:");

  // Assert correct `KVM_API_VERSION`.
  const int api_version = ioctl(kvm.fd, KVM_GET_API_VERSION, 0);
  if(api_version < 0) die("KVM_GET_API_VERSION:");
  if(api_version != KVM_API_VERSION)
    die("Got API version %d, expected %d.", api_version, KVM_API_VERSION);

  kvm.vm = ioctl(kvm.fd, KVM_CREATE_VM, 0);
  if(kvm.vm < 0) die("KVM_CREATE_VM:");

  // Define the physical address of a three-page region in the guest physical
  // address space. Required on Intel-based hosts because of a quirk in the
  // virtualization implementation.
  //
  // TODO: check if and why this is necessary.
  if(ioctl(kvm.vm, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) die("KVM_SET_TSS_ADDR:");
}

int main(void) {
  kvm_init();

  const int test_fd = open("tests/hlt.bin", O_RDONLY);
  if(test_fd < 0) die("tests/hlt.bin:");

  const size_t memory_size = 0x10000;
  void* const memory = mmap(
      NULL,
      memory_size,
      PROT_READ | PROT_WRITE,
      // Zeroed mapping without swap.
      MAP_PRIVATE | MAP_NORESERVE,
      test_fd,
      0);
  if(memory == MAP_FAILED) die("memory:");

  madvise(memory, memory_size, MADV_MERGEABLE);

  struct kvm_userspace_memory_region memory_region = {0};
  memory_region.memory_size = memory_size;
  memory_region.userspace_addr = (uintptr_t)memory;
  if(ioctl(kvm.vm, KVM_SET_USER_MEMORY_REGION, &memory_region) < 0)
    die("KVM_SET_USER_MEMORY_REGION:");

  const int vcpu_fd = ioctl(kvm.vm, KVM_CREATE_VCPU, 0);
  if(vcpu_fd < 0) die("KVM_CREATE_VCPU:");

  int vcpu_mmap_size = ioctl(kvm.fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if(vcpu_mmap_size <= 0) die("KVM_GET_VCPU_MMAP_SIZE:");

  struct kvm_run* const run = mmap(
      NULL,
      vcpu_mmap_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      vcpu_fd,
      0);
  if(run == MAP_FAILED) die("run:");

  struct kvm_sregs sregs;
  if(ioctl(vcpu_fd, KVM_GET_SREGS, &sregs) < 0) die("KVM_GET_SREGS:");
  sregs.cs.selector = 0;
  sregs.cs.base = 0;
  if(ioctl(vcpu_fd, KVM_SET_SREGS, &sregs) < 0) die("KVM_SET_SREGS:");

  struct kvm_regs regs = {0};
  regs.rip = 0;
  if(ioctl(vcpu_fd, KVM_SET_REGS, &regs) < 0) die("KVM_SET_REGS:");

  printf("rax = 0x%llx\n", regs.rax);

  if(ioctl(vcpu_fd, KVM_RUN, 0) < 0) die("KVM_RUN:");

  if(run->exit_reason != KVM_EXIT_HLT) {
    fprintf(stderr, "Got exit_reason %d, expected KVM_EXIT_HLT (%d).\n",
        run->exit_reason, KVM_EXIT_HLT);
    return 1;
  }

  if(ioctl(vcpu_fd, KVM_GET_REGS, &regs) < 0) die("KVM_GET_REGS:");

  printf("rax = 0x%llx\n", regs.rax);

  return 0;
}
