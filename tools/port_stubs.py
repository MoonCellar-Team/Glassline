#!/usr/bin/env python3

import argparse
import collections
import csv
import json
import re
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NM = os.environ.get("NM") or "nm"
UNDEF = re.compile(r"undefined reference to [`'](.+?)'|undefined symbol: (\S+)")
C_DATA = re.compile(r"^(k[A-Z]\w*|cfstr_\w+|cf_\w+|g_\w+|__CFConstantStringClassReference|\w+Key|\w+Notification)$")

def undefined(cxx, flags, objects):
    with tempfile.TemporaryDirectory() as tmp:
        cmd = [cxx, *objects, *flags, "-o", str(Path(tmp) / "probe"), "-Wl,--no-demangle"]
        r = subprocess.run(cmd, capture_output=True, text=True)
    names = sorted({a or b for a, b in UNDEF.findall(r.stderr)})
    if r.returncode != 0 and not names:
        sys.stderr.write(r.stderr)
        raise SystemExit("port_stubs: компоновка упала не из-за неопределённых символов")
    return names

def demangle(names):
    r = subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True)
    out = r.stdout.splitlines() if r.returncode == 0 else []
    return out if len(out) == len(names) else names

def is_data(mangled, pretty):
    if mangled.startswith(("_ZTV", "_ZTI", "_ZTS", "_ZTT")):
        return True
    if mangled.startswith("_Z"):
        return "(" not in pretty
    return bool(C_DATA.match(mangled))

def c_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

def render(names):
    pretty = demangle(names)
    funcs = [(m, p) for m, p in zip(names, pretty) if not is_data(m, p)]
    data = [(m, p) for m, p in zip(names, pretty) if is_data(m, p)]
    out = ["// Сгенерировано tools/port_stubs.py при сборке — не редактировать.",
           f"// Не реализовано: {len(funcs)} функций, {len(data)} объектов данных.",
           "",
           "#include <stdio.h>",
           "#include <stdlib.h>",
           "",
           "namespace {",
           "[[noreturn]] void missing(const char* name) {",
           '    fprintf(stderr, "medecomp: не реализовано: %s\\n", name);',
           "    abort();",
           "}",
           "}  // namespace",
           "",
           'extern "C" {']
    for i, (m, p) in enumerate(funcs):
        out.append(f"void port_stub_{i}() __asm__({c_string(m)});")
        out.append(f"void port_stub_{i}() {{ missing({c_string(p)}); }}")
    out.append("")
    for i, (m, p) in enumerate(data):
        out.append(f"// {p}")
        out.append(f"alignas(16) char port_data_{i}[256] __asm__({c_string(m)});")
    out.append("}")
    return "\n".join(out) + "\n", len(funcs), len(data)

GROUPS = {
    "data": "Данные игры",
    "decomp": "Декомпиляция: объявлено, но не определено",
    "gl": "OpenGL ES",
    "cg": "CoreGraphics",
    "gfx": "Графика: прочее",
    "al": "OpenAL",
    "audio": "AudioToolbox",
    "audiohelp": "Классы platform/*.m для звука",
    "cf": "CoreFoundation",
    "objc": "Foundation, Objective-C",
    "uikit": "UIKit, MediaPlayer, objc_msgSend (их зовут файлы iOS)",
    "uikit-gfx": "UIKit: шрифты и картинки",
    "darwin": "libc Darwin",
    "platform": "Классы platform/*.m и окна iOS",
    "zlib": "zlib",
    "ea": "Сервисы EA и соцсети",
    "other": "Прочее",
}
BY_CATEGORY = {"ea_online": "ea", "platform": "platform", "game": "decomp", "runtime": "decomp", "m3g": "decomp"}
BY_NAME = [
    ("gl", r"^gl[A-Z]"),
    ("al", r"^alc?[A-Z]"),
    ("cf", r"^(CF|kCF|__CF)"),
    ("cg", r"^(CG|kCG)"),
    ("audio", r"^(Audio|kAudio|ExtAudio)"),
    ("uikit-gfx", r"^(UIFont|UIImage|UIGraphics)"),
    ("uikit", r"^(UI[A-Z]|MP[A-Z]|objc_msgSend|OBJC_CLASS_\$_|NSObject::performSelectorOnMainThread|CA[A-Z]|EAGL|kUI)"),
    ("objc", r"^(NS|objc_|_objc|OBJC_|sel_)"),
    ("darwin", r"^(__srget|sysctl|mach_|OSAtomic)"),
    ("ea", r"^(Mayhem|ea|FBConnection|FBFriend|PendingChallenge|GameCenter|SpywareManager)(::|\b)"),
    ("zlib", r"^(inflate|deflate|crc32|adler32|unz|zip)"),
    ("platform", r"^(\(anonymous namespace\)::)?(HostView|TextBoxDelegate|UITextField)::"),
    ("gfx", r"^hash_table<"),
    ("decomp", r"^(midp|util|nio|m3g)::"),
]
FIRST = [
    ("ea", r"^GameCenter::"),
    ("audiohelp", r"^(AudioQueue\w+|QueueObject|PlayerObject|PlayerIPhone2|AudioLock|MediaPickerImpl)::"),
    ("platform", r"^(CocoaTimer|PhotoPickerImpl|iPhoneAccelerometer|RuntimeAppDelegate|EAGLView|MyView|"
                 r"TextBoxDelegate|UnderlinedLabel|XMLElement|XMLParser)::"),
]
APPLE = re.compile(r"^(CF|kCF|__CF|CG|kCG|Audio|kAudio|NS|UI|kUI|objc_|_objc|OBJC_)")
_CATEGORIES = None

