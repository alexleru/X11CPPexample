#pragma once

#include <string>
#include <vector>

enum class Operation {
    Sum = 0,
    Multiply = 1
};

class CalculatorEngine {
public:
    static std::wstring BuildResult(const std::vector<double>& values, Operation operation);
};
