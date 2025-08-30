// test_ms2.cpp
#include <bits/stdc++.h>
using namespace std;

// --- Expose internals & avoid main collision just for tests ---
#define private public
#define protected public
#define main dont_use_main_in_ms
#include "ms.cpp"
#undef main
#undef private
#undef protected

static inline MagicSquare makeSolved()
{
    MagicSquare ms;
    const char faceChar[6] = {'B', 'D', 'F', 'L', 'R', 'U'}; // back,down,front,left,right,up
    for (int f = 0; f < 6; ++f)
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                ms.magicSquare[f][i][j] = faceChar[f];
    return ms;
}
static inline bool equalState(const MagicSquare &a, const MagicSquare &b)
{
    for (int f = 0; f < 6; ++f)
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                if (a.magicSquare[f][i][j] != b.magicSquare[f][i][j])
                    return false;
    return true;
}
static void dumpState(const MagicSquare &ms)
{
    const char *names[6] = {"Back", "Down", "Front", "Left", "Right", "Up"};
    for (int f = 0; f < 6; ++f)
    {
        printf("%s:\n", names[f]);
        for (int i = 0; i < SIZE; ++i)
        {
            for (int j = 0; j < SIZE; ++j)
                putchar(ms.magicSquare[f][i][j]);
            putchar('\n');
        }
    }
}
static void applySeq(MagicSquare &ms, const vector<pair<int, bool>> &seq)
{
    for (auto [m, cw] : seq)
        ms.rotate(m, cw);
}
static vector<pair<int, bool>> invertSeq(const vector<pair<int, bool>> &seq)
{
    vector<pair<int, bool>> inv(seq.rbegin(), seq.rend());
    for (auto &p : inv)
        p.second = !p.second;
    return inv;
}

// Pretty-print a sequence like: 0+,3-,5+,8-
static void printSeq(const vector<pair<int, bool>> &seq, const char *prefix = "Seq")
{
    printf("%s: ", prefix);
    for (auto [m, cw] : seq)
        printf("%d%s ", m, cw ? "+" : "-");
    putchar('\n');
}

// Build a scrambled base
static MagicSquare makeScrambled(unsigned seed, int len = 20)
{
    MagicSquare base = makeSolved();
    mt19937 rng(seed);
    uniform_int_distribution<int> md(0, 8);
    bernoulli_distribution bd(0.5);
    for (int i = 0; i < len; ++i)
        base.rotate(md(rng), bd(rng));
    return base;
}

// 1) Identify which move fails do/undo on mixed bases
static void check_do_undo_on_mixed()
{
    puts("== Per-move do/undo on mixed bases ==");
    bool all_ok = true;
    for (int m = 0; m <= 8; ++m)
    {
        bool ok = true;
        for (unsigned seed = 1; seed <= 50; ++seed)
        {
            MagicSquare base = makeScrambled(1000u + seed, 15);
            MagicSquare ms1 = base;
            ms1.rotate(m, true);
            ms1.rotate(m, false);
            MagicSquare ms2 = base;
            ms2.rotate(m, false);
            ms2.rotate(m, true);
            if (!equalState(ms1, base) || !equalState(ms2, base))
            {
                ok = false;
                break;
            }
        }
        printf("[%-2d] %s\n", m, ok ? "PASS" : "FAIL");
        if (!ok)
            all_ok = false;
    }
    if (!all_ok)
        puts("Some moves break do/undo on mixed states. See counterexamples below.");
}

