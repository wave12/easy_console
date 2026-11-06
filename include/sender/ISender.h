#pragma once
#include <string>
#include <nlohmann/json.hpp>

class ISender {
public:
    virtual ~ISender() = default;
    virtual bool send(const std::string& json_payload) = 0;
};

