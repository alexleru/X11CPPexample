#include "NumberRepository.hpp"

void NumberRepository::Add(double value) {
    numbers_.push_back(value);
}

bool NumberRepository::RemoveAt(std::size_t index) {
    if (index >= numbers_.size()) return false;
    numbers_.erase(numbers_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

const std::vector<double>& NumberRepository::All() const {
    return numbers_;
}

bool NumberRepository::Empty() const {
    return numbers_.empty();
}
