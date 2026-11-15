#!/usr/bin/env python3
"""Fit a stub's sections into the free gaps of the registry's pools.

pack.py REPO OBJ OWNER MOD [MOD ...]
  REPO   the AE-analog-four-mods checkout
  OBJ    the assembled stub (.o), for its section sizes
  OWNER  the mod that will own the claims
  MOD    the mods in the build (their claims are taken as used)
Prints the [[cave]] claims (first fit, largest section first) or the shortfall.
"""
import subprocess
import sys
import tomllib


def sections(obj):
    out = subprocess.run(["m68k-elf-objdump", "-h", obj], capture_output=True, text=True, check=True).stdout
    secs = []
    for line in out.splitlines():
        p = line.split()
        if len(p) >= 7 and p[0].isdigit() and p[1].startswith("."):
            n = int(p[2], 16)
            if n and p[1] not in (".text", ".data", ".bss"):
                secs.append((p[1], n))
    return secs


def gaps(repo, mods):
    with open(f"{repo}/registry/allocations.toml", "rb") as fh:
        reg = tomllib.load(fh)
    out = []
    for pool in reg["pool"]:
        a, e = pool["start"], pool["end"]
        claims = sorted((c for c in reg["cave"] if a <= c["addr"] < e and c["owner"] in mods),
                        key=lambda c: c["addr"])
        pos = a
        for c in claims:
            if c["addr"] > pos:
                out.append([pool["name"], pos, c["addr"] - pos])
            pos = max(pos, c["addr"] + c["size"])
        if e > pos:
            out.append([pool["name"], pos, e - pos])
    return out


def main():
    repo, obj, owner, *mods = sys.argv[1:]
    secs = sorted(sections(obj), key=lambda s: -s[1])
    free = gaps(repo, set(mods))
    for g in free:                                  # claims start on a long
        pad = (-g[1]) % 4
        g[1] += pad
        g[2] -= pad
    placed, short = [], []
    for name, n in secs:
        size = (n + 3) & ~3
        g = next((g for g in sorted(free, key=lambda g: g[2]) if g[2] >= size), None)
        if g is None:
            short.append((name, n))
            continue
        placed.append((name, g[0], g[1], size))
        g[1] += size
        g[2] -= size
    if short:
        print("DOES NOT FIT:", short, "free left:", sorted((g[2] for g in free), reverse=True)[:8])
        sys.exit(1)
    for name, pool, addr, size in sorted(placed, key=lambda p: p[2]):
        print(f'[[cave]]\nowner   = "{owner}"\naddr    = 0x{addr:08x}\nsize    = 0x{size:x}\n'
              f'section = "{name}"\nnote    = "{name} ({pool})."\n')


if __name__ == "__main__":
    main()
