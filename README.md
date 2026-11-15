# a4mk1-arplocks — ARP page p-locks for the Elektron Analog Four MK1

**For the Analog Four MK1 only, OS 1.56.** Not for the Analog Four MKII, Analog Keys or
any other Elektron unit.

A firmware mod that lets you parameter-lock the ARP page, per trig. It is a mod for
[hugz202's AE-analog-four-mods](https://github.com/hugz202/AE-analog-four-mods) build
framework (mod id `0015-arp-locks`): you build your own patched OS from your own copy of
the stock OS with that framework. No Elektron OS files are included here.

## What it does

- **ARP locks:** hold a trig and turn any ARP page control — MODE, SPEED, RANGE, N.LEN,
  LEGATO, NOTE 2, NOTE 3, NOTE 4 — to lock it on that trig, like any sound parameter.
  - The locked knob shows inverted with the locked value while the trig is held.
  - When the trig plays, that track's arpeggiator uses the locked settings until the
    next trig; a trig without ARP locks goes back to the page's values.
  - Locks are saved with the pattern and clear the stock way.
- **Two switches on the ARP page's empty knobs**, saved with the kit:
  - **I — SLD:** per-track 303 slide for `0002-slide-303` (ON: put the slide trig on the
    note you glide *from*; OFF: stock slide). Synth tracks only.
  - **J — MPE:** MPE on/off for [a4mk1-mpe](https://github.com/ClaidArremer/a4mk1-mpe).

## How it works (short)

The A4 has 112 lock slots per step; the sound parameters use 0–100. ARP values go in
slots 101–108, which the pattern already saves and loads. Held-trig edits are written
straight into the pattern's lock storage (stock edits locks in a temporary sound that has
no room for ARP values). When a trig plays, the ARP slots are taken out before the sound
engine sees them, and the arpeggiator reads its settings through a small per-track copy
with the locked values in. Details are in the comments of `arpl.c`, `hook.s` and
`registry/0015-arp-locks.toml`.

## Building

1. Get [AE-analog-four-mods](https://github.com/hugz202/AE-analog-four-mods) and follow
   its BUILD.md (you supply the stock OS 1.56 yourself).
2. Copy `mods/0015-arp-locks` into its `mods/` folder.
3. Append `registry/0015-arp-locks.toml` to its `registry/allocations.toml`.
4. Build, e.g. with MPE and slide:

   ```
   make custom MODS="0001-scale-lock 0002-slide-303 0006-fx-slot 0011-rand-track 0013-mpe 0015-arp-locks" TAG=arplocks
   ```

   - Include `0006-fx-slot` so the SLD / MPE switches are saved with the kit.
   - Build `0002-slide-303` with its default `always_on = 0` so SLD works.
   - 0015 cannot share an image with `0007-digital-path`, `0008-pages`,
     `0010-wave-osc` or `0012-res-limit` (shared code space and hook points).
   - The code placement in the registry block was made for the mod set above; with a
     different set, re-run `mods/0015-arp-locks/pack.py` to place it.

`gen.sh` regenerates `stub.s` from `arpl.c` and `hook.s` (needs an m68k gcc);
`test_host.c` runs the engine-side logic on a computer.

## Status

Hardware-verified on an Analog Four MK1, OS 1.56. Use at your own risk: back up your
projects before flashing a modded OS. The bootloader is never touched, so you can always
reflash the stock OS.

## Credits

Built on hugz202 / Artificial Energy's AE-analog-four-mods framework (MIT).
MIT licence.
