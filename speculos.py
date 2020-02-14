#!/usr/bin/env python3

'''
Emulate the target app along the SE Proxy Hal server.
'''

import argparse
import binascii
import ctypes
from elftools.elf.elffile import ELFFile
import logging
from mnemonic import mnemonic
import os
import re
import signal
import socket
import sys
import threading

import pkg_resources

from mcu import apdu as apdu_server
from mcu import automation
from mcu import display
from mcu import seproxyhal
from mcu.automation_server import AutomationClient, AutomationServer
from mcu.button_tcp import FakeButton
from mcu.finger_tcp import FakeFinger
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
            logger.error('failed to find _estack/END_STACK symbol')
            sys.exit(1)
        estack = sym_estack[0]['st_value']
        supp_ram = elf.get_section_by_name('.rfbss')
        ram_addr, ram_size = (supp_ram['sh_addr'], supp_ram['sh_size']) if supp_ram is not None else (0, 0)
    stack_size = estack - stack
    return sh_offset, sh_size, stack, stack_size, ram_addr, ram_size

def run_qemu(s1, s2, app_path, libraries=[], seed=DEFAULT_SEED, debug=False, trace_syscalls=False, deterministic_rng="", model=None, ram_arg=None, sdk_version="1.5"):
    args = [ 'qemu-arm' ]

    if debug:
        args += [ '-g', '1234', '-singlestep' ]

    args += [ launcher_path ]

    if trace_syscalls:
        args += [ '-t' ]

    if model is not None:
        args += ['-m', model]

    args += [ '-k', str(sdk_version) ]

    extra_ram = ''
    for lib in [ f'main:{app_path}' ] + libraries:
        name, lib_path = lib.split(':')
        load_offset, load_size, stack, stack_size, ram_addr, ram_size = get_elf_infos(lib_path)
        # Since binaries loaded as libs could also declare extra RAM page(s), collect them all
        if (ram_addr, ram_size) != (0, 0):
            arg = f'{ram_addr:#x}:{ram_size:#x}'
            if extra_ram and arg != extra_ram:
                logger.error("different extra RAM pages for main app and/or libraries!")
                sys.exit(1)
            extra_ram = arg
        args.append(f'{name}:{lib_path}:{load_offset:#x}:{load_size:#x}:{stack:#x}:{stack_size:#x}')

    if model == 'blue':
        if ram_arg:
            extra_ram = ram_arg
        if extra_ram:
            args.extend([ '-r', extra_ram ])

    pid = os.fork()
    if pid != 0:
        return

    # ensure qemu is killed when this Python script exits
    set_pdeath(signal.SIGTERM)

    s2.close()

    # replace stdin with the socket
    os.dup2(s1.fileno(), sys.stdin.fileno())

    # handle both BIP39 mnemonics and hex seeds
    if seed.startswith("hex:"):
        seed = bytes.fromhex(seed[4:])
    else:
        seed = mnemonic.Mnemonic.to_seed(seed)

    os.environ['SPECULOS_SEED'] = binascii.hexlify(seed).decode('ascii')

    if deterministic_rng:
        os.environ['RNG_SEED'] = deterministic_rng

    logger.debug(f"executing qemu: {args}")
    try:
        os.execvp(args[0], args)
    except FileNotFoundError:
        logger.error('failed to execute qemu: "%s" not found' % args[0])
        sys.exit(1)
    sys.exit(0)

