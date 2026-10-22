#pragma once

#include "runtime/midp/ReferenceCounted.h"

namespace midp {
class StringBuffer;
}

class Achievement : public midp::ReferenceCounted {
public:
    Achievement(int idx, int nameId, int descriptionId);
    virtual ~Achievement();

    virtual midp::StringBuffer* getNameStringBuffer();
    virtual midp::StringBuffer* getDescriptionStringBuffer();

    int getIdx() const;
    int getNameId() const;
    bool isComplete();
    void complete();

    static const int DYNAMIC_TEXT_ID = -12;

    int m_idx;
    int m_nameId;
    int m_descriptionId;
    bool m_complete;
};
