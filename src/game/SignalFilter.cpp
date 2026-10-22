#include "game/SignalFilter.h"

#include <math.h>

static const int STEP_MS = 30;

SignalFilter::SignalFilter(int type, float param, float steadyState)
    : m_type(type),
      m_numInputTaps(-1),
      m_numFeedbackTaps(-1),
      m_inputPos(0),
      m_outputPos(0),
      m_prevOutput(0),
      m_output(0),
      m_stepTime(0),
      m_input(0) {
    switch (m_type) {
    case SMOOTHSTEP_FIR:
        initSmoothstepFIR(param);
        break;
    case ONE_POLE_LP:
        initOnePoleLP(param);
        break;
    case ONE_POLE_HP:
        initOnePoleHP(param);
        break;
    }
    m_input = steadyState;
    setSteadyState(steadyState);
}

SignalFilter::~SignalFilter() {}

void SignalFilter::initBuffers() {
    m_inputCoeffs = midp::array<float>(m_numInputTaps);
    m_inputHistory = midp::array<float>(m_numInputTaps);
    m_feedbackCoeffs = midp::array<float>(m_numFeedbackTaps);
    m_outputHistory = midp::array<float>(m_numFeedbackTaps);
}

void SignalFilter::initSmoothstepFIR(float length) {
    m_numFeedbackTaps = 0;
    int taps = (int)(length * (1.0f / STEP_MS));
    if (taps < 3) {
        taps = 3;
    }
    m_numInputTaps = taps + (taps % 2 == 1);
    initBuffers();

    float sum = 0;
    for (int i = 0; i < m_numInputTaps / 2; i++) {
        float k = smoothStep(i * (2.0f / m_numInputTaps));
        m_inputCoeffs[i] = k;
        m_inputCoeffs[m_numInputTaps - 1 - i] = k;
        sum = m_inputCoeffs[i] + m_inputCoeffs[i] + sum;
    }
    for (int i = 0; i < m_numInputTaps; i++) {
        m_inputCoeffs[i] /= sum;
    }
}

void SignalFilter::initOnePoleLP(float cutoff) {
    m_numInputTaps = 1;
    m_numFeedbackTaps = 1;
    float k = expf(logf(0.1f) * STEP_MS / cutoff);
    initBuffers();
    m_feedbackCoeffs[0] = -k;
    m_inputCoeffs[0] = m_feedbackCoeffs[0] + 1.0f;
}

void SignalFilter::initOnePoleHP(float cutoff) {
    m_numInputTaps = 2;
    m_numFeedbackTaps = 1;
    float k = expf(logf(0.1f) * STEP_MS / cutoff);
    initBuffers();
    m_feedbackCoeffs[0] = -k;
    m_inputCoeffs[0] = k;
    m_inputCoeffs[1] = -k;
}

float SignalFilter::smoothStep(float t) {
    return t * t * t * -2.0f + t * t * 3.0f;
}

void SignalFilter::setSteadyState(float value) {
    for (int i = 0; i < m_numInputTaps; i++) {
        m_inputHistory[i] = value;
    }
    for (int i = 0; i < m_numFeedbackTaps; i++) {
        m_outputHistory[i] = value;
    }
    m_input = value;
    m_output = value;
    m_prevOutput = value;
}

void SignalFilter::setTargetValue(float value) {
    m_input = value;
}

void SignalFilter::update(int dt) {
    m_stepTime += dt;
    while (m_stepTime >= STEP_MS) {
        doFilterStep();
        m_stepTime -= STEP_MS;
    }
}

void SignalFilter::doFilterStep() {
    m_prevOutput = m_output;
    m_output = 0;
    m_inputHistory[m_inputPos] = m_input;
    for (int i = 0; i < m_numInputTaps; i++) {
        m_output += m_inputHistory[(m_inputPos + i) % m_numInputTaps] * m_inputCoeffs[i];
    }
    m_inputPos--;
    if (m_inputPos < 0) {
        m_inputPos += m_numInputTaps;
    }

    if (m_numFeedbackTaps > 0) {
        for (int i = 0; i < m_numFeedbackTaps; i++) {
            m_output -= m_outputHistory[(m_outputPos + i) % m_numFeedbackTaps] * m_feedbackCoeffs[i];
        }
        m_outputHistory[m_outputPos] = m_output;
        m_outputPos--;
        if (m_outputPos < 0) {
            m_outputPos += m_numFeedbackTaps;
        }
    }
}

float SignalFilter::getFilteredValue() {
    float t = m_stepTime * (1.0f / STEP_MS);
    return m_prevOutput * (1.0f - t) + m_output * t;
}
