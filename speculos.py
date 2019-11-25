#!/usr/bin/env python3

'''
Emulate the target app along the SE Proxy Hal server.
'''

import argparse
import binascii
import ctypes
from elftools.elf.elffile import ELFFile
from mnemonic import mnemonic
import os
import signal
import socket
import sys

import pkg_resources

from mcu import apdu as apdu_server
from mcu import display
from mcu import seproxyhal
from mcu.button_tcp import FakeButton
from mcu.vnc import VNC

DEFAULT_SEED = 'glory promote mansion idle axis finger extra february uncover one trip resource lawn turtle enact monster seven myth punch hobby comfort wild raise skin'

launcher_path = pkg_resources.resource_filename(__name__, "/build/src/launcher")

def set_pdeath(sig):
    '''Set the parent death signal of the calling process.'''

    PR_SET_PDEATHSIG = 1
    libc = ctypes.cdll.LoadLibrary('libc.so.6')
    libc.prctl(PR_SET_PDEATHSIG, sig)

def get_elf_infos(app_path):
    with open(app_path, 'rb') as fp:
        elf = ELFFile(fp)
        text = elf.get_section_by_name('.text')
        symtab =  elf.get_section_by_name('.symtab')
        bss = elf.get_section_by_name('.bss')
        sh_offset = text['sh_offset']
        sh_size = text['sh_size']
        stack = bss['sh_addr']
        sym_estack = symtab.get_symbol_by_name('_estack')
        if sym_estack is None:
            sym_estack = symtab.get_symbol_by_name('END_STACK')
        if sym_estack is None:
            print('[-] failed to find _estack/END_STACK symbol', file=sys.stderr)
            sys.exit(1)
        estack = sym_estack[0]['st_value']
    stack_size = estack - stack
    return hex(sh_offset), hex(sh_size), hex(stack), hex(stack_size)

