#!/usr/bin/env python3

'''
Emulate the target app along the SE Proxy Hal server.
'''

import argparse
import binascii
import ctypes
import logging
import os
import re
import signal
import socket
import sys
import threading
from elftools.elf.elffile import ELFFile
from ledgered.binary import LedgerBinaryApp
from mnemonic import mnemonic
from typing import Optional, Type

from .api import ApiRunner, EventsBroadcaster
from .mcu import apdu as apdu_server
from .mcu import automation
from .mcu import display
from .mcu import seproxyhal
from .mcu.automation_server import AutomationClient, AutomationServer
from .mcu.button_tcp import FakeButton
from .mcu.finger_tcp import FakeFinger
from .mcu.struct import DisplayArgs, ServerArgs
from .mcu.vnc import VNC
from .observer import BroadcastInterface
from .resources_importer import resources


DEFAULT_SEED = ('glory promote mansion idle axis finger extra february uncover one trip resource lawn turtle enact '
                'monster seven myth punch hobby comfort wild raise skin')

logger = logging.getLogger("speculos")

launcher_path = str(resources.files(__package__) / "resources" / "launcher")


def set_pdeath(sig):
    '''Set the parent death signal of the calling process.'''

    PR_SET_PDEATHSIG = 1
    libc = ctypes.cdll.LoadLibrary('libc.so.6')
    libc.prctl(PR_SET_PDEATHSIG, sig)


def get_elf_infos(app_path, use_bagl):
    with open(app_path, 'rb') as fp:
        elf = ELFFile(fp)
        text = elf.get_section_by_name('.text')
        for seg in elf.iter_segments():
            if seg['p_type'] != 'PT_LOAD':
                continue
            if seg.section_in_segment(text):
                text_seg = seg
                break
        else:
            raise RuntimeError("No program header with text section!")
        symtab = elf.get_section_by_name('.symtab')
        bss = elf.get_section_by_name('.bss')
        sh_offset = text_seg['p_offset']
        sh_size = text_seg['p_filesz']
        text_load_addr = text['sh_addr']
        stack = bss['sh_addr']
        sym_estack = symtab.get_symbol_by_name('_estack')
        if sym_estack is None:
            sym_estack = symtab.get_symbol_by_name('END_STACK')
        if sym_estack is None:
            logger.error('failed to find _estack/END_STACK symbol')
            sys.exit(1)
        estack = sym_estack[0]['st_value']

        # Look for the symbols SVC_Call and SVC_cx_call
        # if they are found, save their addresses to patch them to replace the SYSCALL later
        svc_call_addr = 0
        svc_cx_call_addr = 0
        svc_call_symbol = symtab.get_symbol_by_name("SVC_Call")
        if svc_call_symbol is not None:
            svc_call_addr = svc_call_symbol[0]['st_value'] & (~1)
        svc_cx_call_symbol = symtab.get_symbol_by_name("SVC_cx_call")
        if svc_cx_call_symbol is not None:
            svc_cx_call_addr = svc_cx_call_symbol[0]['st_value'] & (~1)
        # Check where are located fonts in .elf file (LNX/LNS+ with BAGL only)
        # (on apps using NBGL, fonts are loaded from a known location: STAX_FONTS_ARRAY_ADDR,
        #  FLEX_FONTS_ARRAY_ADDR, NANOX_FONTS_ARRAY_ADDR or NANOSP_FONTS_ARRAY_ADDR)
        fonts_addr = 0
        fonts_size = 0
        bagl_fonts_symbol = symtab.get_symbol_by_name('C_bagl_fonts')
        if bagl_fonts_symbol is not None:
            fonts_addr = bagl_fonts_symbol[0]['st_value']
            fonts_size = bagl_fonts_symbol[0]['st_size']
            logger.info(f"Found C_bagl_fonts at 0x{fonts_addr:X} ({fonts_size} bytes)\n")
        elif use_bagl:
            logger.info("Disabling OCR.")

        supp_ram = elf.get_section_by_name('.rfbss')
        ram_addr, ram_size = (supp_ram['sh_addr'], supp_ram['sh_size']) if supp_ram is not None else (0, 0)
    stack_size = estack - stack
    return sh_offset, sh_size, stack, stack_size, ram_addr, ram_size, text_load_addr, \
        svc_call_addr, svc_cx_call_addr, fonts_addr, fonts_size


