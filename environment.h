#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
class Environment {
private:
    std::vector<std::unordered_map<std::string, T>> ribs;

    int search_rib(const std::string& var) const {
        for (int idx = static_cast<int>(ribs.size()) - 1; idx >= 0; --idx) {
            if (ribs[idx].count(var))
                return idx;
        }
        return -1;
    }

public:
    Environment() = default;

    void add_level() {
        ribs.emplace_back();
    }

    bool remove_level() {
        if (ribs.empty()) return false;
        ribs.pop_back();
        return true;
    }

    void clear() {
        ribs.clear();
    }

    void add_var(const std::string& var, const T& value) {
        if (ribs.empty()) {
            std::cerr << "Environment empty rib on add_var" << std::endl;
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = value;
    }

    void add_var(const std::string& var) {
        if (ribs.empty()) {
            std::cerr << "Environment empty rib on add_var" << std::endl;
            exit(EXIT_FAILURE);
        }
        ribs.back()[var] = T{};
    }

    bool update(const std::string& x, const T& v) {
        int idx = search_rib(x);
        if (idx < 0) return false;
        ribs[idx][x] = v;
        return true;
    }

    bool check(const std::string& x) const {
        return search_rib(x) >= 0;
    }

    T lookup(const std::string& x) const {
        int idx = search_rib(x);
        if (idx < 0) {
            return T{};
        }
        return ribs[idx].at(x);
    }

    bool lookup(const std::string& x, T& v) const {
        int idx = search_rib(x);
        if (idx < 0) return false;
        v = ribs[idx].at(x);
        return true;
    }
};

#endif
