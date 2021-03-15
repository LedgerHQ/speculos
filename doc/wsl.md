## Using speculos from Windows Subsystem for Linux 2

Building in WSL2 is identical to the procedure in [building](build.md).

Using Speculos with display features requires correctly exporting the X display. Steps to do so:

- Add the following to your `.bashrc` within WSL2:

```bash
export DISPLAY="$(sed -n 's/nameserver //p' /etc/resolv.conf):0"
```

- Install an X server on your Windows host, like [VcXSrv](https://sourceforge.net/projects/vcxsrv/).
- Simply launch the X server prior to launching an app through Speculos from WSL
