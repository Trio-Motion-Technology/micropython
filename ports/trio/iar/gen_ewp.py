import os

TEMPLATE_FILE = "uPy_iar.ewp_template"
OUTPUT_FILE = "uPy_iar.ewp"

BASE_PATH_REL = "../../.."

INCLUDE_DIRS = [
    "",
    "extmod",
    "ports/trio",
    "ports/trio/portal",
    "ports/trio/msvc",
    "ports/trio/build-standard",
]

CFILE_SEARCH_DIRS = [
    "py",
    "ports/trio",
    "ports/trio/portal",
    "ports/trio/msvc",
]

CFILE_ADDITIONALS = [
    "extmod/modtime.c",
]


def rel_plat_path(abs_path: str) -> str:
    rel_path = BASE_PATH_REL + "/" + abs_path
    if os.path.sep == "\\":
        return rel_path.replace("/", "\\")
    return rel_path

includes_fmt = "\n".join((
    f"                    <state>$PROJ_DIR$\\{rel_plat_path(i)}</state>" 
    for i in INCLUDE_DIRS
))

c_files = [
    os.path.join(path, f) for path in map(rel_plat_path, CFILE_SEARCH_DIRS) 
        for f in os.listdir(path) if f.endswith('.c') and os.path.isfile(os.path.join(path, f))
]

c_files_fmt = "\n".join((
    f"    <file>\n        <name>$PROJ_DIR$\\{i}</name>\n    </file>" 
    for i in c_files
))

with open(TEMPLATE_FILE, "r") as template:
    template_string = template.read()

final = template_string.replace("$$INCLUDES$$", includes_fmt).replace("$$CFILES$$", c_files_fmt)

with open(OUTPUT_FILE, "w+") as output:
    output.write(final)