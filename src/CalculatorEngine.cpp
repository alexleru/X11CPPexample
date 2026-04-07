#include "CalculatorEngine.hpp"

#include "StringUtils.hpp"

std::wstring CalculatorEngine::BuildResult(const std::vector<double>& values, Operation operation) {
    if (values.empty()) return L"Result: (no numbers)";

    double result = (operation == Operation::Sum) ? 0.0 : 1.0;
    for (double value : values) {
        if (operation == Operation::Sum) {
            result += value;
        } else {
            result *= value;
        }
    }

    const std::wstring prefix =
        (operation == Operation::Sum) ? L"Result (Sum): " : L"Result (Multiply): ";
    return prefix + ToWide(FormatNumber(result));
}
