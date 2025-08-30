// test_ms.cpp
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

// -------- Test helpers --------
static inline MagicSquare makeSolved()
{
    MagicSquare ms;
    // Fill each face with distinct letters so strip/reversals are observable
    const char faceChar[6] = {'B', 'D', 'F', 'L', 'R', 'U'}; // back,down,front,left,right,up
    for (int f = 0; f < 6; ++f)
    {
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j)
                ms.magicSquare[f][i][j] = faceChar[f];
    }
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
    // Compact dump: 6 faces line by line
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

#define ASSERT_TRUE(expr, msg)                                       \
    do                                                               \
    {                                                                \
        if (!(expr))                                                 \
        {                                                            \
            printf("[FAIL] %s at %s:%d\n", msg, __FILE__, __LINE__); \
            return false;                                            \
        }                                                            \
    } while (0)

struct Test
{
    const char *name;
    function<bool()> fn;
};

// Apply a move sequence
static void applySeq(MagicSquare &ms, const vector<pair<int, bool>> &seq)
{
    for (auto [m, cw] : seq)
        ms.rotate(m, cw);
}

// Invert a sequence (reverse order, invert direction)
static vector<pair<int, bool>> invertSeq(const vector<pair<int, bool>> &seq)
{
    vector<pair<int, bool>> inv(seq.rbegin(), seq.rend());
    for (auto &p : inv)
        p.second = !p.second;
    return inv;
}

// -------- Individual tests --------
static bool test_do_undo_identity()
{
    MagicSquare base = makeSolved();
    for (int m = 0; m <= 8; ++m)
    {
        MagicSquare ms = base;
        ms.rotate(m, true);
        ms.rotate(m, false);
        ASSERT_TRUE(equalState(ms, base), "do/undo identity failed for move CW then CCW");

        ms = base;
        ms.rotate(m, false);
        ms.rotate(m, true);
        ASSERT_TRUE(equalState(ms, base), "do/undo identity failed for move CCW then CW");
    }
    return true;
}

static bool test_four_quarter_turns_identity()
{
    MagicSquare base = makeSolved();
    for (int m = 0; m <= 8; ++m)
    {
        MagicSquare ms = base;
        for (int k = 0; k < 4; ++k)
            ms.rotate(m, true);
        ASSERT_TRUE(equalState(ms, base), "four CW turns should be identity");
    }
    return true;
}

static bool test_double_turn_equivalence()
{
    MagicSquare base = makeSolved();
    for (int m = 0; m <= 8; ++m)
    {
        MagicSquare a = base, b = base;
        // Two CW quarter-turns vs two CCW quarter-turns should be the same state
        a.rotate(m, true);
        a.rotate(m, true);
        b.rotate(m, false);
        b.rotate(m, false);
        ASSERT_TRUE(equalState(a, b), "two CW equals two CCW (half-turn) failed");
    }
    return true;
}

static bool test_random_scramble_then_inverse_restores()
{
    MagicSquare base = makeSolved();
    std::mt19937 rng(123456); // fixed seed for reproducibility
    std::uniform_int_distribution<int> moveDist(0, 8);
    std::bernoulli_distribution dirDist(0.5);

    for (int trial = 0; trial < 50; ++trial)
    {
        // Build a random sequence (length 20)
        vector<pair<int, bool>> seq;
        seq.reserve(20);
        for (int i = 0; i < 20; ++i)
            seq.push_back({moveDist(rng), dirDist(rng)});

        MagicSquare ms = base;
        applySeq(ms, seq);
        auto inv = invertSeq(seq);
        applySeq(ms, inv);

        if (!equalState(ms, base))
        {
            printf("[FAIL] scramble+inverse did not restore on trial %d\n", trial);
            printf("Final state dump for debugging:\n");
            dumpState(ms);
            printf("Expected solved state:\n");
            dumpState(base);
            return false;
        }
    }
    return true;
}

// Optional: check that doing a move then its inverse on a *non-solved* state restores it (stronger)
static bool test_do_undo_on_mixed_state()
{
    // Make a nontrivial base by applying a fixed short sequence
    MagicSquare base = makeSolved();
    vector<pair<int, bool>> seed = {{0, true}, {3, true}, {5, false}, {8, true}, {2, false}};
    applySeq(base, seed);

    for (int m = 0; m <= 8; ++m)
    {
        MagicSquare ms = base;
        ms.rotate(m, true);
        ms.rotate(m, false);
        ASSERT_TRUE(equalState(ms, base), "do/undo failed on mixed state (CW→CCW)");

        ms = base;
        ms.rotate(m, false);
        ms.rotate(m, true);
        ASSERT_TRUE(equalState(ms, base), "do/undo failed on mixed state (CCW→CW)");
    }
    return true;
}

// -------- Test runner --------
int main()
{
    vector<Test> tests = {
        {"do_undo_identity", test_do_undo_identity},
        {"four_quarter_turns_identity", test_four_quarter_turns_identity},
        {"double_turn_equivalence", test_double_turn_equivalence},
        {"random_scramble_then_inverse", test_random_scramble_then_inverse_restores},
        {"do_undo_on_mixed_state", test_do_undo_on_mixed_state},
    };

    int passed = 0;
    for (auto &t : tests)
    {
        bool ok = false;
        try
        {
            ok = t.fn();
        }
        catch (...)
        {
            ok = false;
        }
        printf("[%s] %s\n", ok ? "PASS" : "FAIL", t.name);
        if (ok)
            ++passed;
    }
    printf("\nSummary: %d/%zu tests passed\n", passed, tests.size());
    return (passed == (int)tests.size()) ? 0 : 1;
}
