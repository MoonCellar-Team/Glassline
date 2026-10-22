#pragma once

#include <string>

class PendingChallenge {
public:
    PendingChallenge(const std::string& id, const std::string& name);
    PendingChallenge(const PendingChallenge& other);
    ~PendingChallenge();

    std::string m_id;
    std::string m_name;
};
