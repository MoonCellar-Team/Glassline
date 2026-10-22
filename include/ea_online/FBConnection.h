#pragma once

#include <vector>

#include <string>

class FBFriend;

class FBConnection {
public:
    enum LoginResult {
        LOGIN_IN_PROGRESS = 1,
        LOGIN_SUCCEEDED = 2,
        LOGIN_CANCELED = 3,
    };

    int GetLoginResult() const;
    void GetFriendIDs(std::vector<long long>& ids) const;
    bool isLoggedIn();
    void login();
    const FBFriend& GetFriend(int index) const;

    long long m_userId;
    std::string m_unk1c;
};
