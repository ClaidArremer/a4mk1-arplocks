/* 0015-arp-locks - parameter locks for the ARP page (OS 1.56).
 *
 * The ARP page's eight controls (ids 11..18: MOD SPD RNG LEN LEG NO2 NO3 NO4) are
 * group 0, which stock never lets you lock. Every step's lock block has 112 lock
 * words; the sound parameters use 0..100, so 101..108 are free and are what the
 * pattern already saves and loads (the save loop covers 0..0x6f). A held trig
 * writes and shows those slots through arpl_held / arpl_pgv / arpl_clear;  the trig's apply routine
 * takes them out (so the sound engine never sees them) and remembers them per track,
 * and the arpeggiator reads its settings through a per-track copy of the track data
 * with the locked values put in. A trig with no ARP lock goes back to the page's
 * values, as any other lock does.
 *
 * C clobbers d0/d1/a0/a1 only (the stock ABI).
 */
typedef unsigned int u32;
typedef unsigned char u8;
typedef short s16;

#define STATE_UNUSED 0      /* free DDR (0x46803000..0x4680ffff gap), after 0013 */
#define MAGIC      0x4152504cu      /* "ARPL" */
#define PATTERNS_UNUSED 0      /* pattern 0's data; pattern stride 0x16fda */
#define PAT_STRIDE 0x16fdau
#define TRK_SIZE   0x3b1u           /* one track's data in a pattern */
#define SLOT0      101              /* lock slot of ARP id 11 */
#define NLOCK      112

struct st {
    u32 magic;
    s16 val[6][8];                  /* the ARP value each track is locked to, or -1 */
    s16 scratch[6][NLOCK];          /* the lock list apply sees, ARP slots taken out */
    u8  shadow[6][TRK_SIZE];        /* the track data the arpeggiator reads */
};
#define ST ((struct st *)mem)

/* Where ids 11..18 live in a track's data (from the setter switch at 0x4009c0e6). */
static const unsigned short field[8] = { 0x20b, 0x20c, 0x20d, 0x20f, 0x20e, 0x211, 0x212, 0x213 };

static void init(void)
{
    struct st *s = ST;
    if (s->magic != MAGIC) {
        for (int t = 0; t < 6; t++)
            for (int p = 0; p < 8; p++)
                s->val[t][p] = -1;
        s->magic = MAGIC;
    }
}

/* ---------------------------------------------------------------- the UI
 * Stock edits a held trig's locks in a temporary sound (0x3dc-byte objects whose
 * value array only covers the sound parameters), so it cannot carry an ARP lock.
 * These write and read the pattern's lock words for the held steps directly. */
typedef int (*fn)();
#define F(a)        ((fn)(a))
#define NOT_OURS    ((int)0x80000000)
#define PARAMS      0x401699c8u     /* parameter records, 56 B: +8 min, +12 max */

/* The current pattern's data (as the arpeggiator and track note-on find it). */
static u8 *pattern(void)
{
    u32 pat = F(0x400779b6)();
    if (pat > 127)
        pat = 0;
    return (pat_mem + pat * PAT_STRIDE);
}

/* The track's lock words, after the six tracks' data (pattern + 0x1626, as the trig
 * routine passes them to apply): step s, slot i at [s * 0x72 + i]; the step's lock
 * count (a long) at byte 0xe0 of its 0xe4; per-slot "fine" flags at byte 0x3900 + i. */
static s16 *lock_words(u32 track)
{
    return (s16 *)(pattern() + 6 * TRK_SIZE + track * 0x3970u);
}

static int view_track(u8 *view)
{
    return F(0x400aab78)(*(u32 *)(view + 0x74));
}

static int held(u8 *view, u32 *hi, u32 *lo)
{
    u8 *mgr = *(u8 **)(view + 0x6c);
    if (!mgr)
        return 0;
    *hi = *(u32 *)(mgr + 0x158);
    *lo = *(u32 *)(mgr + 0x15c);
    return (*hi | *lo) != 0;
}

static int is_held(u32 hi, u32 lo, int s)
{
    return s < 32 ? (lo >> s) & 1 : (hi >> (s - 32)) & 1;
}

static void recount(s16 *step)
{
    u32 n = 0;
    for (int i = 0; i < NLOCK; i++)
        n += step[i] >= 0;
    *(u32 *)((u8 *)step + 0xe0) = n;
}

/* The encoder's held-trig call (view +0x7c), for an ARP id. delta is already in
 * value units (the handler shifts it by 8 for coarse parameters). No stock calls
 * beyond the two getters above. */
