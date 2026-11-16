        | 0015-arp-locks - the detours and gates. C (arpl.c) does the work.
        | C functions clobber %d0/%d1/%a0/%a1 only (the stock ABI).

        .section .c_arpl_gates,"ax",@progbits
        .align  2

        | ------------------------------------------------------------------
        | The encoder's held-trig call (0x40025a06: `movea.l 0x7c(%a0),%a0 ;
        | jsr (%a0)`, (view, id, delta) pushed, a0 = the view's vtable). ARP ids
        | go to arpl_held; anything else, or nothing held, to stock.
        | ------------------------------------------------------------------
        .globl  arpl_held_gate
arpl_held_gate:
        move.l  8(%sp),%d0
        cmpi.l  #276,%d0
        beq.b   3f
        cmpi.l  #277,%d0
        beq.b   3f
        subi.l  #11,%d0
        moveq   #7,%d1
        cmp.l   %d1,%d0
        bhi.b   9f
        move.l  %a0,-(%sp)
        move.l  16(%sp),-(%sp)
        move.l  16(%sp),-(%sp)
        move.l  16(%sp),-(%sp)
        jsr     arpl_held
        lea     12(%sp),%sp
        movea.l (%sp)+,%a0
        tst.l   %d0
        blt.b   9f
        rts
9:      movea.l 0x7c(%a0),%a0
        jmp     (%a0)
3:      addq.l  #4,%sp                            | a switch: no lock; the
        lea     12(%sp),%sp                       | handler exits
        jmp     0x40025a68

        | page_get_value(view, id, x) (0x400239e2) at entry: an ARP id with a
        | trig held shows the held step's lock. Else the displaced prologue.
        .globl  arpl_pgv_gate
arpl_pgv_gate:
        move.l  8(%sp),%d0
        cmpi.l  #276,%d0
        beq.b   5f
        cmpi.l  #277,%d0
        beq.b   5f
        subi.l  #11,%d0
        moveq   #7,%d1
        cmp.l   %d1,%d0
        bhi.b   8f
        tst.l   12(%sp)
        blt.b   8f
        move.l  12(%sp),-(%sp)
        move.l  12(%sp),-(%sp)
        move.l  12(%sp),-(%sp)
        jsr     arpl_pgv
        lea     12(%sp),%sp
        cmpi.l  #0x80000000,%d0
        beq.b   8f
        rts
8:      lea     -12(%sp),%sp
        movem.l %d2-%d3/%a2,(%sp)
        jmp     0x400239ea
5:      move.l  %d0,-(%sp)                        | a switch: its dial
        jsr     arpl_sw_dial
        addq.l  #4,%sp
        rts

        | The lock-clear routine (0x40023a80) (view, id) at entry: an ARP id
        | with a trig held clears our slot. Else the displaced prologue.
        .globl  arpl_clear_gate
arpl_clear_gate:
        move.l  8(%sp),%d0
        subi.l  #11,%d0
        moveq   #7,%d1
        cmp.l   %d1,%d0
        bhi.b   7f
        move.l  8(%sp),-(%sp)
        move.l  8(%sp),-(%sp)
        jsr     arpl_clear
        addq.l  #8,%sp
        tst.l   %d0
        blt.b   7f
        rts
7:      lea     -0x20(%sp),%sp
        movem.l %d2-%d3/%a2-%a3,(%sp)
        jmp     0x40023a88

        | knob_value(view, id, &a, &locked) (0x40024b42, the page's knob
        | drawer asks it each knob's value and whether it is locked) at entry:
        | an ARP id locked on the held trig gives its lock, marked locked (the
        | inverted knob). Else the displaced prologue.
        .globl  arpl_kv_gate
arpl_kv_gate:
        move.l  8(%sp),%d0
        cmpi.l  #276,%d0
        beq.b   4f
        cmpi.l  #277,%d0
        beq.b   4f
        subi.l  #11,%d0
        moveq   #7,%d1
        cmp.l   %d1,%d0
        bhi.b   6f
        clr.l   -(%sp)
        move.l  12(%sp),-(%sp)
        move.l  12(%sp),-(%sp)
        jsr     arpl_pgv
        lea     12(%sp),%sp
        cmpi.l  #0x80000000,%d0
        beq.b   6f
        movea.l 12(%sp),%a0
        clr.b   (%a0)
        movea.l 16(%sp),%a0
        move.b  #1,(%a0)
        rts
6:      lea     -0x34(%sp),%sp
        movem.l %d2-%d3/%a2-%a5,(%sp)
        jmp     0x40024b4a
4:      movea.l 12(%sp),%a0                       | a switch: never locked
        clr.b   (%a0)
        movea.l 16(%sp),%a0
        clr.b   (%a0)
        move.l  %d0,-(%sp)
        jsr     arpl_sw_dial
        addq.l  #4,%sp
        rts

        | ------------------------------------------------------------------
        | The encoder's param_apply_delta call (0x40025a56: `movea.l
        | 0x58(%a0),%a0 ; jsr (%a0)`, (view, id, delta, FUNC, &flag) pushed):
        | a switch changes here and the handler exits; else stock.
        | ------------------------------------------------------------------
        .globl  arpl_enc_gate
arpl_enc_gate:
        move.l  8(%sp),%d0
        cmpi.l  #276,%d0
        beq.b   1f
        cmpi.l  #277,%d0
        beq.b   1f
        movea.l 0x58(%a0),%a0
        jmp     (%a0)
1:      move.l  12(%sp),-(%sp)
        move.l  %d0,-(%sp)
        jsr     arpl_sw_enc
        addq.l  #8,%sp
        addq.l  #4,%sp
        lea     20(%sp),%sp
        jmp     0x40025a68

        | param_label(set, id, x, out) (0x4009bac4) at entry: the switches'
        | text. Displaced: move.l %d3,-(%sp); move.l %d2,-(%sp);
        | move.l 20(%sp),%d2.
        .globl  arpl_lbl_gate
arpl_lbl_gate:
        move.l  16(%sp),-(%sp)
        move.l  12(%sp),-(%sp)
        jsr     arpl_label
        addq.l  #8,%sp
        tst.l   %d0
        beq.b   1f
        rts
1:      move.l  %d3,-(%sp)
        move.l  %d2,-(%sp)
        move.l  20(%sp),%d2
        jmp     0x4009bacc

        | The short-name getter (set, id) -> d0 (0x4009b7be): the switches'
        | names. Displaced: move.l 8(%sp),%d0; cmpi.l #295,%d0.
        .globl  arpl_sname_gate
arpl_sname_gate:
        move.l  8(%sp),-(%sp)
        jsr     arpl_sname
        addq.l  #4,%sp
        tst.l   %d0
        beq.b   1f
        rts
1:      move.l  8(%sp),%d0
        cmpi.l  #295,%d0
        jmp     0x4009b7c8

        | ------------------------------------------------------------------
        | The knob draw's graphic call (0x400231ec: container +0x90 (cont, id,
        | value, ...)): the switches are drawn as LEGATO (id 15), the ARP page's
        | own on/off switch. Only the graphic; the name and value text keep the
        | switch's own id. Displaced: movea.l (%a2),%a0; move.l %a4,-(%sp);
        | move.l %a3,-(%sp).
        | ------------------------------------------------------------------
        .globl  arpl_draw_gate
arpl_draw_gate:
        movea.l (%a2),%a0
        move.l  %a4,-(%sp)
        move.l  %a3,-(%sp)
        cmpi.l  #276,%d3
        beq.b   1f
        cmpi.l  #277,%d3
        beq.b   1f
        jmp     0x400231f2                        | anything else: stock
1:      move.l  %d2,-(%sp)                        | a switch: stock's call,
        move.l  0x20(%a6),-(%sp)                  | with id 15
        mvz.b   0x24c(%a5),%d0
        move.l  %d0,-(%sp)
        move.l  %d5,-(%sp)
        pea     15.w
        move.l  %a2,-(%sp)
        movea.l 0x90(%a0),%a0
        jsr     (%a0)
        jmp     0x4002320a

        | ------------------------------------------------------------------
        | The lock apply routine (0x40077ad2) at entry, (track, out, locks, x)
        | pushed. arpl_apply notes the ARP slots and may hand back a copy of
        | the list without them. Then the displaced prologue.
        | ------------------------------------------------------------------
        .globl  arpl_apply_gate
arpl_apply_gate:
        move.l  12(%sp),-(%sp)
        move.l  8(%sp),-(%sp)
        jsr     arpl_apply
        addq.l  #8,%sp
        move.l  %d0,12(%sp)
        lea     -0x40(%sp),%sp
        movem.l %d2-%d7/%a2-%a6,(%sp)
        jmp     0x40077ada

        | ------------------------------------------------------------------
        | The arpeggiator's `adda.l #0x4096ff52,%a2` / `%a0` (jsr in its place):
        | the register holds the offset; it comes back as the address to read.
        | Everything else is kept.
        | ------------------------------------------------------------------
        .globl  arpl_base_a2
arpl_base_a2:
        lea     -16(%sp),%sp
        movem.l %d0-%d1/%a0-%a1,(%sp)
        move.l  %a2,-(%sp)
        jsr     arpl_base
        addq.l  #4,%sp
        movea.l %d0,%a2
        movem.l (%sp),%d0-%d1/%a0-%a1
        lea     16(%sp),%sp
        rts

        .globl  arpl_base_a0
arpl_base_a0:
        lea     -12(%sp),%sp
        movem.l %d0-%d1/%a1,(%sp)
        move.l  %a0,-(%sp)
        jsr     arpl_base
        addq.l  #4,%sp
        movea.l %d0,%a0
        movem.l (%sp),%d0-%d1/%a1
        lea     12(%sp),%sp
        rts
