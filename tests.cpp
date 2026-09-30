#include "tests.h"
#include "mylib.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using clk = std::chrono::steady_clock;

namespace {

constexpr uint64_t TEST_SEED = 20260930ULL;
const std::string DIR = "tests"; // paruošti testiniai failai (Exp. 1-3) ir konstitucija.txt (Exp. 4)
const std::string CONSTITUTION = DIR + "/konstitucija.txt";

// Abėcėlė: spausdinami ASCII simboliai 32..126 (95 simboliai, 1 simbolis = 1 baitas)
const std::string ALPHABET = [] {
    std::string s;
    for (int c = 32; c <= 126; ++c) s += static_cast<char>(c);
    return s;
}();

struct Case { std::string name; std::string data; };

// ---------- pagalbinės funkcijos ----------

// Deterministinė įvestis pagal (ilgis, indeksas) - leidžia atkurti bet kurią įvestį nesaugant jų atmintyje
std::string genInput(size_t len, uint64_t idx) {
    std::seed_seq seq{ uint32_t(TEST_SEED), uint32_t(TEST_SEED >> 32),
                       uint32_t(len), uint32_t(idx), uint32_t(idx >> 32) };
    std::mt19937_64 g(seq);
    std::string s(len, ' ');
    for (char& c : s) c = ALPHABET[g() % ALPHABET.size()];
    return s;
}

// Pora skirtingų eilučių (.first != .second visada)
std::pair<std::string, std::string> makePair(size_t len, uint64_t i) {
    std::string s = genInput(len, 2 * i);
    std::string t = genInput(len, 2 * i + 1);
    if (s == t) t[0] = (t[0] == 'x') ? 'y' : 'x';
    return {s, t};
}

int nib(char c) { return c <= '9' ? c - '0' : c - 'a' + 10; }
int popc(int x) { int n = 0; while (x) { n += x & 1; x >>= 1; } return n; }

bool isHex64(const std::string& h) {
    if (h.size() != 64) return false;
    for (char c : h) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

// Bitų skirtumas (%) po hex dekodavimo
double bitDiffPct(const std::string& h1, const std::string& h2) {
    int d = 0;
    for (size_t i = 0; i < h1.size(); ++i) d += popc(nib(h1[i]) ^ nib(h2[i]));
    return 100.0 * d / (h1.size() * 4);
}
// Hex skirtumas (%) - skirtingų hex skaitmenų pozicijų dalis
double hexDiffPct(const std::string& h1, const std::string& h2) {
    int d = 0;
    for (size_t i = 0; i < h1.size(); ++i) d += (h1[i] != h2[i]);
    return 100.0 * d / h1.size();
}

struct Stats {
    double mn = 1e300, mx = -1e300, sum = 0; size_t n = 0;
    void add(double v) { mn = std::min(mn, v); mx = std::max(mx, v); sum += v; ++n; }
    double mean() const { return n ? sum / n : 0; }
};

size_t utf8Chars(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
    return n;
}

void writeFile(const std::string& path, const std::string& data) {
    std::ofstream f(path, std::ios::binary);
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
}

volatile uint64_t sink = 0; // kad kompiliatorius nepašalintų skaičiavimų

// ---------- 1 eksperimentas: įvestys (paruošti failai aplanke tests/) ----------
std::vector<Case> loadCases() {
    if (!fs::is_directory(DIR))
        throw std::runtime_error("Nerastas aplankas \"" + DIR + "/\" su testiniais failais");
    std::vector<fs::path> files;
    for (const auto& e : fs::directory_iterator(DIR))
        if (e.is_regular_file() && e.path().filename() != "konstitucija.txt") files.push_back(e.path()); // konstitucija.txt - tik 4 eksperimentui
    std::sort(files.begin(), files.end());
    if (files.empty())
        throw std::runtime_error("Aplankas \"" + DIR + "/\" tuščias");
    std::vector<Case> c;
    for (const auto& p : files)
        c.push_back({p.filename().string(), readFileBytes(p.string())});
    return c;
}

const Case* findCase(const std::vector<Case>& cases, const std::string& name) {
    for (const Case& k : cases) if (k.name == name) return &k;
    return nullptr;
}

void runExp1(const std::vector<Case>& cases) {
    std::cout << "\n=== 1 eksperimentas: įvestys (failai iš " << DIR << "/) ===\n";
    std::cout << std::left << std::setw(34) << "failas" << std::setw(10) << "baitai" << "simboliai(UTF-8)\n";
    for (const Case& k : cases)
        std::cout << std::setw(34) << k.name << std::setw(10) << k.data.size() << utf8Chars(k.data) << '\n';

    // patikra: *_mod_* failai nuo pradinio skiriasi tiksliai vienu baitu
    std::cout << "\nVieno baito pakeitimų patikra:\n";
    for (const Case& k : cases) {
        size_t p = k.name.find("_mod_");
        if (p == std::string::npos) continue;
        const Case* base = findCase(cases, k.name.substr(0, p) + ".txt");
        if (!base || base->data.size() != k.data.size()) { std::cout << "  " << k.name << ": nerastas pradinis failas\n"; continue; }
        size_t diff = 0;
        for (size_t i = 0; i < k.data.size(); ++i) diff += (k.data[i] != base->data[i]);
        std::cout << "  " << std::setw(32) << k.name << " skirtingų baitų: " << diff << (diff == 1 ? "  OK" : "  KLAIDA") << '\n';
    }
}

// ---------- 2 eksperimentas: formatas ----------
void runExp2(const std::vector<Case>& cases) {
    std::cout << "\n=== 2 eksperimentas: išvesties formatas ===\n";
    std::cout << std::left << std::setw(34) << "failas" << std::setw(8) << "ilgis" << std::setw(8) << "hex" << "ranka==failas\n";
    int bad = 0;
    for (const Case& k : cases) {
        std::string h = hash(k.data);
        bool ok = isHex64(h);
        // "ranka": readFile() eilutės (be \n) - palyginama tik kai turinyje nėra eilučių pabaigų
        std::string cmp;
        if (k.data.find_first_of("\r\n") == std::string::npos) {
            std::vector<std::string> lines = readFile(DIR + "/" + k.name);
            std::string manual = lines.empty() ? "" : lines[0];
            bool same = (hash(manual) == h);
            cmp = same ? "OK" : "FAIL";
            bad += !same;
        } else cmp = "n/a (yra eilučių pabaigų)";
        bad += !ok;
        std::cout << std::setw(34) << k.name << std::setw(8) << h.size() << std::setw(8) << (ok ? "OK" : "FAIL") << cmp << '\n';
    }
    std::cout << (bad ? "YRA KLAIDŲ: " + std::to_string(bad) : std::string("Visi testai praėjo")) << '\n';
}

// ---------- 3 eksperimentas: determinizmas ----------
void runExp3(const std::vector<Case>& cases) {
    std::cout << "\n=== 3 eksperimentas: determinizmas (A, B, A) ===\n";
    int bad = 0;
    std::ostringstream log;
    for (size_t i = 0; i < cases.size(); ++i) {
        const Case& A = cases[i];
        const Case& B = cases[(i + 1) % cases.size()];
        std::string h1 = hash(A.data);
        std::string hb = hash(B.data);
        std::string h2 = hash(A.data);
        (void)hb;
        if (h1 != h2) { ++bad; std::cout << "NEATITIKIMAS: " << A.name << '\n'; }
        log << A.name << ' ' << h1 << '\n';
    }
    writeFile("determinism_run.txt", log.str());
    std::cout << (bad ? "Yra neatitikimų" : "Kvietimų A,B,A neatitikimų nėra") << '\n'
              << "Hash sąrašas įrašytas į determinism_run.txt. Paleisk programą dar kartą ir palygink su ankstesniu failu (pvz. diff).\n";
}

// ---------- 4 eksperimentas: sparta ----------
double timeNsPerCall(const std::string& s, int reps) {
    auto t0 = clk::now();
    for (int i = 0; i < reps; ++i) {
        std::string h = hash(s);
        sink = sink + static_cast<unsigned char>(h[0]);
    }
    auto t1 = clk::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / reps;
}

void runExp4() {
    const std::string& path = CONSTITUTION;
    std::cout << "\n=== 4 eksperimentas: sparta (" << path << ") ===\n";
    std::string all;
    try { all = readFileBytes(path); }
    catch (const std::exception& e) {
        std::cout << "Praleista: " << e.what() << "\nPadėk konstitucija.txt į aplanką " + DIR + "/\n";
        return;
    }
    // eilučių pabaigų pozicijos (skirtukai išsaugomi - ištraukos yra tikslūs failo prefiksai)
    std::vector<size_t> ends;
    for (size_t i = 0; i < all.size(); ++i) if (all[i] == '\n') ends.push_back(i + 1);
    if (ends.empty() || ends.back() != all.size()) ends.push_back(all.size());
    size_t totalLines = ends.size();

    std::vector<size_t> counts;
    for (size_t k = 1; k < totalLines; k *= 2) counts.push_back(k);
    counts.push_back(totalLines);

    constexpr int WARMUP = 3, RUNS = 7;
    std::ofstream csv("efficiency.csv");
    csv << "lines,bytes,mean_ns,min_ns,max_ns,reps_per_run\n";
    std::cout << std::left << std::setw(8) << "eilutes" << std::setw(10) << "baitai"
              << std::setw(14) << "vid. ns" << std::setw(14) << "min ns" << "max ns\n";
    for (size_t k : counts) {
        std::string s = all.substr(0, ends[k - 1]); // paruošta PRIEŠ matuojant
        int reps = static_cast<int>(std::max<size_t>(1, (size_t(1) << 22) / std::max<size_t>(1, s.size())));
        for (int w = 0; w < WARMUP; ++w) timeNsPerCall(s, reps);
        Stats st;
        for (int r = 0; r < RUNS; ++r) st.add(timeNsPerCall(s, reps));
        csv << k << ',' << s.size() << ',' << st.mean() << ',' << st.mn << ',' << st.mx << ',' << reps << '\n';
        std::cout << std::setw(8) << k << std::setw(10) << s.size() << std::fixed << std::setprecision(0)
                  << std::setw(14) << st.mean() << std::setw(14) << st.mn << st.mx << '\n';
    }
    std::cout << "Matavimai: std::chrono::steady_clock, ns / vieną hash() kvietimą, " << WARMUP
              << " apšilimo ir " << RUNS << " matavimų. Išsaugota efficiency.csv\n";
}

// ---------- 5 eksperimentas: kolizijos ----------
void runExp5(size_t pairs) {
    std::cout << "\n=== 5 eksperimentas: kolizijos (" << pairs << " porų kiekvienam ilgiui) ===\n";
    for (size_t len : {size_t(10), size_t(100), size_t(500), size_t(1000)}) {
        size_t pairCollisions = 0;
        std::unordered_map<std::string, uint64_t> seen; // hash -> pirmos įvesties id (2i arba 2i+1)
        seen.reserve(pairs * 2);
        size_t groups = 0;
        std::vector<std::pair<std::string, std::string>> examples;
        std::unordered_map<std::string, bool> groupCounted;

        for (uint64_t i = 0; i < pairs; ++i) {
            auto [s, t] = makePair(len, i);
            std::string hs = hash(s), ht = hash(t);
            if (hs == ht) ++pairCollisions;
            for (int w = 0; w < 2; ++w) {
                const std::string& h = w ? ht : hs;
                uint64_t id = 2 * i + w;
                auto it = seen.find(h);
                if (it == seen.end()) { seen.emplace(h, id); continue; }
                auto p0 = makePair(len, it->second / 2);
                const std::string& other = (it->second % 2) ? p0.second : p0.first;
                const std::string& cur = w ? t : s;
                if (other == cur) continue; // ta pati įvestis, ne kolizija
                if (!groupCounted[h]) { groupCounted[h] = true; ++groups; }
                if (examples.size() < 3) examples.push_back({other, cur});
            }
        }
        std::cout << "ilgis " << std::setw(5) << len << ": porų kolizijų " << pairCollisions << "/" << pairs
                  << ", skirtingų maišų " << seen.size() << " iš " << 2 * pairs
                  << " įvesčių, kolizinių grupių " << groups << '\n';
        for (auto& e : examples)
            std::cout << "   pavyzdys: \"" << e.first.substr(0, 40) << "\" ir \"" << e.second.substr(0, 40) << "\"\n";
    }

    // struktūruoti atvejai
    std::cout << "\nStruktūruoti atvejai:\n";
    {   // visos 'abcdef' permutacijos
        std::string p = "abcdef";
        std::unordered_map<std::string, int> m; int total = 0;
        do { ++total; m[hash(p)]++; } while (std::next_permutation(p.begin(), p.end()));
        std::cout << "  'abcdef' permutacijos: " << total << " skirtingų įvesčių -> " << m.size() << " skirtingų maišų\n";
    }
    std::cout << "  \"ab\" vs \"ba\": " << (hash("ab") == hash("ba") ? "KOLIZIJA" : "skiriasi") << '\n';
    std::cout << "  \"a\\0\" vs \"b\\0\": " << (hash(std::string("a\0", 2)) == hash(std::string("b\0", 2)) ? "KOLIZIJA" : "skiriasi") << '\n';
    std::cout << "  \"\" vs \"\\0\": " << (hash("") == hash(std::string(1, '\0')) ? "KOLIZIJA" : "skiriasi") << '\n';
    std::cout << "  \"aa\" vs \"aaa\": " << (hash("aa") == hash("aaa") ? "KOLIZIJA" : "skiriasi") << '\n';
}

// ---------- 6 eksperimentas: lavinos efektas ----------
void runExp6(size_t totalPairs) {
    std::cout << "\n=== 6 eksperimentas: lavinos efektas (" << totalPairs << " porų) ===\n";
    std::mt19937_64 g(TEST_SEED + 6);
    const size_t lens[4] = {10, 100, 500, 1000};
    size_t per = totalPairs / 4;
    Stats bitAll, hexAll;
    std::vector<size_t> hist(101, 0); // 1 procentinio punkto dėtuvės
    std::ofstream raw("avalanche_raw.csv");
    raw << "len,bit_diff_pct,hex_diff_pct\n";
    std::cout << std::left << std::setw(7) << "ilgis" << std::setw(28) << "bitai % (min/max/vid)" << "hex % (min/max/vid)\n";
    for (size_t len : lens) {
        Stats bs, hs;
        for (size_t i = 0; i < per; ++i) {
            std::string s = genInput(len, i + 1000000);
            std::string t = s;
            size_t pos = g() % len;
            char c;
            do { c = ALPHABET[g() % ALPHABET.size()]; } while (c == s[pos]);
            t[pos] = c;
            std::string h1 = hash(s), h2 = hash(t);
            double b = bitDiffPct(h1, h2), h = hexDiffPct(h1, h2);
            bs.add(b); hs.add(h); bitAll.add(b); hexAll.add(h);
            ++hist[static_cast<size_t>(b)];
            raw << len << ',' << b << ',' << h << '\n';
        }
        std::ostringstream a, b;
        a << std::fixed << std::setprecision(2) << bs.mn << " / " << bs.mx << " / " << bs.mean();
        b << std::fixed << std::setprecision(2) << hs.mn << " / " << hs.mx << " / " << hs.mean();
        std::cout << std::setw(7) << len << std::setw(28) << a.str() << b.str() << '\n';
    }
    std::cout << std::fixed << std::setprecision(2)
              << "Iš viso: bitai " << bitAll.mn << " / " << bitAll.mx << " / " << bitAll.mean()
              << " (orientyras ~50), hex " << hexAll.mn << " / " << hexAll.mx << " / " << hexAll.mean()
              << " (orientyras ~93.75)\n";
    std::ofstream hf("avalanche_hist.csv");
    hf << "bit_diff_pct_bin,count\n";
    for (size_t i = 0; i < hist.size(); ++i) hf << i << ',' << hist[i] << '\n';
    std::cout << "Išsaugota avalanche_raw.csv ir avalanche_hist.csv (histogramai braižomi iš jų)\n";
}

// ---------- 7 eksperimentas: spėjimas, druska ----------
std::string cand(int i) {
    std::ostringstream os;
    os << std::setw(4) << std::setfill('0') << i;
    return os.str();
}

void runExp7() {
    std::cout << "\n=== 7 eksperimentas: spėjimas ir druska ===\n";
    std::mt19937_64 g(TEST_SEED + 7);
    int target = static_cast<int>(g() % 10000);
    std::string tHash = hash(cand(target));

    auto scan = [&](const std::string& targetHash, const std::string& salt, const char* label) {
        auto t0 = clk::now();
        std::vector<int> matches;
        for (int i = 0; i < 10000; ++i)
            if (hash(cand(i) + salt) == targetHash) matches.push_back(i);
        double ms = std::chrono::duration<double, std::milli>(clk::now() - t0).count();
        std::cout << label << ": bandymų 10000, laikas " << std::fixed << std::setprecision(2) << ms
                  << " ms, sutampančių kandidatų " << matches.size() << " [";
        for (int m : matches) std::cout << cand(m) << ' ';
        std::cout << "]\n";
    };

    scan(tHash, "", "be druskos      ");
    std::cout << "  (taikinys buvo " << cand(target) << "; sutapimas neįrodo įvesties - kolizijos galimos, bet kandidatų rinkinyje jis identifikuoja, jei sutapimas vienintelis)\n";

    // 16 baitų atsitiktinė druska, prijungiama kaip RAW baitai
    std::string salt;
    for (int i = 0; i < 16; ++i) salt += static_cast<char>(g() & 0xFF);
    std::ostringstream hx;
    for (unsigned char c : salt) hx << std::hex << std::setw(2) << std::setfill('0') << int(c);
    std::cout << "druska (16 baitų, raw, hex užrašas): " << hx.str() << '\n';
    std::string sHash = hash(cand(target) + salt);
    scan(sHash, salt, "su vieša druska ");

    // iš anksto apskaičiuota lentelė (be druskos) - tinka visiems taikiniams be druskos
    std::unordered_map<std::string, int> table;
    for (int i = 0; i < 10000; ++i) table[hash(cand(i))] = i;
    std::cout << "Lentelė be druskos: " << table.size() << " įrašų, taikinys rastas "
              << (table.count(tHash) ? "taip" : "ne") << "; druskuotas taikinys šioje lentelėje rastas "
              << (table.count(sHash) ? "taip" : "ne") << " (kitai druskai reikia naujo pilno perrinkimo)\n";
    std::cout << "3 dalis (slaptas r): aptarkite README - kodu tikrinti nereikia.\n";
}

} // namespace

void runTests(const std::string& which) {
    auto want = [&](const char* n) { return which == "all" || which == n; };

    std::vector<Case> cases;
    if (want("1") || want("2") || want("3")) cases = loadCases();

    if (want("1")) runExp1(cases);
    if (want("2")) runExp2(cases);
    if (want("3")) runExp3(cases);
    if (want("4")) runExp4();
    if (want("5")) runExp5(100000);
    if (want("6")) runExp6(100000);
    if (want("7")) runExp7();
}
