## High priority

- Nano X support
- check api level
- print call stack on app segfault

## Low priority

- Don't allow apps to make Linux syscalls since Qemu will happily forward them
  to the host. Solution: launch Qemu using a seccomp profile.
- Add check when accessing app memory in the syscall implementation.
- MCU: add animated text support
- cleanup app loading
- Qt: circle