int arpl_held(u8 *view, u32 id, int delta)
{
    u32 hi, lo, p = id - 11, slot = SLOT0 + p;
    int t, lo_v, hi_v, base;
    s16 *tw;

    if (!held(view, &hi, &lo))
        return -1;
    t = view_track(view);
    if (t < 0 || t > 5)
        return -1;
    tw = lock_words(t);
    base = (int)(signed char)pattern()[t * TRK_SIZE + field[p]] << 8;
    lo_v = *(int *)(PARAMS + id * 56 + 8);
    hi_v = *(int *)(PARAMS + id * 56 + 12);
    for (int s = 0; s < 64; s++) {
        if (!is_held(hi, lo, s))
            continue;
        s16 *st = tw + s * 0x72;
        int v = (st[slot] >= 0 ? st[slot] : base) + delta;
        if (v < lo_v) v = lo_v;
        if (v > hi_v) v = hi_v;
        st[slot] = (s16)(v & 0x7f00);
        recount(st);
    }
    *((u8 *)tw + 0x3900 + slot) = 0;
    return 1;
}

/* page_get_value(view, id, x) with x >= 0 (a trig held), for an ARP id: the first
 * held step's lock, or NOT_OURS (stock then shows the page's value). */
int arpl_pgv(u8 *view, u32 id, int x)
{
    u32 hi, lo, slot = SLOT0 + (id - 11);
    int t;
    s16 *tw;

    (void)x;
    if (!held(view, &hi, &lo))
        return NOT_OURS;
    t = view_track(view);
    if (t < 0 || t > 5)
        return NOT_OURS;
    tw = lock_words(t);
    for (int s = 0; s < 64; s++)
        if (is_held(hi, lo, s)) {
            s16 v = tw[s * 0x72 + slot];
            return v >= 0 ? v : NOT_OURS;
        }
    return NOT_OURS;
}

/* The lock-clear routine (0x40023a80) (view, id), for an ARP id. */
int arpl_clear(u8 *view, u32 id)
{
    u32 hi, lo, slot = SLOT0 + (id - 11);
    int t;
    s16 *tw;

    if (!held(view, &hi, &lo))
        return -1;
    t = view_track(view);
    if (t < 0 || t > 5)
        return -1;
    tw = lock_words(t);
    for (int s = 0; s < 64; s++)
        if (is_held(hi, lo, s)) {
            s16 *st = tw + s * 0x72;
            st[slot] = -1;
            recount(st);
        }
    return 1;
}

/* ---------------------------------------------------------------- the engine
 * The trig's apply routine (0x40077ad2) at its entry: (track, out, locks, ...).
 * Returns the lock list it should use. */
s16 *arpl_apply(u32 track, s16 *locks)
{
    struct st *s = ST;
    int any = 0;

    init();
    if (track > 5)
        return locks;
    for (int p = 0; p < 8; p++) {
        s16 v = locks[SLOT0 + p];
        s->val[track][p] = v < 0 ? -1 : (s16)(v >> 8);
        any |= v >= 0;
    }
    if (!any)
        return locks;
    s16 *d = s->scratch[track];
    for (int i = 0; i < NLOCK; i++)
        d[i] = locks[i];
    for (int p = 0; p < 8; p++)
        d[SLOT0 + p] = -1;
    return d;
}

/* The arpeggiator's `adda.l #PATTERNS,An` (five sites): off = pattern * 0x16fda +
 * track * 0x3b1. Returns the address it should read from. */
u8 *arpl_base(u32 off)
{
    struct st *s = ST;
    u8 *real = (pat + off);
    u32 t = (off % PAT_STRIDE) / TRK_SIZE;
    s16 *v;
    int any = 0;

    init();
    if (t > 5)
        return real;
    v = s->val[t];
    for (int p = 0; p < 8; p++)
        any |= v[p] >= 0;
    if (!any)
        return real;
    u8 *d = s->shadow[t];
    /* The arpeggiator reads only these bytes of a track (0x20b..0x213, its
     * settings; 0x2d4.. its note table, indexed): copy them, not all 945. */
    for (u32 i = 0x20b; i < 0x214; i++)
        d[i] = real[i];
    for (u32 i = 0x2d4; i < 0x300; i++)
        d[i] = real[i];
    for (int p = 0; p < 8; p++)
        if (v[p] >= 0)
            d[field[p]] = (u8)v[p];
    return d;
}
