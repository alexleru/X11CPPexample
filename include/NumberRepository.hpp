#pragma once

#include <cstddef>
#include <vector>

class NumberRepository {
public:
    void Add(double value);
    bool RemoveAt(std::size_t index);
    const std::vector<double>& All() const;
    bool Empty() const;

private:
    std::vector<double> numbers_;
};