def get_cx_infos(app_path):
    with open(app_path, 'rb') as fp:
        elf = ELFFile(fp)
        text = elf.get_section_by_name('.text')
        cxram = elf.get_section_by_name('.cxram')
        sh_offset = text['sh_offset']
        sh_size = text['sh_size']
        sh_load = text['sh_addr']
        cx_ram_load = cxram["sh_addr"]
        cx_ram_size = cxram["sh_size"]

    return sh_offset, sh_size, sh_load, cx_ram_size, cx_ram_load


def run_qemu(s1: socket.socket, s2: socket.socket, args: argparse.Namespace, use_bagl: bool) -> int:
    argv = ['qemu-arm-static']

    if args.debug:
        argv += ['-g', '1234', '-singlestep']

    argv += [launcher_path]

    if args.trace:
        argv += ['-t']

    argv += ['-m', args.model]

    if args.apiLevel:
        argv += ['-a', str(args.apiLevel)]
    else:
        argv += ['-k', str(args.sdk)]

    # load cxlib only if available for the specified api level or sdk
    if args.apiLevel:
        cxlib_filepath = f"cxlib/{args.model}-api-level-cx-{args.apiLevel}.elf"
    else:
        cxlib_filepath = f"cxlib/{args.model}-cx-{args.sdk}.elf"
    cxlib = str(resources.files(__package__) / cxlib_filepath)
    if os.path.exists(cxlib):
        sh_offset, sh_size, sh_load, cx_ram_size, cx_ram_load = get_cx_infos(cxlib)
        cxlib_args = f'{cxlib}:{sh_offset:#x}:{sh_size:#x}:{sh_load:#x}:{cx_ram_size:#x}:{cx_ram_load:#x}'
        argv += ['-c', cxlib_args]
    else:
        logger.warn(f"Cx lib {cxlib_filepath} not found")

    # for NBGL apps, fonts binary file is mandatory
    if not use_bagl:
        fonts_filepath = f"fonts/{args.model}-fonts-{args.apiLevel}.bin"
        fonts = str(resources.files(__package__) / fonts_filepath)
        if os.path.exists(fonts):
            argv += ['-f', fonts]
        else:
            logger.error(f"Fonts {fonts_filepath} not found")
            sys.exit(1)

    extra_ram = ''
    app_path = getattr(args, 'app.elf')
    for lib in [f'main:{app_path}'] + args.library:
        name, lib_path = lib.split(':')
        load_offset, load_size, stack, stack_size, ram_addr, ram_size, \
            text_load_addr, svc_call_address, svc_cx_call_address, \
            fonts_addr, fonts_size = get_elf_infos(lib_path, use_bagl)

        # Since binaries loaded as libs could also declare extra RAM page(s), collect them all
        if (ram_addr, ram_size) != (0, 0):
            arg = f'{ram_addr:#x}:{ram_size:#x}'
            if extra_ram and arg != extra_ram:
                logger.error("different extra RAM pages for main app and/or libraries!")
                sys.exit(1)
            extra_ram = arg
        lib_arg = f'{name}:{lib_path}:{load_offset:#x}:{load_size:#x}'
        lib_arg += f':{stack:#x}:{stack_size:#x}:{svc_call_address:#x}'
        lib_arg += f':{svc_cx_call_address:#x}:{text_load_addr:#x}'
        lib_arg += f':{fonts_addr:#x}:{fonts_size:#x}'
        argv.append(lib_arg)

    if args.model == 'blue':
        if args.rampage:
            extra_ram = args.rampage
        if extra_ram:
            argv.extend(['-r', extra_ram])

    pid = os.fork()
    if pid != 0:
        return pid

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

    if args.user_private_key:
        os.environ['USER_PRIVATE_KEY'] = args.user_private_key
    if args.attestation_key:
        os.environ['ATTESTATION_PRIVATE_KEY'] = args.attestation_key

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


