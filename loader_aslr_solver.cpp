#include <cstdio>
#include <climits>
#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdbool>
#include <cstring>
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <getopt.h>
#include "Mt19937.h"

std::mutex g_found_seeds_mutex;
std::vector<uint32_t> g_found_seeds;
std::vector<std::thread> g_threads;

struct RngInfo {
    bool initialized, has_sample;
    char id[33];
    uint64_t maxval;
    uint64_t sample;
};

struct ThreadArguments {
    uint64_t low;
    uint64_t high;
};

size_t g_rnginfo_list_lastsample;
std::array<RngInfo, 64> g_rnginfo_list;
std::vector<ThreadArguments> g_thread_args;

void ThreadFunction(const ThreadArguments &args) {
    for (uint64_t tv = args.low; tv < args.high - 1; ++tv) {
        if ((tv & 0xFFFFF) == 0 && args.low == 0) {
            printf("0: %07lx/%07lx\n", tv, args.high);
        }

        Mt19937 mt;
        mt.Seed(tv);

        bool correct = true;
        for (size_t i=0; i<=g_rnginfo_list_lastsample; i++) {
            if (g_rnginfo_list[i].initialized) {
                uint64_t rngout = mt.GetValue(g_rnginfo_list[i].maxval);
                if (g_rnginfo_list[i].has_sample && rngout != g_rnginfo_list[i].sample) {
                    correct = false;
                    break;
                }
            }
        }

        if (correct) {
            std::scoped_lock lk(g_found_seeds_mutex);

            g_found_seeds.push_back(tv);

            printf("!!! Found Seed: 0x%08lx\n", tv);
        }
    }
}

void PrintRngOutput(uint32_t seed) {
    Mt19937 mt;
    mt.Seed(seed);

    printf("RNG output ({id} {value}):\n");

    for (size_t i=0; i<g_rnginfo_list.max_size(); i++) {
        if (g_rnginfo_list[i].initialized) {
            uint64_t rngout = mt.GetValue(g_rnginfo_list[i].maxval);
            printf("%s 0x%lx\n", g_rnginfo_list[i].id, rngout);
        }
    }
}

void usage(void) {
    printf("Usage: loader_aslr_solver [options]\nOptions:\n"
    "  -t --threads Number of threads to use, defaults to 8.\n"
    "  -m --proclistrngmax Path to file containing a list of processes, in the form '{id} {RNG max value}'. This is required.\n"
    "  -s --proclistsamples Path to file containing a list of RNG samples for processes, in the form '{id} {RNG output value}'. Bruteforce is only done if this is specified.\n"
    "  -S --seed Generate RNG output with the specified seed. Only done if --proclistsamples is not specified.\n"
    );
}