def run_qemu(s1, s2, app_path, libraries=[], seed=DEFAULT_SEED, debug=False, trace_syscalls=False, model=None, rampage=None, pagesize=None, sdk_version="1.5"):
    args = [ 'qemu-arm-static' ]

    if debug:
        args += [ '-g', '1234', '-singlestep' ]

    args += [ launcher_path ]

    if trace_syscalls:
        args += [ '-t' ]

    if rampage is not None and pagesize is not None:
        args += [ '-r' , rampage ] + [ '-s' , pagesize ]

    if model is not None:
        args += ['-m', model]

    args += [ '-k', str(sdk_version) ]

    for lib in [ f'main:{app_path}' ] + libraries:
        name, lib_path = lib.split(':')
        load_offset, load_size, stack, stack_size = get_elf_infos(lib_path)
        args.append(f'{name}:{lib_path}:{load_offset}:{load_size}:{stack}:{stack_size}')

    pid = os.fork()
    if pid != 0:
        return

    # ensure qemu is killed when this Python script exits
    set_pdeath(signal.SIGTERM)

    s2.close()

    # replace stdin with the socket
    os.dup2(s1.fileno(), sys.stdin.fileno())

    seed = mnemonic.Mnemonic.to_seed(seed)
    os.environ['SPECULOS_SEED'] = binascii.hexlify(seed).decode('ascii')

    #print('[*] seproxyhal: executing qemu', file=sys.stderr)
    try:
        os.execvp(args[0], args)
    except FileNotFoundError:
        print('[-] failed to execute qemu: "%s" not found' % args[0], file=sys.stderr)
        sys.exit(1)

    sys.exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Emulate Ledger Nano/Blue apps.')
    parser.add_argument('app.elf', type=str, help='application path')
    parser.add_argument('-a', '--apdu-port', default=9999, type=int, help='ApduServer TCP port')
    parser.add_argument('--color', default='MATTE_BLACK', choices=list(display.COLORS.keys()), help='Nano color')
    parser.add_argument('-b', '--button-tcp', action='store_true', help='Spawn a TCP server on port 1235 to receive button press (lLrR)')
    parser.add_argument('-d', '--debug', action='store_true', help='Wait gdb connection to port 1234')
    parser.add_argument('-k', '--sdk', default='1.6', help='SDK version')
    parser.add_argument('-l', '--library', default=[], action='append', help='Additional library (eg. Bitcoin:app/btc.elf) which can be called through os_lib_call')
    parser.add_argument('-m', '--model', default='nanos', choices=list(display.MODELS.keys()))
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-n', '--headless', action='store_true', help="Don't display the GUI")
    group.add_argument('-o', '--ontop', action='store_true', help='The window stays on top of all other windows')
    parser.add_argument('-x', '--text', action='store_true', help="Text UI (implies --headless)")
    parser.add_argument('-y', '--keymap', action='store', help="Text UI keymap in the form of a string (e.g. 'was' => 'w' for left button, 'a' right, 's' both). Default: arrow keys")
    parser.add_argument('-r', '--rampage', required='--pagesize' in sys.argv or '-q' in sys.argv, type=lambda x: f"{int(x,16):X}", action='store', help='Hex address (prefixed or not with \'0x\') of one additional RAM page available to the app. Requires -q.')
    parser.add_argument('-q', '--pagesize', required='--rampage' in sys.argv or '-r' in sys.argv, type=lambda x: f"{int(x,16):X}", action='store', help='Byte size (in decimal or in hex if prefixed with \'0x\') of the additional RAM page available to the app. Requires -r.')
    group_progressive = parser.add_mutually_exclusive_group()
    group_progressive.add_argument('-p', '--progressive', action='store_true', help='Enable step-by-step rendering of graphical elements (default for Blue, can be disabled with -P)')
    group_progressive.add_argument('-P', '--flushed', action='store_true', help='Disable step-by-step rendering of graphical elements (default for Nano S)')
    parser.add_argument('-s', '--seed', default=DEFAULT_SEED, help='Seed')
    parser.add_argument('-t', '--trace', action='store_true', help='Trace syscalls')
    parser.add_argument('-v', '--vnc-port', type=int, help='Start a VNC server on the specified port')
    parser.add_argument('-z', '--zoom', help='Display pixel size.', type=int, choices=range(1, 11))

    args = parser.parse_args()
    args.model.lower()

    if not (args.flushed or args.progressive):
        if args.model == 'blue':
            args.progressive = True
        else:
            args.flushed = True

    if args.text:
        if args.model != 'nanos':
            raise ValueError(f"Unsupported model '{args.model}' with -x")
        if args.ontop:
            raise ValueError("-x (--text) and -o (--ontop) are mutually exclusive")

        args.headless = True
        from mcu import screen_text as screen
    elif args.headless:
        from mcu import headless as screen
    else:
        from mcu import screen

    s1, s2 = socket.socketpair()

    run_qemu(s1, s2, getattr(args, 'app.elf'), args.library, args.seed, args.debug, args.trace, args.model, args.rampage, args.pagesize, args.sdk)
    s1.close()

    apdu = apdu_server.ApduServer(host="0.0.0.0", port=args.apdu_port)
    seph = seproxyhal.SeProxyHal(s2)

    button_tcp = None
    if args.button_tcp:
        button_tcp = FakeButton()

    vnc = None
    if args.vnc_port:
        vnc = VNC(args.vnc_port, args.model)

    zoom = args.zoom
    if zoom is None:
        zoom = {'nanos':2, 'blue': 1}.get(args.model, 1)

    render_method = display.RENDER_METHOD.PROGRESSIVE if args.progressive == True else display.RENDER_METHOD.FLUSHED
    screen.display(apdu, seph, button_tcp=button_tcp, color=args.color, model=args.model, ontop=args.ontop, rendering=render_method, vnc=vnc, keymap=args.keymap, pixel_size=zoom)

    s2.close()
