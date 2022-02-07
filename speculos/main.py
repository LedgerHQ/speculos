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

from .api import ApiRunner, EventsBroadcaster
from .mcu import apdu as apdu_server
from .mcu import automation
from .mcu import display
from .mcu import seproxyhal
from .mcu.automation_server import AutomationClient, AutomationServer
from .mcu.button_tcp import FakeButton
from .mcu.finger_tcp import FakeFinger
from .mcu.vnc import VNC


DEFAULT_SEED = ('glory promote mansion idle axis finger extra february uncover one trip resource lawn turtle enact '
                'monster seven myth punch hobby comfort wild raise skin')

logger = logging.getLogger("speculos")

launcher_path = pkg_resources.resource_filename(__name__, "/resources/launcher")


def set_pdeath(sig):
    '''Set the parent death signal of the calling process.'''

    PR_SET_PDEATHSIG = 1
    libc = ctypes.cdll.LoadLibrary('libc.so.6')
    libc.prctl(PR_SET_PDEATHSIG, sig)


def get_elf_infos(app_path):
    with open(app_path, 'rb') as fp:
        elf = ELFFile(fp)
        text = elf.get_section_by_name('.text')
        symtab = elf.get_section_by_name('.symtab')
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


def run_qemu(s1: socket.socket, s2: socket.socket, args: argparse.Namespace) -> None:
    argv = ['qemu-arm-static']

    if args.debug:
        argv += ['-g', '1234', '-singlestep']

    argv += [launcher_path]

    if args.trace:
        argv += ['-t']

    if args.model is not None:
        argv += ['-m', args.model]

    argv += ['-k', str(args.sdk)]

    # load cxlib only if available for the specified sdk
    cxlib = pkg_resources.resource_filename(__name__, f"/cxlib/{args.model}-cx-{args.sdk}.elf")
    if os.path.exists(cxlib):
        argv += ['-c', cxlib]

    extra_ram = ''
    app_path = getattr(args, 'app.elf')
    for lib in [f'main:{app_path}'] + args.library:
        name, lib_path = lib.split(':')
        load_offset, load_size, stack, stack_size, ram_addr, ram_size = get_elf_infos(lib_path)
        # Since binaries loaded as libs could also declare extra RAM page(s), collect them all
        if (ram_addr, ram_size) != (0, 0):
            arg = f'{ram_addr:#x}:{ram_size:#x}'
            if extra_ram and arg != extra_ram:
                logger.error("different extra RAM pages for main app and/or libraries!")
                sys.exit(1)
            extra_ram = arg
        argv.append(f'{name}:{lib_path}:{load_offset:#x}:{load_size:#x}:{stack:#x}:{stack_size:#x}')

    if args.model == 'blue':
        if args.rampage:
            extra_ram = args.rampage
        if extra_ram:
            argv.extend(['-r', extra_ram])

    pid = os.fork()
    if pid != 0:
        return

    # ensure qemu is killed when this Python script exits
    set_pdeath(signal.SIGTERM)

    s2.close()

    # replace stdin with the socket
    os.dup2(s1.fileno(), sys.stdin.fileno())

    # handle both BIP39 mnemonics and hex seeds
    if args.seed.startswith("hex:"):
        seed = bytes.fromhex(args.seed[4:])
    else:
        seed = mnemonic.Mnemonic.to_seed(args.seed)

    os.environ['SPECULOS_SEED'] = binascii.hexlify(seed).decode('ascii')

    if args.deterministic_rng:
        os.environ['RNG_SEED'] = args.deterministic_rng

    logger.debug(f"executing qemu: {argv}")
    try:
        os.execvp(argv[0], argv)
    except FileNotFoundError:
        logger.error('failed to execute qemu: "%s" not found' % argv[0])
        sys.exit(1)
    sys.exit(0)