// 2) Find a *short* counterexample scramble where do/undo fails for a given move
static bool find_short_counterexample_for_move(int badMove, vector<pair<int, bool>> &out_scramble, bool &cw_then_ccw)
{
    // Progressive deepening over scramble length
    for (int len = 1; len <= 12; ++len)
    {
        mt19937 rng(12345 + len);
        uniform_int_distribution<int> md(0, 8);
        bernoulli_distribution bd(0.5);
        const int tries = 4000;
        for (int t = 0; t < tries; ++t)
        {
            vector<pair<int, bool>> scramble;
            scramble.reserve(len);
            for (int i = 0; i < len; ++i)
                scramble.push_back({md(rng), bd(rng)});
            MagicSquare base = makeSolved();
            applySeq(base, scramble);

            // Try CW then CCW
            {
                MagicSquare ms = base;
                ms.rotate(badMove, true);
                ms.rotate(badMove, false);
                if (!equalState(ms, base))
                {
                    out_scramble = scramble;
                    cw_then_ccw = true;
                    return true;
                }
            }
            // Try CCW then CW
            {
                MagicSquare ms = base;
                ms.rotate(badMove, false);
                ms.rotate(badMove, true);
                if (!equalState(ms, base))
                {
                    out_scramble = scramble;
                    cw_then_ccw = false;
                    return true;
                }
            }
        }
    }
    return false;
}

// 3) Global scramble+inverse validator (as before), but when it fails, shrink the sequence
static void shrink_and_show_counterexample()
{
    puts("\n== Scramble + inverse check with shrinking ==");
    MagicSquare solved = makeSolved();
    mt19937 rng(777);
    uniform_int_distribution<int> md(0, 8);
    bernoulli_distribution bd(0.5);

    for (int trial = 0; trial < 50; ++trial)
    {
        vector<pair<int, bool>> seq(20);
        for (auto &p : seq)
            p = {md(rng), bd(rng)};

        MagicSquare ms = solved;
        applySeq(ms, seq);
        auto inv = invertSeq(seq);
        applySeq(ms, inv);

        if (!equalState(ms, solved))
        {
            puts("[FAIL] scramble+inverse did not restore. Attempting to minimize sequence…");
            // Delta-debugging: try removing single moves greedily
            vector<pair<int, bool>> cur = seq;
            bool changed = true;
            while (changed && cur.size() > 1)
            {
                changed = false;
                for (size_t i = 0; i < cur.size(); ++i)
                {
                    vector<pair<int, bool>> test = cur;
                    test.erase(test.begin() + i);
                    MagicSquare tmp = solved;
                    applySeq(tmp, test);
                    auto inv2 = invertSeq(test);
                    applySeq(tmp, inv2);
                    if (!equalState(tmp, solved))
                    {
                        cur.swap(test);
                        changed = true;
                        break;
                    }
                }
            }
            printSeq(cur, "Minimal counterexample");
            MagicSquare tmp = solved;
            applySeq(tmp, cur);
            auto inv2 = invertSeq(cur);
            applySeq(tmp, inv2);
            puts("Final state (should be solved but is not):");
            dumpState(tmp);
            return;
        }
    }
    puts("[PASS] 50 trials: scramble+inverse always restored.");
}

int main()
{
    // Quick smoke identical to your prior passing tests
    {
        MagicSquare s = makeSolved();
        bool ok = true;
        for (int m = 0; m <= 8; ++m)
        {
            MagicSquare a = s;
            a.rotate(m, true);
            a.rotate(m, false);
            ok &= equalState(a, s);
        }
        printf("Solved do/undo: %s\n", ok ? "PASS" : "FAIL");
    }
    // Pinpoint per-move failures on mixed bases
    check_do_undo_on_mixed();

    // Try to find a short reproducible counterexample for each failing move
    puts("\n== Per-move minimal counterexamples ==");
    for (int m = 0; m <= 8; ++m)
    {
        vector<pair<int, bool>> scr;
        bool cw_ccw = true;
        if (find_short_counterexample_for_move(m, scr, cw_ccw))
        {
            printf("Move %d fails on mixed base. Order: %s\n", m, cw_ccw ? "CW→CCW" : "CCW→CW");
            printSeq(scr, "Scramble");
        }
        else
        {
            printf("Move %d: no short counterexample found (<=12 moves)\n", m);
        }
    }

    // Also provide one global minimized counterexample for the whole system
    shrink_and_show_counterexample();
    return 0;
}
