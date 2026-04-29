#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "thal.h"

static const char BASES[4] = {'A', 'C', 'G', 'T'};

/* xorshift32 - deterministic PRNG for reproducible corpora. */
static uint32_t rng_state;
static uint32_t xorshift32() {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static std::string random_seq(int len) {
    std::string s(len, 'A');
    for (int i = 0; i < len; i++) s[i] = BASES[xorshift32() & 3u];
    return s;
}

int main(int argc, char** argv) {
    constexpr int N_PAIRS = 500000;
    constexpr int LEN = 20;
    constexpr uint32_t SEED = 0xDEADBEEFu;
    const char* out_path = (argc > 1) ? argv[1] : "regression.txt";

    /* Deterministic corpus: 20-mer x 20-mer pairs drawn i.i.d. from {A,C,G,T}.
       Random content gives statistical coverage of every Watson-Crick-reachable
       entry in the stack / stackint2 / tstack / tstack2 / dangle / atp tables
       once the corpus is large enough. The fixed seed makes it reproducible so
       the dG checksum at the end can be diffed across builds. */
    std::vector<std::pair<std::string, std::string>> corpus;
    corpus.reserve(N_PAIRS);
    rng_state = SEED;
    for (int i = 0; i < N_PAIRS; i++) {
        std::string s1 = random_seq(LEN);
        std::string s2 = random_seq(LEN);
        corpus.emplace_back(std::move(s1), std::move(s2));
    }

    thal_args a;
    set_thal_default_args(&a);
    a.temp = 310.15;

    double dG_sum = 0.0;
    double dG_min =  1e30;
    double dG_max = -1e30;
    int valid = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (const auto& p : corpus) {
        thal_results r;
        memset(&r, 0, sizeof(r));
        thal(reinterpret_cast<const unsigned char*>(p.first.c_str()),
             reinterpret_cast<const unsigned char*>(p.second.c_str()),
             &a, THL_FAST, &r);
        if (r.temp != THAL_ERROR_SCORE) {
            double dG = r.dh - a.temp * r.ds;
            dG_sum += dG;
            if (dG < dG_min) dG_min = dG;
            if (dG > dG_max) dG_max = dG;
            valid++;
        }
        free(r.sec_struct);
    }
    auto t1 = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    double dG_mean = dG_sum / valid;

    printf("Pairs:        %d (length %d, seed 0x%08X)\n", N_PAIRS, LEN, SEED);
    printf("Valid:        %d\n", valid);
    printf("Total time:   %.4f s\n", elapsed);
    printf("Per pair:     %.2f us\n", 1e6 * elapsed / N_PAIRS);
    printf("dG checksum:  %.6f cal/mol\n", dG_sum);
    printf("dG min:       %.2f cal/mol\n", dG_min);
    printf("dG max:       %.2f cal/mol\n", dG_max);
    printf("dG mean:      %.2f cal/mol\n", dG_mean);

    /* Stable, diff-able regression record. Excludes timing values, which vary
       run-to-run. Includes the corpus parameters so a future run with a
       different seed/size cannot accidentally compare against this baseline. */
    FILE* f = fopen(out_path, "w");
    if (!f) {
        fprintf(stderr, "could not open %s for writing: %s\n",
                out_path, strerror(errno));
        return 1;
    }
    fprintf(f, "seed=0x%08X\n", SEED);
    fprintf(f, "n_pairs=%d\n", N_PAIRS);
    fprintf(f, "len=%d\n", LEN);
    fprintf(f, "valid=%d\n", valid);
    fprintf(f, "dG_checksum=%.6f\n", dG_sum);
    fprintf(f, "dG_min=%.6f\n", dG_min);
    fprintf(f, "dG_max=%.6f\n", dG_max);
    fprintf(f, "dG_mean=%.6f\n", dG_mean);
    fclose(f);
    printf("Wrote regression record: %s\n", out_path);
    return 0;
}
