#pragma once

namespace ea {

class EASpywareManager {
public:
    static EASpywareManager* getInstance();
    void handleOpenURL(void* url);
    bool HaveLoginAtLeastOnceIntoOrigin();
};

}
