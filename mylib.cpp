#include "mylib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <stdexcept>
#include <random>
#include <utility>

int getInt(int min, int max) {
    int value;
    while (true) {
        try {
            if (!(std::cin>>value)) {
                if (std::cin.eof()) throw std::ios_base::failure("Įvestis nutraukta");
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                throw std::runtime_error("Neteisinga įvestis");
            }
            char leftover;
            if (std::cin.get(leftover)&&leftover!='\n') {
                std::cin.ignore(10000, '\n');
                throw std::runtime_error("Neteisinga įvestis");
            }
            if (value<min||value>max)
                throw std::runtime_error("Pasirinkimas turi būti tarp "+std::to_string(min)+" ir "+std::to_string(max));
            return value;
        }
        catch (const std::runtime_error& e) {
            std::cout<<e.what()<<", pabandykite dar kartą: ";
        }
    }
}

std::string getFile() {
    std::string failas;
    std::cout << "Įveskite failo pavadinimą: ";
    while (std::getline(std::cin, failas)) {
        std::ifstream fin(failas);
        if (fin.is_open()) return failas;
        std::cout << "Failas \"" << failas << "\" nerastas, pabandykite dar kartą: ";
    }
    throw std::runtime_error("Įvestis nutraukta");
}

std::vector<std::string> readManual() {
    std::vector<std::string> A;
    std::string eil;
    std::cout << "Įveskite eilutes (tuščia eilutė - pabaiga):\n";
    while (std::getline(std::cin, eil) && !eil.empty())
        A.push_back(eil);
    return A;
}

std::vector<std::string> readFile(const std::string& failas) {
    std::vector<std::string> A;
    std::ifstream fin(failas);
    if (!fin.is_open())
        throw std::runtime_error("Nepavyko atidaryti failo \"" + failas + "\"");
    std::string eil;
    while (std::getline(fin, eil)) {
        if (!eil.empty() && eil.back()=='\r') eil.pop_back();
        A.push_back(eil);
    }
    return A;
}

namespace {
    constexpr uint64_t SETUP_SEED = 0x5EED0F1A5C0DE001ULL;

    struct Params {
        uint64_t a[4];
        uint64_t b[4];
    };

    Params makeParams() {
        std::mt19937_64 gen(SETUP_SEED);
        Params p;
        for (int i = 0; i < 4; ++i) p.a[i] = gen() | 1ULL;
        for (int i = 0; i < 4; ++i) p.b[i] = gen() | 1ULL;
        return p;
    }

    const Params& params() {
        static const Params p = makeParams();
        return p;
    }
}

std::string hash(const std::string& in) {
    const Params& p = params();
    uint64_t a[4] = {p.a[0], p.a[1], p.a[2], p.a[3]};
    const uint64_t* b = p.b;

    for (unsigned char c : in)
        for (int i=0; i<4; ++i)
            a[i]=(a[i]+c)*b[i];

    std::ostringstream os;
    for (uint64_t v : a) os<<std::hex<<std::setw(16)<<std::setfill('0')<<v;
    std::string h = os.str();

    std::mt19937_64 gen(a[0]);
    for (size_t i = 0, j = h.size(); i + 1 < j; ++i) {
        --j;
        if (gen() >> 63)
            std::swap(h[i], h[j]);
    }
    return h;
}

std::string readFileBytes(const std::string& failas) {
    std::ifstream fin(failas, std::ios::binary);
    if (!fin.is_open())
        throw std::runtime_error("Nepavyko atidaryti failo \"" + failas + "\"");
    std::ostringstream ss;
    ss << fin.rdbuf();
    if (fin.bad())
        throw std::runtime_error("Klaida skaitant failą \"" + failas + "\"");
    return ss.str();
}

std::vector<std::string> hashAll(const std::vector<std::string>& in) {
    std::vector<std::string> H;
    H.reserve(in.size());
    for (const std::string& s : in)
        H.push_back(hash(s));
    return H;
}

void printRez(std::ostream& out, const std::vector<std::string>& in, const std::vector<std::string>& hashes) {
    if (in.empty()) {
        out << "Nėra duomenų.\n";
        return;
    }
    for (size_t i=0; i<in.size(); ++i)
        out << hashes[i] << "  " << in[i] << " " << in[i].size() << '\n';
}