def setup_logging(args):
    logging.basicConfig(level=logging.INFO, format='%(asctime)s.%(msecs)03d:%(name)s: %(message)s', datefmt='%H:%M:%S')

    for arg in args.log_level:
        if ":" not in arg:
            logging.getLogger("speculos").error(f"invalid --log argument {arg}")
            sys.exit(1)
        name, level = arg.split(":", 1)
        logger = logging.getLogger(name)
        try:
            logger.setLevel(level)
        except ValueError as e:
            logging.getLogger("speculos").error(f"invalid --log argument: {e}")
            sys.exit(1)

    return logging.getLogger("speculos")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Emulate Ledger Nano/Blue apps.')
    parser.add_argument('app.elf', type=str, help='application path')
    parser.add_argument('--automation', type=str, help='Load a JSON document automating actions (prefix with "file:" to specify a path'),
    parser.add_argument('--color', default='MATTE_BLACK', choices=list(display.COLORS.keys()), help='Nano color')
    parser.add_argument('-d', '--debug', action='store_true', help='Wait gdb connection to port 1234')
    parser.add_argument('--deterministic-rng', default="", help='Seed the rng with a given value to produce deterministic randomness')
    parser.add_argument('-k', '--sdk', default='1.6', help='SDK version')
    parser.add_argument('-l', '--library', default=[], action='append', help='Additional library (eg. Bitcoin:app/btc.elf) which can be called through os_lib_call')
    parser.add_argument('--log-level', default=[], action='append', help='Configure the logger levels (eg. usb:DEBUG), can be specified multiple times')
    parser.add_argument('-m', '--model', default='nanos', choices=list(display.MODELS.keys()))
    parser.add_argument('-r', '--rampage', help='Additional RAM page and size available to the app (eg. 0x123000:0x100). Supercedes the internal probing for such page.')
    parser.add_argument('-s', '--seed', default=DEFAULT_SEED, help='BIP39 mnemonic or hex seed. Default to mnemonic: to use a hex seed, prefix it with "hex:"')
    parser.add_argument('-t', '--trace', action='store_true', help='Trace syscalls')

    group = parser.add_argument_group('network arguments')
    group.add_argument('--apdu-port', default=9999, type=int, help='ApduServer TCP port')
    group.add_argument('--automation-port', type=int, help='Forward text displayed on the screen to TCP clients'),
    group.add_argument('--vnc-port', type=int, help='Start a VNC server on the specified port')
    group.add_argument('--vnc-password', type=str, help='VNC plain-text password (required for MacOS Screen Sharing)')
    group.add_argument('--button-port', type=int, help='Spawn a TCP server on the specified port to receive button press (lLrR)')
    group.add_argument('--finger-port', type=int, help='Spawn a TCP server on the specified port to receive finger touch (x,y,pressed)+')

    group = parser.add_argument_group('display arguments', 'These arguments might only apply to one of the display method.')
    group.add_argument('--display', default='text', choices=['headless', 'qt', 'text'])
    group.add_argument('--ontop', action='store_true', help='The window stays on top of all other windows')
    group.add_argument('--keymap', action='store', help="Text UI keymap in the form of a string (e.g. 'was' => 'w' for left button, 'a' right, 's' both). Default: arrow keys")
    group.add_argument('--progressive', action='store_true', help='Enable step-by-step rendering of graphical elements')
    group.add_argument('--zoom', help='Display pixel size.', type=int, choices=range(1, 11))

    args = parser.parse_args()
    args.model.lower()

    logger = setup_logging(args)

    rendering = display.RENDER_METHOD.FLUSHED
    if args.progressive:
        rendering = display.RENDER_METHOD.PROGRESSIVE

    if args.rampage:
        if args.model != 'blue':
            logger.error("extra RAM page arguments -r (--rampage) require '-m blue'")
            sys.exit(1)
        if not re.match('(0x)?[0-9a-fA-F]+:(0x)?[0-9a-fA-F]+$', args.rampage):
            logger.error("invalid ram page argument")
            sys.exit(1)

    if args.display == 'text' and args.model != 'nanos':
        logger.error(f"unsupported model '{args.model}' with argument -x")
        sys.exit(1)

    if args.ontop and args.display != 'qt':
        logger.error("-o (--ontop) can only be used with --display qt")
        sys.exit(1)

    if args.zoom and args.display != 'qt':
        logger.error("-z (--zoom) can only be used with --display qt")
        sys.exit(1)

    if args.keymap and args.display != 'text':
        logger.error("-y (--keymap) can only be used with --display text")
        sys.exit(1)

    if args.vnc_password and not args.vnc_port:
        logger.error("--vnc-password can only be used with --vnc-port")
        sys.exit(1)

    if args.display == 'text':
        from mcu.screen_text import TextScreen as Screen
    elif args.display == 'headless':
        from mcu.headless import Headless as Screen
    else:
        from mcu.screen import QtScreen as Screen

    automation_path = None
    if args.automation:
        automation_path = automation.Automation(args.automation)

    automation_server = None
    if args.automation_port:
        automation_server = AutomationServer(("0.0.0.0", args.automation_port), AutomationClient)
        automation_thread = threading.Thread(target=automation_server.serve_forever, daemon=True)
        automation_thread.start()

    s1, s2 = socket.socketpair()

    run_qemu(s1, s2, getattr(args, 'app.elf'), args.library, args.seed, args.debug, args.trace, args.deterministic_rng, args.model, args.rampage, args.sdk)
    s1.close()

    apdu = apdu_server.ApduServer(host="0.0.0.0", port=args.apdu_port)
    seph = seproxyhal.SeProxyHal(s2, automation=automation_path, automation_server=automation_server)

    button_tcp = None
    if args.button_port:
        button_tcp = FakeButton(args.button_port)

    finger_tcp = None
    if args.finger_port:
        finger_tcp = FakeFinger(args.finger_port)

    vnc = None
    if args.vnc_port:
        vnc = VNC(args.vnc_port, args.model, args.vnc_password)

    zoom = args.zoom
    if zoom is None:
        zoom = {'nanos':2, 'blue': 1}.get(args.model, 1)

    screen = Screen(apdu, seph, button_tcp=button_tcp, finger_tcp=finger_tcp, color=args.color, model=args.model, ontop=args.ontop, rendering=rendering, vnc=vnc, keymap=args.keymap, pixel_size=zoom)
    screen.run()

    s2.close()