def categories():
    global _CATEGORIES
    if _CATEGORIES is None:
        _CATEGORIES = {}
        with open(ROOT / "symbols" / "functions.csv") as fh:
            for r in csv.DictReader(fh):
                _CATEGORIES.setdefault(r["name"].split("(")[0], r["category"])
    return _CATEGORIES

def plain(pretty):
    return re.sub(r"^(vtable for |typeinfo for |typeinfo name for |VTT for |non-virtual thunk to )", "", pretty)

def group_of(mangled, pretty):
    name = plain(pretty)
    if is_data(mangled, pretty) and not mangled.startswith(("_ZTV", "_ZTI", "_ZTS", "_ZTT")) and not APPLE.match(name):
        return "data"
    for gid, rx in FIRST:
        if re.match(rx, name):
            return gid
    cat = categories().get(name.split("(")[0])
    if cat in BY_CATEGORY:
        return BY_CATEGORY[cat]
    for gid, rx in BY_NAME:
        if re.match(rx, name):
            return gid
    return "other"

def unit_of_object(obj):
    key = "medecomp_game.dir/"
    i = obj.rfind(key)
    rest = obj[i + len(key):] if i >= 0 else obj
    root = str(ROOT).lstrip("/") + "/"
    rel = rest[len(root):] if rest.startswith(root) else "port/" + rest
    return re.sub(r"\.o(bj)?$", "", rel)

def defined_in(objects):
    out = {}
    r = subprocess.run([NM, "--defined-only", "-A", *objects], capture_output=True, text=True)
    for line in r.stdout.splitlines():
        m = re.match(r"^(.+?):[0-9a-fA-F]*\s+[A-Za-z]\s+(\S+)$", line)
        if m:
            out.setdefault(m.group(2), unit_of_object(m.group(1)))
    return out

def references(objects, names):
    want, refs = set(names), collections.defaultdict(set)
    r = subprocess.run([NM, "-u", "-A", *objects], capture_output=True, text=True)
    for line in r.stdout.splitlines():
        m = re.match(r"^(.+?):\s+U\s+(\S+)$", line)
        if m and m.group(2) in want:
            refs[m.group(2)].add(unit_of_object(m.group(1)))
    return refs

def class_units():
    count = collections.defaultdict(collections.Counter)
    with open(ROOT / "symbols" / "functions.csv") as fh:
        for r in csv.DictReader(fh):
            name = r["name"].split("(")[0]
            if "::" in name:
                count[name.rsplit("::", 1)[0]][r["file"]] += 1
    return {k: c.most_common(1)[0][0] for k, c in count.items()}

def status(cxx, flags, objects, missing, bits):
    game = [o for o in objects if unit_of_object(o).startswith("src/")]
    port_objects = [o for o in objects if unit_of_object(o).startswith("port/")]
    needed = set(undefined(cxx, flags, game)) | set(missing)
    names = sorted(needed)
    pretty = dict(zip(names, demangle(names)))
    missing = set(missing)
    refs = references(objects, names)
    owners = class_units()
    provided = defined_in(port_objects)
    layers = {gid: {"id": gid, "title": title, "needed": 0, "missing": 0, "names": [], "refs": {}}
              for gid, title in GROUPS.items()}
    data, units = [], collections.Counter()
    for m in names:
        p = pretty[m]
        lid = group_of(m, p)
        where = provided.get(m, "")
        if lid in ("decomp", "other") and where.startswith("port/src/"):
            layer = where.split("/")[2]
            lid = "replaced-" + layer
            layers.setdefault(lid, {"id": lid, "title": "Свои версии файлов iOS", "needed": 0, "missing": 0,
                                    "names": [], "refs": {}})
        L = layers[lid]
        L["needed"] += 1
        if m in missing:
            L["missing"] += 1
            L["names"].append(p)
            L["refs"][p] = sorted(u[len("src/"):] for u in refs.get(m, ()) if u.startswith("src/"))
            for u in refs.get(m, ()):
                if u.startswith("src/"):
                    units[u[len("src/"):]] += 1
            if lid == "data":
                cls = p.rsplit("::", 1)[0] if "::" in p else None
                data.append({"name": p, "mangled": m, "owner": owners.get(cls) if cls else None,
                             "refs": sorted(u[len("src/"):] for u in refs.get(m, ()) if u.startswith("src/"))})
    return {"bits": bits, "layers": [L for L in layers.values() if L["needed"]],
            "data": data, "units": dict(units)}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--cxx", default="clang++")
    ap.add_argument("--flag", action="append", default=[], help="флаг компоновки (можно несколько)")
    ap.add_argument("--status", help="записать сводку для страницы статуса (JSON)")
    ap.add_argument("objects", nargs="+")
    a = ap.parse_args()
    names = undefined(a.cxx, a.flag, a.objects)
    if a.status and (ROOT / "symbols" / "functions.csv").exists():
        bits = 32 if "-m32" in a.flag else 64
        Path(a.status).parent.mkdir(parents=True, exist_ok=True)
        Path(a.status).write_text(json.dumps(status(a.cxx, a.flag, a.objects, names, bits),
                                             ensure_ascii=False, indent=1))
    text, nf, nd = render(names)
    out = Path(a.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not out.exists() or out.read_text() != text:
        out.write_text(text)
    print(f"port_stubs: заглушек {nf} функций и {nd} объектов данных -> {out}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
