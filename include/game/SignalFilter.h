#pragma once

#include "runtime/midp/array.h"

class SignalFilter {
public:
    enum Type {
        SMOOTHSTEP_FIR = 0,
        ONE_POLE_LP = 1,
        ONE_POLE_HP = 2,
    };

    SignalFilter(int type, float param, float steadyState);
    ~SignalFilter();

    void initSmoothstepFIR(float length);
    void initOnePoleLP(float cutoff);
    void initOnePoleHP(float cutoff);
    void initBuffers();
    void setSteadyState(float value);
    void setTargetValue(float value);
    float getFilteredValue();
    void update(int dt);
    void doFilterStep();
    float smoothStep(float t);

    int m_type;
    midp::array<float> m_feedbackCoeffs;
    midp::array<float> m_inputCoeffs;
    midp::array<float> m_inputHistory;
    midp::array<float> m_outputHistory;
    int m_numInputTaps;
    int m_numFeedbackTaps;
    int m_inputPos;
    int m_outputPos;
    float m_prevOutput;
    float m_output;
    int m_stepTime;
    float m_input;
};