int main(int argc, char **argv) {
    FILE *f = nullptr;
    size_t num_threads = 8;
    bool seed_set=false;
    uint32_t seed=0;
    size_t i=0;
    size_t total_rnginfo=0, sample_count=0;
    char *tmpstr = nullptr, *endptr = nullptr;
    char proclistrngmax_path[PATH_MAX] = {};
    char proclistsamples_path[PATH_MAX] = {};
    char linestr[256]={};

    if (argc<3) {
        usage();
        return EXIT_FAILURE;
    }

    while (1) {
        int longindex=0;
        static struct option long_options[] = {
            {"threads", 1, NULL, 't'},
            {"proclistrngmax", 1, NULL, 'm'},
            {"proclistsamples", 1, NULL, 's'},
            {"seed", 1, NULL, 'S'},
            {},
        };

        int c = getopt_long(argc, argv, "t:m:s:S:", long_options, &longindex);
        if (c==-1) break;

        switch (c) {
            case 't':
                num_threads = std::strtoul(optarg, nullptr, 0);
            break;

            case 'm':
                std::strncpy(proclistrngmax_path, optarg, sizeof(proclistrngmax_path)-1);
                proclistrngmax_path[sizeof(proclistrngmax_path)-1] = 0;
            break;

            case 's':
                std::strncpy(proclistsamples_path, optarg, sizeof(proclistsamples_path)-1);
                proclistsamples_path[sizeof(proclistsamples_path)-1] = 0;
            break;

            case 'S':
                seed = std::strtoul(optarg, &endptr, 0);
                if (endptr != optarg) seed_set = true;
            break;

            default:
                usage();
                return EXIT_FAILURE;
        }
    }

    if (proclistrngmax_path[0]==0) {
        printf("proclistrngmax not specified.\n");
        return EXIT_FAILURE;
    }

    f = fopen(proclistrngmax_path, "r");
    if (!f) {
        printf("Failed to open %s\n", proclistrngmax_path);
        return EXIT_FAILURE;
    }

    i = 0;
    while (std::fgets(linestr, sizeof(linestr), f)) {
        tmpstr = std::strtok(linestr, " ");
        if (!tmpstr) continue;

        RngInfo info = {.initialized = true};
        std::strncpy(info.id, tmpstr, sizeof(info.id)-1);
        info.id[sizeof(info.id)-1] = 0;

        tmpstr = strtok(NULL, " ");
        if (!tmpstr) continue;
        info.maxval = std::strtoul(tmpstr, &endptr, 0);
        if (endptr == tmpstr) continue;

        g_rnginfo_list[i] = info;
        i++;
        total_rnginfo++;
        if (i >= g_rnginfo_list.max_size()) break;
    }

    fclose(f);

    if (total_rnginfo && proclistsamples_path[0]) {
        f = fopen(proclistsamples_path, "r");
        if (!f) {
            printf("Failed to open %s\n", proclistsamples_path);
            return EXIT_FAILURE;
        }

        while (std::fgets(linestr, sizeof(linestr), f)) {
            tmpstr = std::strtok(linestr, " ");
            if (!tmpstr) continue;

            bool found=false;
            for (i=0; i<g_rnginfo_list.max_size(); i++) {
                if (g_rnginfo_list[i].initialized && std::strcmp(tmpstr, g_rnginfo_list[i].id)==0) {
                    found = true;
                    break;
                }
            }
            if (!found) continue;

            tmpstr = strtok(NULL, " ");
            if (!tmpstr) continue;
            g_rnginfo_list[i].sample = std::strtoul(tmpstr, &endptr, 0);
            if (endptr == tmpstr) continue;
            g_rnginfo_list[i].has_sample = true;
            sample_count++;
            if (g_rnginfo_list_lastsample < i) g_rnginfo_list_lastsample = i;
        }

        fclose(f);
    }

    printf("total_rnginfo = 0x%lx, sample_count = 0x%lx\n", total_rnginfo, sample_count);
    if (total_rnginfo) {
        printf("Using RngInfo:\n");
        for (i=0; i<g_rnginfo_list.max_size(); i++) {
            if (g_rnginfo_list[i].initialized) {
                printf("[0x%lx]: id = %s, maxval = 0x%lx", i, g_rnginfo_list[i].id, g_rnginfo_list[i].maxval);
                if (g_rnginfo_list[i].has_sample) printf(", sample = 0x%lx", g_rnginfo_list[i].sample);
                printf("\n");
            }
        }
    }
    else {
        printf("No rnginfo found.\n");
        return EXIT_FAILURE;
    }

    if (proclistsamples_path[0]==0) {
        printf("proclistsamples not specified, not bruteforcing.\n");

        if (!seed_set) {
            printf("Valid seed not specified.\n");
            return EXIT_FAILURE;
        }

        PrintRngOutput(seed);

        return 0;
    }

    if (!sample_count) {
        printf("No samples found, can't bruteforce.\n");
        return EXIT_FAILURE;
    }

    printf("Solving with %zu threads\n", num_threads);

    const uint64_t unit = 1 + ((1ull << 32) / num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        g_thread_args.push_back(ThreadArguments{ static_cast<uint32_t>(i * unit), std::min<uint64_t>((i + 1) * unit, (1ull << 32)) });

        g_threads.emplace_back(ThreadFunction, g_thread_args[i]);
    }

    for (size_t i = 0; i < num_threads; ++i) {
        g_threads[i].join();
    }

    printf("All threads completed!\n");

    for (const auto &curseed : g_found_seeds) {
        printf("Found Seed: 0x%08X\n", curseed);
        PrintRngOutput(curseed);
        printf("\n");
    }

    return EXIT_SUCCESS;
}