def setup_logging(args):
    logging.basicConfig(level=logging.INFO, format='%(asctime)s.%(msecs)03d:%(name)s: %(message)s', datefmt='%H:%M:%S')

    for arg in args.log_level:
        if ":" not in arg:
            logger.error(f"invalid --log argument {arg}")
            sys.exit(1)
        name, level = arg.split(":", 1)
        given_logger = logging.getLogger(name)
        try:
            given_logger.setLevel(level)
        except ValueError as e:
            logger.error(f"invalid --log argument: {e}")
            sys.exit(1)


def main(prog=None):
    parser = argparse.ArgumentParser(description='Emulate Ledger Nano/Blue apps.')
    parser.add_argument('app.elf', type=str, help='application path')
    parser.add_argument('--automation', type=str, help='Load a JSON document automating actions (prefix with "file:" '
                                                       'to specify a path')
    parser.add_argument('--color', default='MATTE_BLACK', choices=list(display.COLORS.keys()), help='Nano color')
    parser.add_argument('-d', '--debug', action='store_true', help='Wait gdb connection to port 1234')
    parser.add_argument('--deterministic-rng', default="", help='Seed the rng with a given value to produce '
                                                                'deterministic randomness')
    parser.add_argument('-k', '--sdk', type=str, help='SDK version')
    parser.add_argument('-l', '--library', default=[], action='append', help='Additional library (eg. '
                        'Bitcoin:app/btc.elf) which can be called through os_lib_call')
    parser.add_argument('--log-level', default=[], action='append', help='Configure the logger levels (eg. usb:DEBUG), '
                                                                         'can be specified multiple times')
    parser.add_argument('-m', '--model', default='nanos', choices=list(display.MODELS.keys()))
    parser.add_argument('-r', '--rampage', help='Additional RAM page and size available to the app (eg. '
                                                '0x123000:0x100). Supersedes the internal probing for such page.')
    parser.add_argument('-s', '--seed', default=DEFAULT_SEED, help='BIP39 mnemonic or hex seed. Default to mnemonic: '
                                                                   'to use a hex seed, prefix it with "hex:"')
    parser.add_argument('-t', '--trace', action='store_true', help='Trace syscalls')
    parser.add_argument('-u', '--usb', default='hid', help='Configure the USB transport protocol, '
                                                           'either HID (default) or U2F')

    group = parser.add_argument_group('network arguments')
    group.add_argument('--apdu-port', default=9999, type=int, help='ApduServer TCP port')
    group.add_argument('--api-port', default=5000, type=int, help='Set the REST API TCP port (0 disables it)')
    group.add_argument('--automation-port', type=int, help='Forward text displayed on the screen to TCP clients')
    group.add_argument('--vnc-port', type=int, help='Start a VNC server on the specified port')
    group.add_argument('--vnc-password', type=str, help='VNC plain-text password (required for MacOS Screen Sharing)')
    group.add_argument('--button-port', type=int, help='Spawn a TCP server on the specified port to receive button '
                                                       'press (lLrR)')
    group.add_argument('--finger-port', type=int, help='Spawn a TCP server on the specified port to receive finger '
                                                       'touch (x,y,pressed)+')

    group = parser.add_argument_group('display arguments', 'These arguments might only apply to one of the display '
                                                           'method.')
    group.add_argument('--display', default='qt', choices=['headless', 'qt', 'text'])
    group.add_argument('--ontop', action='store_true', help='The window stays on top of all other windows')
    group.add_argument('--xy', action='store', help='Window position in "XxY" format (eg. "--xy 30x100").')
    group.add_argument('--keymap', action='store', help="Text UI keymap in the form of a string (e.g. 'was' => 'w' for "
                                                        "left button, 'a' right, 's' both). Default: arrow keys")
    group.add_argument('--progressive', action='store_true', help='Enable step-by-step rendering of graphical elements')
    group.add_argument('--zoom', help='Display pixel size.', type=int, choices=range(1, 11))

    if prog:
        parser.prog = prog
    args = parser.parse_args()
    args.model.lower()

    setup_logging(args)

    rendering = seproxyhal.RENDER_METHOD.FLUSHED
    if args.progressive:
        rendering = seproxyhal.RENDER_METHOD.PROGRESSIVE

    if args.rampage:
        if args.model != 'blue':
            logger.error("extra RAM page arguments -r (--rampage) require '-m blue'")
            sys.exit(1)
        if not re.match('(0x)?[0-9a-fA-F]+:(0x)?[0-9a-fA-F]+$', args.rampage):
            logger.error("invalid ram page argument")
            sys.exit(1)

    if args.display == 'text' and args.model not in ['nanos', 'nanox', 'nanosp']:
        logger.error(f"unsupported model '{args.model}' with argument --display text")
        sys.exit(1)

    if args.ontop and args.display != 'qt':
        logger.error("--ontop can only be used with --display qt")
        sys.exit(1)

    if args.xy and args.display != 'qt':
        logger.error("--xy can only be used with --display qt")
        sys.exit(1)

    if args.zoom and args.display != 'qt':
        logger.error("--zoom can only be used with --display qt")
        sys.exit(1)

    if args.keymap and args.display != 'text':
        logger.error("--keymap can only be used with --display text")
        sys.exit(1)

    if args.vnc_password and not args.vnc_port:
        logger.error("--vnc-password can only be used with --vnc-port")
        sys.exit(1)

    if args.display == 'text':
        from .mcu.screen_text import TextScreen as Screen
    elif args.display == 'headless':
        from .mcu.headless import Headless as Screen
    else:
        from .mcu.screen import QtScreen as Screen

    if args.sdk is None:
        default_sdk = {
            "nanos": "2.1",
            "nanox": "2.0.2",
            "blue": "blue-2.2.5",
            "nanosp": "1.0"
        }
        args.sdk = default_sdk.get(args.model)

    api_enabled = (args.api_port != 0)

    automation_path = None
    if args.automation:
        logger.warn("--automation is deprecated, please use the REST API instead")
        automation_path = automation.Automation(args.automation)

    automation_server = None
    if args.automation_port:
        logger.warn("--automation-port is deprecated, please use the REST API instead")
        if api_enabled:
            logger.warn("--automation-port is incompatible with the the API server, disabling the latter")
            api_enabled = False
        automation_server = AutomationServer(("0.0.0.0", args.automation_port), AutomationClient)
        automation_thread = threading.Thread(target=automation_server.serve_forever, daemon=True)
        automation_thread.start()

    if api_enabled:
        automation_server = EventsBroadcaster()

    s1, s2 = socket.socketpair()

    run_qemu(s1, s2, args)
    s1.close()

    apdu = apdu_server.ApduServer(host="0.0.0.0", port=args.apdu_port)
    seph = seproxyhal.SeProxyHal(s2, automation=automation_path, automation_server=automation_server,
                                 transport=args.usb)

    button = None
    if args.button_port:
        logger.warn("--button-port is deprecated, please use the REST API instead")
        button = FakeButton(args.button_port)

    finger = None
    if args.finger_port:
        logger.warn("--finger-port is deprecated, please use the REST API instead")
        finger = FakeFinger(args.finger_port)

    vnc = None
    if args.vnc_port:
        screen_size = display.MODELS[args.model].screen_size
        vnc = VNC(args.vnc_port, screen_size, args.vnc_password)

    zoom = args.zoom
    if zoom is None:
        default_zoom = {
            "nanos": 2,
            "nanox": 2,
            "nanosp": 2,
            "blue": 1,
        }
        zoom = default_zoom.get(args.model)

    x, y = None, None
    if args.xy:
        x, y = (int(i) for i in args.xy.split('x'))

    apirun = None
    if api_enabled:
        apirun = ApiRunner(args.api_port)

    display_args = display.DisplayArgs(args.color, args.model, args.ontop, rendering, args.keymap, zoom, x, y)
    server_args = display.ServerArgs(apdu, apirun, button, finger, seph, vnc)
    screen = Screen(display_args, server_args)

    if api_enabled:
        apirun.start_server_thread(screen, seph, automation_server)

    screen.run()

    s2.close()
