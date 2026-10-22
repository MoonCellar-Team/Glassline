#pragma once

#include "game/MathVector.h"

class InterpolationTimed {
public:
    enum InterpolationType {
        LINEAR = 0,
        EASE_IN_QUAD = 1,
        EASE_OUT_QUAD = 2,
        EASE_IN_CUBIC = 3,
        EASE_OUT_CUBIC = 4,
    };

    InterpolationTimed();
    virtual ~InterpolationTimed();
    virtual void applyProgress(float t) = 0;
    virtual void applyEndValue() = 0;

    void start(int duration, InterpolationType type);
    void update(int dt);
    void stop();
    bool isFinished();

    int m_elapsed;
    int m_duration;
    int m_type;
};

class InterpolationFloatTimed : public InterpolationTimed {
public:
    InterpolationFloatTimed();
    virtual ~InterpolationFloatTimed();
    virtual void applyProgress(float t);
    virtual void applyEndValue();

    void start(float from, float to, int duration, InterpolationType type);
    float getCurrentValue();

    float m_start;
    float m_end;
    float m_current;
};

class InterpolationVectorTimed : public InterpolationTimed {
public:
    InterpolationVectorTimed();
    virtual ~InterpolationVectorTimed();
    virtual void applyProgress(float t);
    virtual void applyEndValue();

    void start(const MathVector& from, const MathVector& to, int duration, InterpolationType type);
    MathVector& getStartValue();
    MathVector& getEndValue();
    MathVector& getCurrentValue();

    MathVector m_start;
    MathVector m_end;
    MathVector m_current;
};
