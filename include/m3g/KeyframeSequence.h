#pragma once
#include "m3g/Object3D.h"

namespace m3g {

class KeyframeSequence : public Object3D {
public:
    enum { LINEAR=176, SLERP=177, SPLINE=178, SQUAD=179, STEP=180 };
    KeyframeSequence();
    KeyframeSequence(int keyframes,int components,int interpolation);
    virtual ~KeyframeSequence();
    virtual int getM3GUniqueClassID() const;
    virtual void updateAnimationProperty(int,const midp::array<float>&);
    virtual void updateAnimationProperty(int,const midp::array<int>&);
    int getComponentCount() const;
    int getKeyframeCount() const;
    int getInterpolationType() const;
    int getDuration() const;
    int getRepeatMode() const;
    int getValidRangeFirst() const;
    int getValidRangeLast() const;
    int getKeyframeTime(int index) const;
    const float* getKeyframeValue(int index) const;
    float* getKeyframeValue(int index);
    void getKeyframeValue(int index,float* value) const;
    void getKeyframeValue(int index,midp::array<float>& value) const;
    float getDeltaTime(int index) const;
    float* getOutgoingTangent(int index);
    float* getIncomingTangent(int index);
    void invalidateTangentCache();
    void invalidateDeltaTimeCache();
    void updateDeltaTimeCache();
    void updateTangentCache();
    void setValidRange(int first,int last);
    void setRepeatMode(int mode);
    void setKeyframeTime(int index,int time);
    void setDuration(int duration);
    void setInterpolation(int interpolation);
    void setKeyframeValue(int index,const float* value);
    void setKeyframe(int index,int time,const float* value);
    void setKeyframe(int index,int time,const midp::array<float>& value);
    void setKeyframeSize(int keyframes,int components);
    void sample(float time,int offset,float* value);
    void sample(float time,int offset,midp::array<float>& value);
    float sample(float time,int offset);
    static int calcCurrentIndex(int time,int first,int last,const int* times);
    static int calcPrevIndex(int index,int first,int last,int repeat);
    static int calcNextIndex(int index,int first,int last,int repeat);
    static float getInterpolant(const KeyframeSequence& sequence,int index,float time,int unused);
    static void calcIndicesAndTimes(KeyframeSequence& sequence,int time,int repeat);
    static void interpolateValue(float a,float b,const float* first,const float* last,float* out,int components);
    static void interpolateValueLINEAR(float t,const float* first,const float* last,float* out,int components);
    static void interpolateValueSPLINE(float t,const float* first,const float* last,const float* incoming,const float* outgoing,float* out,int components);
    static void interpolateValueSLERP(float t,const float* first,const float* last,float* out);
    int m_keyframes;
    int m_components;
    int m_interpolation;
    int m_duration;
    int m_repeat;
    int m_first;
    int m_last;
    int* m_times;
    float* m_values;
    int m_current;
    int m_next;
    float* m_deltaTimes;
    midp::array<float> m_tangents;
    int m_firstTime;
    int m_lastTime;
    bool m_deltaValid;
    bool m_tangentsValid;
};

}
