## Using speculos from Windows Subsystem for Linux 2

Building in WSL2 is identical to the procedure in [building](doc/build.md).

Using Speculos with display features requires correctly exporting the X display. 

Rough steps to do so below, detailed procedure available on [this blogpost](https://techcommunity.microsoft.com/t5/windows-dev-appconsult/running-wsl-gui-apps-on-windows-10/ba-p/1493242).

- add the following to your `.bashrc` within WSL2:

```bash
export DISPLAY="`grep nameserver /etc/resolv.conf | sed 's/nameserver //'`:0"
```

- Install an X server on your Windows host, like [VcXSrv](https://sourceforge.net/projects/vcxsrv/), and disable access control when prompted to do so.
- Simply launch the X server prior to launching an app through Speculos from WSL


