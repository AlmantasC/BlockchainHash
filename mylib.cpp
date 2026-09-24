#include "mylib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <stdexcept>

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

std::string hash(const std::string& in) {
    uint64_t a[4] = {1,2,3,4};
    uint64_t b[4] = {
        0x9e3779b97f4a7c15ULL, 0xbf58476d1ce4e5b9ULL,
        0x94d049bb133111ebULL, 0x100000001b3ULL
    };

    for (unsigned char c : in)
        for (int i=0; i<4; ++i)
            a[i]=a[i]*b[i]+c;

    std::ostringstream os;
    for (uint64_t v : a) os<<std::hex<<std::setw(16)<<std::setfill('0')<<v;
    return os.str();
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
