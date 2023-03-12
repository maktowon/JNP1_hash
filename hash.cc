#include "hash.h"
#include <unordered_map>
#include <vector>
#include <set>
#include <iostream>

using namespace std;
namespace {
    //Zmienna aktywująca wiadomości debugowania.
#ifdef DNDEBUG
    const bool debug = false;
#else
    const bool debug = true;
#endif

    // Typ mapy z id na hashmapy.
    using hash_table_map_t = unordered_map<unsigned long,
            pair<hash_function_t, unordered_map<uint64_t, set<vector<uint64_t>>>>>;

    // Typ pojedynczej hashmapy.
    using hash_table_t = unordered_map<uint64_t, set<vector<uint64_t>>>;

    // Funkcje zwracające zmienne globalne:
    // Mapę przechowującą hashmapy z odpowiednimi funkcjami.
    hash_table_map_t &hash_tables() {
        static hash_table_map_t *hash_tables = new hash_table_map_t();
        return *hash_tables;
    }

    // Id kolejnych hashmap.
    unsigned long &id_of_last_set() {
        static unsigned long id_of_last_set = 0;
        return id_of_last_set;
    }

    // Funkcja wypisująca tablicę liczb na stderr.
    void write_sequence(uint64_t const *seq, size_t size) {
        if (!seq) {
            cerr << "NULL";
            return;
        }
        if (size == 0) {
            cerr << "\"\"";
            return;
        }
        cerr << "\"";
        for (size_t i = 0; i < size - 1; ++i) {
            cerr << seq[i] << " ";
        }
        cerr << seq[size - 1];
        cerr << "\"";
    }

    // Sprawdza poprawność danych wejściowych.
    bool
    check_data_correctness(uint64_t const *seq, size_t size, string &func) {
        bool ret = true;
        if (!seq) {
            if (debug) {
                cerr << "hash_" << func << ": invalid pointer (NULL)\n";
            }
            ret = false;
        }
        if (!size) {
            if (debug) {
                cerr << "hash_" << func << ": invalid size (0)\n";
            }
            ret = false;
        }
        return ret;
    }
}

namespace jnp1 {

    unsigned long
    hash_create(uint64_t (*hash_function)(uint64_t const *v, size_t n)) {
        if (debug) {
            cerr << "hash_create(" << (void const*) hash_function << ")\n";
        }

        //Tworzenie nowej hashmapy.
        unsigned long new_id = id_of_last_set()++;
        hash_table_t new_map;
        hash_tables().emplace(new_id, make_pair(hash_function, new_map));

        if (debug) {
            cerr << "hash_create: hash table #" << (new_id) << " created\n";
        }
        return new_id;
    }

    void hash_delete(unsigned long id) {
        if (debug) {
            cerr << "hash_delete(" << id << ")\n";
        }
        if (hash_tables().erase(id) == 1) {
            if (debug) {
                cerr << "hash_delete: hash table #" << id << " deleted\n";
            }
        } else if (debug) {
            cerr << "hash_delete: hash table #" << id << " does not exist\n";
        }
    }

    size_t hash_size(unsigned long id) {
        if (debug) {
            cerr << "hash_size(" << id << ")\n";
        }

        auto hash_map_it = hash_tables().find(id);
        if (hash_map_it == hash_tables().end()) {
            if (debug) {
                cerr << "hash_size: hash table #" << id << " does not exist\n";
            }
            return 0;
        }

        hash_table_t hash_map = (*hash_map_it).second.second;
        size_t size = 0;
        //Rozmiar mapy liczymy przez sumę rozmiarów jej kubełków.
        for (const auto &[key, value]: hash_map) {
            size += value.size();
        }

        if (debug) {
            cerr << "hash_size: hash table #" << id << " contains " << size
                 << " element(s)\n";
        }
        return size;
    }

    bool hash_insert(unsigned long id, uint64_t const *seq, size_t size) {
        if (debug) {
            cerr << "hash_insert(" << id << ", ";
            write_sequence(seq, size);
            cerr << ", " << size << ")\n";
        }

        // Sprawdzamy czy dane są zgodne z wymaganiami.
        string func = "insert";
        bool ret = check_data_correctness(seq, size, func);
        auto hash_map_it = hash_tables().find(id);
        if (hash_map_it == hash_tables().end()) {
            if (debug && ret) {
                cerr << "hash_insert: hash table #" << id
                     << " does not exist\n";
            }
            ret = false;
        }
        if (!ret) {
            return false;
        }

        // Wyszukujemy odpowiedni kubełek i próbujemy dodać do niego sekwencję.
        uint64_t (*hash_function)(uint64_t const *v,
                                  size_t n) = (*hash_map_it).second.first;
        uint64_t hash = hash_function(seq, size);
        vector<uint64_t> seq_vector(seq, seq + size);
        ret = (*hash_map_it).second.second[hash].insert(seq_vector).second;
        if (debug) {
            if (ret) {
                cerr << "hash_insert: hash table #" << id << ", sequence ";
                write_sequence(seq, size);
                cerr << " inserted\n";
            } else {
                cerr << "hash_insert: hash table #" << id << ", sequence ";
                write_sequence(seq, size);
                cerr << " was present\n";
            }
        }

        return ret;
    }

