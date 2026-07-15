#ifndef SIMPLE_XML_VALIDATOR_CORE_VALIDATION_INPUT_VALIDATOR_H
#define SIMPLE_XML_VALIDATOR_CORE_VALIDATION_INPUT_VALIDATOR_H

#include <filesystem>
#include <string>

struct InputCheckResult {
    bool                  ok;
    std::filesystem::path absolutePath;
    std::string           errorMessage;
};

class InputValidator {
public:
    static InputCheckResult check(const std::filesystem::path& path);
};

#endif
