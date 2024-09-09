---
sort: 7
---

# Semihosting as an additional debug mechanism

QEMU implements some semihosted operations which can be triggered from the app.
For instance, messages can be printed to stderr with the following code:

## SYS_WRITE0

```C
void debug_write(char *buf)
{
  asm volatile (
     "movs r0, #0x04\n"
     "movs r1, %0\n"
     "svc      0xab\n"
     :: "r"(buf) : "r0", "r1"
  );
}
```

The operation number must be passed in `r0` (here `SYS_WRITE0` operation is
defined to `0x04`) and arguments are in `r1`, `r2` and `r3`.

Usage:

```C
debug_write("magic!\n");
```


## References

- [ARM semihosting](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471c/Bgbjjgij.html)
- [Semihosting for AArch32 and AArch64 - Release 2.0](https://static.docs.arm.com/100863/0200/semihosting.pdf)
- [qemu/target/arm/arm-semi.c](https://github.com/qemu/qemu/blob/8de702cb677c8381fb702cae252d6b69aa4c653b/target/arm/arm-semi.c)