    bool hash_remove(unsigned long id, uint64_t const *seq, size_t size) {
        if (debug) {
            cerr << "hash_remove(" << id << ", ";
            write_sequence(seq, size);
            cerr << ", " << size << ")\n";
        }

        // Sprawdzamy czy dane są zgodne z wymaganiami.
        string func = "remove";
        bool ret = check_data_correctness(seq, size, func);
        auto hash_map_it = hash_tables().find(id);
        if (hash_map_it == hash_tables().end()) {
            if (debug && ret) {
                cerr << "hash_remove: hash table #" << id
                     << " does not exist\n";
            }
            ret = false;
        }
        if (!ret) {
            return false;
        }

        //Znajdujemy odpowiedni kubełek (jeśli istnieje).
        uint64_t (*hash_function)(uint64_t const *v,
                                  size_t n) = (*hash_map_it).second.first;
        uint64_t hash = hash_function(seq, size);
        vector<uint64_t> seq_vector(seq, seq + size);
        auto map = &(*hash_map_it).second.second;
        auto bucket = (*map).find(hash);

        //Jeśli taki kubełek istnieje - to usuwamy z niego sekwencję.
        //Jeśli spowodowuje to, że kubełek jest pusty, to go usuwamy.
        if (bucket != (*map).end()) {
            int num_of_elements_erased = (*bucket).second.erase(seq_vector);
            if ((*bucket).second.size() == 0) {
                (*map).erase(bucket);
            }
            if (debug) {
                if (num_of_elements_erased) {
                    cerr << "hash_remove: hash table #" << id << ", sequence ";
                    write_sequence(seq, size);
                    cerr << " removed\n";
                } else {
                    cerr << "hash_remove: hash table #" << id << ", sequence ";
                    write_sequence(seq, size);
                    cerr << " was not present\n";
                }
            }
            return num_of_elements_erased;
        }
        //Jeśli nie ma takiego kubełka, to nie ma też sekwencji.
        cerr << "hash_remove: hash table #" << id << ", sequence ";
        write_sequence(seq, size);
        cerr << " was not present\n";
        return false;
    }

    void hash_clear(unsigned long id) {
        if (debug) {
            cerr << "hash_clear(" << id << ")\n";
        }

        // Sprawdzamy, czy hashmapa o danym id istnieje.
        auto hash_map_it = hash_tables().find(id);
        if (hash_map_it == hash_tables().end()) {
            if (debug) {
                cerr << "hash_clear: hash table #" << id << " does not exist\n";
            }
            return;
        }

        if ((*hash_map_it).second.second.empty()) {
            if (debug) {
                cerr << "hash_clear: hash table #" << id << " was empty\n";
            }
            return;
        }

        (*hash_map_it).second.second.clear();
        if (debug) {
            cerr << "hash_clear: hash table #" << id << " cleared\n";
        }
    }

    bool hash_test(unsigned long id, uint64_t const *seq, size_t size) {
        if (debug) {
            cerr << "hash_test(" << id << ", ";
            write_sequence(seq, size);
            cerr << ", " << size << ")\n";
        }

        // Sprawdzamy zgodność danych z wymaganiami.
        string func = "test";
        bool ret = check_data_correctness(seq, size, func);
        auto hash_map_it = hash_tables().find(id);
        if (hash_map_it == hash_tables().end()) {
            if (debug && ret) {
                cerr << "hash_test: hash table #" << id << " does not exist\n";
            }
            ret = false;
        }
        if (!ret) {
            return false;
        }

        //Znajdujemy odpowiedni kubełek.
        uint64_t (*hash_function)(uint64_t const *v,
                                  size_t n) = (*hash_map_it).second.first;
        uint64_t hash = hash_function(seq, size);
        set<vector<uint64_t>> set = (*hash_map_it).second.second[hash];
        vector<uint64_t> seq_vector(seq, seq + size);

        if (set.find(seq_vector) != set.end()) {
            if (debug) {
                cerr << "hash_test: hash table #" << id << ", sequence ";
                write_sequence(seq, size);
                cerr << " is present\n";
            }
            return true;
        }
        if (debug) {
            cerr << "hash_test: hash table #" << id << ", sequence ";
            write_sequence(seq, size);
            cerr << " is not present\n";
        }

        return false;
    }
}
