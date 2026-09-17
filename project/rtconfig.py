import os
import shutil

ARCH = 'arm'
CPU = 'cortex-m7'
CROSS_TOOL = os.getenv('RTT_CC', 'gcc')
BSP_LIBRARY_TYPE = None
BUILD = 'debug'

PROJECT_DIR = os.path.abspath(os.path.dirname(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(PROJECT_DIR, '..'))

# RT-Thread Studio/Eclipse display name used by `scons --target=eclipse`.
RTT_STUDIO_PROJECT_NAME = 'ART-PI'

# Used by RT-Thread project generators. Sources remain physically shared with
# the repository instead of being copied into project/.
PROJECT_SOURCE_FOLDERS = [
    '../UserSrc',
    '../BSP',
    '../Libraries/ThirdParty',
    '../Libraries/Vendor',
]

if CROSS_TOOL == 'gcc':
    PLATFORM = 'gcc'
    EXEC_PATH = ''
elif CROSS_TOOL == 'keil':
    PLATFORM = 'armclang'
    EXEC_PATH = os.getenv('KEIL_ROOT', r'D:/Program files (x86)/Keil/Keil_v5')
else:
    raise RuntimeError('Unsupported RTT_CC=%s; ART-PI supports gcc and keil only' % CROSS_TOOL)

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.path.abspath(os.getenv('RTT_EXEC_PATH'))
elif PLATFORM == 'gcc':
    gcc = shutil.which('arm-none-eabi-gcc')
    if gcc:
        EXEC_PATH = os.path.dirname(os.path.abspath(gcc))

if os.name == 'nt' and EXEC_PATH:
    EXEC_PATH = EXEC_PATH.replace('\\', '/')


def _shell_path(path):
    return os.path.abspath(path).replace('\\', '/')


FIRMWARE_TOOL_DIR = _shell_path(os.path.join(PROJECT_ROOT, 'tools', 'firmware'))
VERSION_HEADER_FILE = _shell_path(
    os.path.join(PROJECT_ROOT, 'BSP', 'Board', 'CubeMX_Config', 'Core', 'Inc', 'main.h')
)
FIRMWARE_PREFIX = os.getenv('FIRMWARE_PREFIX', 'ART-PI')
FIRMWARE_MIDDLE = os.getenv('FIRMWARE_MIDDLE', 'app')
FIRMWARE_SUFFIX = os.getenv('FIRMWARE_SUFFIX', '')
BOOT_HEX_FILE = os.getenv('BOOT_HEX_FILE', '')
if BOOT_HEX_FILE:
    BOOT_HEX_FILE = _shell_path(BOOT_HEX_FILE)

VERSION_CMD = (
    '$$(sh "{0}/get_version_from_header.sh" '
    '--header-file "{1}" --macro VERSION)'
).format(FIRMWARE_TOOL_DIR, VERSION_HEADER_FILE)

FIRMWARE_NAMER_BASE_CMD = (
    'sh "{tools}/firmware_namer.sh" '
    '--project-root "{root}" '
    '--prefix "{prefix}" '
    '--middle "{middle}" '
    '--version "{version}" '
    '--suffix "{suffix}"'
).format(
    tools=FIRMWARE_TOOL_DIR,
    root=_shell_path(PROJECT_ROOT),
    prefix=FIRMWARE_PREFIX,
    middle=FIRMWARE_MIDDLE,
    version=VERSION_CMD,
    suffix=FIRMWARE_SUFFIX,
)

FIRMWARE_MERGE_ARGS = ''
if BOOT_HEX_FILE:
    FIRMWARE_MERGE_ARGS = (
        ' --boot-hex "{boot}"'
        ' --merge-hex "{tools}/merge_hex_fast.sh"'
        ' --merged-start-addr first'
    ).format(boot=BOOT_HEX_FILE, tools=FIRMWARE_TOOL_DIR)

if PLATFORM == 'gcc':
    PREFIX = 'arm-none-eabi-'
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'elf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    DEVICE = ' -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections'
    CFLAGS = DEVICE + ' -Dgcc'
    AFLAGS = ' -c' + DEVICE + ' -x assembler-with-cpp -Wa,-mimplicit-it=thumb '
    LFLAGS = DEVICE + ' -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,Reset_Handler -T ../BSP/Board/linker_scripts/STM32H750XBHx/link.lds'
    CPATH = ''
    LPATH = ''

    if BUILD == 'debug':
        CFLAGS += ' -O0 -gdwarf-2 -g'
        AFLAGS += ' -gdwarf-2'
    else:
        CFLAGS += ' -O2'

    CXXFLAGS = CFLAGS
    CFLAGS += ' -std=c99'

    # firmware_namer generates rtthread.hex/rtthread.bin and then copies them to
    # ART-PI_app_<VERSION>[_suffix].{hex,bin}. BOOT_HEX_FILE optionally turns
    # the renamed HEX into a bootloader+application merged image.
    POST_ACTION = (
        FIRMWARE_NAMER_BASE_CMD
        + ' --elf "$TARGET" --objcopy "' + OBJCPY + '"'
        + FIRMWARE_MERGE_ARGS
        + '\n' + SIZE + ' $TARGET\n'
    )

elif PLATFORM == 'armclang':
    CC = 'armclang'
    CXX = 'armclang'
    AS = 'armasm'
    AR = 'armar'
    LINK = 'armlink'
    TARGET_EXT = 'axf'

    DEVICE = ' --cpu Cortex-M7.fp.dp '
    CFLAGS = ' --target=arm-arm-none-eabi -mcpu=cortex-M7 -mfpu=fpv5-d16 '
    CFLAGS += ' -mfloat-abi=hard -c -fno-rtti -funsigned-char -fshort-enums -fshort-wchar '
    CFLAGS += ' -gdwarf-3 -ffunction-sections '
    AFLAGS = DEVICE + ' --apcs=interwork '
    LFLAGS = DEVICE + ' --info sizes --info totals --info unused --info veneers '
    LFLAGS += ' --list rtthread.map '
    LFLAGS += r' --strict --scatter "..\BSP\Board\linker_scripts\STM32H750XBHx\link.sct" '
    CFLAGS += ' -I' + EXEC_PATH + '/ARM/ARMCLANG/include'
    LFLAGS += ' --libpath=' + EXEC_PATH + '/ARM/ARMCLANG/lib'
    EXEC_PATH += '/ARM/ARMCLANG/bin/'

    if BUILD == 'debug':
        CFLAGS += ' -g -O1'
        AFLAGS += ' -g'
    else:
        CFLAGS += ' -O2'

    CXXFLAGS = CFLAGS
    CFLAGS += ' -std=c99'

    POST_ACTION = (
        'fromelf --bin $TARGET --output rtthread.bin\n'
        'fromelf -i32combined $TARGET --output rtthread.hex\n'
        + FIRMWARE_NAMER_BASE_CMD
        + ' --hex "rtthread.hex" --bin "rtthread.bin" --no-objcopy'
        + FIRMWARE_MERGE_ARGS
        + '\nfromelf -z $TARGET\n'
    )