def main(prog=None) -> int:

    parser = argparse.ArgumentParser(description='Emulate Ledger Nano/Blue apps.')
    parser.add_argument('app.elf', type=str, help='application path')
    parser.add_argument('--automation', type=str, help='Load a JSON document automating actions (prefix with "file:" '
                                                       'to specify a path')
    parser.add_argument('--color', default='MATTE_BLACK', choices=list(display.COLORS.keys()), help='Nano color')
    parser.add_argument('-d', '--debug', action='store_true', help='Wait gdb connection to port 1234')
    parser.add_argument('--deterministic-rng', default='', help='Seed the rng with a given value to produce '
                                                                'deterministic randomness')
    parser.add_argument('--user-private-key', default='',
                        help='32B in hex format, will be used as the user private keys')
    parser.add_argument('--attestation-key', default='', help='32B in hex format, will be used as the private '
                                                              'attestation key')
    parser.add_argument('-k', '--sdk', type=str, help='SDK version')
    parser.add_argument('-a', '--apiLevel', type=str, help='Api level')
    parser.add_argument('-l', '--library', default=[], action='append', help='Additional library (eg. '
                        'Bitcoin:app/btc.elf) which can be called through os_lib_call'
                        'You can also only pass the lib elf path if the lib name is included in the metadata')
    parser.add_argument('--log-level', default=[], action='append', help='Configure the logger levels (eg. usb:DEBUG), '
                                                                         'can be specified multiple times')
    parser.add_argument('-m', '--model', choices=list(display.MODELS.keys()))
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

    if args.model:
        args.model = args.model.lower()

    # Initialize root logging level and handlers and module specific level if requested in command line
    setup_logging(args)

    # Init model and api_level if not specified from app elf metadata
    app_path = getattr(args, 'app.elf')
    binary = LedgerBinaryApp(app_path)
    if not args.model:
        if binary.sections.target is None:
            logger.error("Device model not detected from elf. Then it must be specified")
            sys.exit(1)
        else:
            args.model = "nanosp" if binary.sections.target == "nanos2" else binary.sections.target
            logger.warn(f"Device model detected from metadata: {args.model}")

    # 'bagl' is the default value of the binary.sections.sdk_graphics. We need to
    # manage the cases where it is NOT 'bagl' but the section does not exists yet
    if args.model in ["stax", "flex"]:
        use_bagl = False
    else:
        use_bagl = binary.sections.sdk_graphics == "bagl"

    if not args.apiLevel:
        if binary.sections.api_level is not None:
            args.apiLevel = binary.sections.api_level
            logger.warn(f"Api level detected from metadata: {args.apiLevel}")

    # Check args.apiLevel, 0 is an invalid value
    if args.apiLevel == "0":
        logger.error(f"Invalid api_level {args.apiLevel}")
        sys.exit(1)

    # Set SPECULOS_DETECTED_APPNAME env variable for proper emulation.
    # See sys_os_registry_get_current_app_tag() for corresponding usage.
    app_name = binary.sections.app_name
    app_version = binary.sections.app_version
    if app_name and app_version:
        os.environ["SPECULOS_DETECTED_APPNAME"] = f"{app_name}:{app_version}"

    # Retrieve lib app_name if available and check it against argument now optional
    libs = []
    for lib_arg in args.library:
        if ":" in lib_arg:
            lib_name, lib_path = lib_arg.split(":")
        else:
            lib_name = None
            lib_path = lib_arg

        elf_lib_name = LedgerBinaryApp(lib_path).sections.app_name

        if lib_name is None:
            if elf_lib_name is None:
                logger.error("Lib name not detected from elf. Then it must be specified")
                sys.exit(1)
            else:
                logger.warn(f"Lib name detected from metadata: {elf_lib_name}")
                lib_name = elf_lib_name
        else:
            if elf_lib_name is not None and elf_lib_name != lib_name:
                logger.error(f"Invalid lib name in {lib_path} ({elf_lib_name} vs {lib_name})")
                sys.exit(1)

        libs.append(f"{lib_name}:{lib_path}")
    args.library = libs

    # Check model and api_level against all lib elf metadata
    for path in [app_path] + [x.split(":")[1] for x in args.library]:
        binary = LedgerBinaryApp(path)

        elf_model = ("nanosp" if binary.sections.target == "nanos2" else binary.sections.target) or args.model
        if args.model != elf_model:
            logger.error(f"Invalid model in {path} ({elf_model} vs {args.model})")
            sys.exit(1)

        elf_api_level = binary.sections.api_level or "0"
        # Check args.apiLevel against elf api level. If elf api level == 0 (SDK master
        # reserved value) ignore it.
        if elf_api_level != "0" and args.apiLevel != elf_api_level:
            logger.error(f"Invalid api_level in {path} ({elf_api_level} vs {args.apiLevel})")
            sys.exit(1)

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

    ScreenNotifier: Type[display.DisplayNotifier]
    if args.display == 'text':
        from .mcu.screen_text import TextScreenNotifier as ScreenNotifier
    elif args.display == 'headless':
        from .mcu.headless import HeadlessNotifier as ScreenNotifier
    else:
        from .mcu.screen import QtScreenNotifier as ScreenNotifier

    if args.sdk and args.apiLevel:
        logger.error("Either SDK version or api level should be specified")
        sys.exit(1)

    if args.apiLevel is None and args.sdk is None:
        default_sdk = {
            "nanos": "2.1",
            "nanox": "2.0.2",
            "blue": "blue-2.2.5",
            "nanosp": "1.0.4"
        }
        args.sdk = default_sdk.get(args.model)

    if args.model == "nanosp" and args.sdk == "1.0.4":
        # NanoS+ 1.0.4 OS can be emulated as 1.0.3 OS
        args.sdk = "1.0.3"

    api_enabled = (args.api_port != 0)

    automation_path: Optional[automation.Automation] = None
    if args.automation:
        # TODO: remove this condition and all associated code in next major version
        logger.warn("--automation is deprecated, please use the REST API instead")
        automation_path = automation.Automation(args.automation)

    automation_server: Optional[BroadcastInterface] = None
    if args.automation_port:
        # TODO: remove this condition and all associated code in next major version
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

    qemu_pid = run_qemu(s1, s2, args, use_bagl)
    s1.close()

    apdu = apdu_server.ApduServer(host="0.0.0.0", port=args.apdu_port)
    seph = seproxyhal.SeProxyHal(
        s2,
        model=args.model,
        use_bagl=use_bagl,
        automation=automation_path,
        automation_server=automation_server,
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
            "stax": 1,
            "flex": 1
        }
        zoom = default_zoom.get(args.model)

    x, y = None, None
    if args.xy:
        x, y = (int(i) for i in args.xy.split('x'))

    apirun: Optional[ApiRunner] = None
    if api_enabled:
        apirun = ApiRunner(args.api_port)

    display_args = DisplayArgs(args.color, args.model, args.ontop, rendering,
                               args.keymap, zoom, x, y, use_bagl)
    server_args = ServerArgs(apdu, apirun, button, finger, seph, vnc)
    screen_notifier = ScreenNotifier(display_args, server_args)

    if apirun is not None:
        assert automation_server is not None
        apirun.start_server_thread(screen_notifier, seph, automation_server)

    try:
        screen_notifier.run()
    except BaseException:
        # Will deal with exception triggered in the ScreenNotifier, including
        # KeyboardInterrupt (if not Qt display, else it will segfault)
        logger.exception("An error occurred")
        logger.critical("Stopping Speculos")
    finally:
        if apirun is not None:
            apirun.stop()

        s2.close()
        _, status = os.waitpid(qemu_pid, 0)
        qemu_exit_status = os.WEXITSTATUS(status)
        sys.exit(qemu_exit_status)
