#include "m3g/KeyframeSequence.h"
namespace midp { class System { public: static void arraycopy(const float*,int,float*,int,int); }; }

namespace m3g {

KeyframeSequence::KeyframeSequence() : m_keyframes(0),m_components(0),m_interpolation(0),m_duration(0),m_repeat(0),m_first(0),m_last(0),m_times(nullptr),m_values(nullptr),m_current(0),m_next(0),m_deltaTimes(nullptr),m_tangents(),m_firstTime(0),m_lastTime(0),m_deltaValid(false),m_tangentsValid(false) {}

KeyframeSequence::KeyframeSequence(int keyframes,int components,int interpolation) : m_keyframes(0),m_components(0),m_interpolation(0),m_duration(0),m_repeat(0),m_first(0),m_last(0),m_times(nullptr),m_values(nullptr),m_current(0),m_next(0),m_deltaTimes(nullptr),m_tangents(),m_firstTime(0),m_lastTime(0),m_deltaValid(false),m_tangentsValid(false) { setInterpolation(interpolation); setKeyframeSize(keyframes,components); setRepeatMode(0xc0); setValidRange(0,keyframes-1); }

KeyframeSequence::~KeyframeSequence() { delete[] m_times; delete[] m_values; delete[] m_deltaTimes; }

int KeyframeSequence::getM3GUniqueClassID() const { return 19; }

void KeyframeSequence::updateAnimationProperty(int,const midp::array<float>&) {}

void KeyframeSequence::updateAnimationProperty(int,const midp::array<int>&) {}

int KeyframeSequence::getKeyframeTime(int index) const { return m_times[index]; }

const float* KeyframeSequence::getKeyframeValue(int index) const { return m_values+getComponentCount()*index; }

float* KeyframeSequence::getKeyframeValue(int index) { return m_values+getComponentCount()*index; }

float KeyframeSequence::getDeltaTime(int index) const { return m_deltaTimes[index]; }

void KeyframeSequence::invalidateTangentCache() { m_tangentsValid=false; }

void KeyframeSequence::invalidateDeltaTimeCache() { m_deltaValid=false; invalidateTangentCache(); }

void KeyframeSequence::setValidRange(int first,int last) { if(m_first!=first) { m_first=first; invalidateDeltaTimeCache(); } if(m_last!=last) { m_last=last; invalidateDeltaTimeCache(); } }

void KeyframeSequence::setKeyframeTime(int index,int time) { if(getKeyframeTime(index)==time) return; m_times[index]=time; invalidateDeltaTimeCache(); }

void KeyframeSequence::setInterpolation(int interpolation) { m_interpolation=interpolation; invalidateDeltaTimeCache(); }

int KeyframeSequence::calcCurrentIndex(int time,int first,int last,const int* times) {
    while(last-first>5) { int middle=(first+last)>>1; if(time<times[middle]) last=middle; else first=middle; }
    while(last>first && times[last]>time) --last;
    return last;
}

int KeyframeSequence::calcPrevIndex(int index,int first,int last,int repeat) { return index==first ? (repeat==0xc1 ? last : first) : index-1; }

int KeyframeSequence::calcNextIndex(int index,int first,int last,int repeat) { return index==last ? (repeat==0xc1 ? first : last) : index+1; }

float KeyframeSequence::getInterpolant(const KeyframeSequence& sequence,int index,float time,int) {
    float elapsed=time-static_cast<float>(sequence.getKeyframeTime(index));
    if(elapsed<0.0f) elapsed=static_cast<float>(sequence.getDuration())+elapsed;
    float delta=sequence.getDeltaTime(index); return elapsed<delta ? elapsed/delta : 1.0f;
}

float* KeyframeSequence::getOutgoingTangent(int index) { return &m_tangents[getComponentCount()*(index*2+1)]; }

float* KeyframeSequence::getIncomingTangent(int index) { return &m_tangents[index*getComponentCount()*2]; }

void KeyframeSequence::updateDeltaTimeCache() {
    if(m_deltaValid) return;
    int count=getKeyframeCount(); if(!m_deltaTimes) m_deltaTimes=new float[count];
    int first=getValidRangeFirst(), last=getValidRangeLast();
    if(first<last && count>0) {
        int previous=getKeyframeTime(0);
        for(int i=0;i<count;++i) {
            int delta=0;
            if(i>=first && i<last) { int next=getKeyframeTime(i+1); delta=next-previous; previous=next; }
            else if(i==last && getRepeatMode()==0xc1) { int duration=getDuration(); delta=(duration-previous)+getKeyframeTime(first); }
            m_deltaTimes[i]=static_cast<float>(delta);
        }
    } else if(first==last) { m_deltaTimes[first]=getRepeatMode()==0xc1 ? static_cast<float>(getDuration()) : 0.0f; }
    int index=getValidRangeFirst(); m_firstTime=getKeyframeTime(index);
    index=getValidRangeLast(); m_lastTime=getKeyframeTime(index); m_deltaValid=true;
}

void KeyframeSequence::calcIndicesAndTimes(KeyframeSequence& sequence,int time,int repeat) {
    sequence.updateDeltaTimeCache(); int first=sequence.getValidRangeFirst(), last=sequence.getValidRangeLast();
    if(first>last) return;
    int current=last, next=first;
    if(sequence.m_firstTime<=time && time<sequence.m_lastTime) { current=calcCurrentIndex(time,first,last,sequence.m_times); next=calcNextIndex(current,first,last,repeat); }
    sequence.m_current=current; sequence.m_next=next;
}

void KeyframeSequence::setKeyframeValue(int index,const float* value) { float* out=getKeyframeValue(index); int count=getComponentCount(); midp::System::arraycopy(value,0,out,0,count); invalidateTangentCache(); }

void KeyframeSequence::setKeyframe(int index,int time,const float* value) { setKeyframeTime(index,time); setKeyframeValue(index,value); }

void KeyframeSequence::setKeyframe(int index,int time,const midp::array<float>& value) { setKeyframe(index,time,&value[0]); }

void KeyframeSequence::getKeyframeValue(int index,float* value) const { const float* data=getKeyframeValue(index); int count=getComponentCount(); midp::System::arraycopy(data,0,value,0,count); }

void KeyframeSequence::getKeyframeValue(int index,midp::array<float>& value) const { getKeyframeValue(index,&value[0]); }

void KeyframeSequence::setKeyframeSize(int keyframes,int components) {
    m_keyframes=keyframes; m_components=components; delete[] m_times; m_times=new int[keyframes];
    delete[] m_values; m_values=new float[keyframes*components]; invalidateDeltaTimeCache();
}

void KeyframeSequence::interpolateValue(float a,float b,const float* first,const float* last,float* out,int components) {
    for(int i=0;i<components;++i) { float x=first[i], y=last[i]; out[i]=x*a+y*b; }
}

void KeyframeSequence::interpolateValueLINEAR(float t,const float* first,const float* last,float* out,int components) { interpolateValue(1.0f-t,t,first,last,out,components); }

void KeyframeSequence::interpolateValueSPLINE(float t,const float* first,const float* last,const float* incoming,const float* outgoing,float* out,int components) {
    float square=t*t, cube=square*t;
    float h=((cube+cube)+square*-3.0f)+1.0f;
    for(int i=0;i<components;++i) {
        float a=first[i], in=incoming[i], b=last[i], next=outgoing[i];
        out[i]=((a*h+in*(((cube-square)-square)+t))+b*(1.0f-h))+next*(cube-square);
    }
}

void KeyframeSequence::interpolateValueSLERP(float t,const float* first,const float* last,float* out) {
    float a=1.0f-t;
    float dot=((first[0]*last[0]+first[1]*last[1])+first[2]*last[2])+first[3]*last[3];
    if(dot<0.0f) t=-t;
    float x=first[0], y=first[1], z=first[2], w=first[3];
    float ly=last[1], lz=last[2], lw=last[3];
    out[0]=x*a+last[0]*t; out[1]=y*a+ly*t; out[2]=z*a+lz*t; out[3]=w*a+lw*t;
}

void KeyframeSequence::updateTangentCache() {
    updateDeltaTimeCache(); if(m_tangentsValid) return;
    int count=getKeyframeCount(), components=getComponentCount();
    if(m_tangents==nullptr) m_tangents=midp::array<float>(count*components*2);
    int first=getValidRangeFirst(), last=getValidRangeLast(), repeat=getRepeatMode();
    midp::array<float> previous(components), next(components);
    if(first<=last) for(int i=0;i<count;++i) {
        int before=i, after=i; float incomingScale=0.0f, outgoingScale=0.0f;
        if(i>=first && i<=last) {
            before=calcPrevIndex(i,first,last,repeat); after=calcNextIndex(i,first,last,repeat);
            if(repeat!=0xc0 || (i!=first && i!=last)) {
                float currentDelta=getDeltaTime(i), previousDelta=getDeltaTime(before), total=currentDelta+previousDelta;
                if(currentDelta>0.0f && total>0.0f) incomingScale=(currentDelta+currentDelta)/total;
                if(previousDelta>0.0f && total>0.0f) outgoingScale=(previousDelta+previousDelta)/total;
            }
        }
        getKeyframeValue(before,previous); getKeyframeValue(after,next);
        float* incoming=getIncomingTangent(i); float* outgoing=getOutgoingTangent(i);
        for(int j=0;j<components;++j) { float value=(next[j]-previous[j])*0.5f; incoming[j]=value*incomingScale; outgoing[j]=value*outgoingScale; }
    }
    m_tangentsValid=true;
}

void KeyframeSequence::sample(float time,int,float* value) {
    int repeat=getRepeatMode(), components=getComponentCount(), integerTime=static_cast<int>(time);
    if(repeat==0xc1) {
        int duration=getDuration();
        if(duration<=integerTime) { int remainder=integerTime%duration; time-=static_cast<float>(integerTime-remainder); integerTime=static_cast<int>(time); }
    } else if(repeat==0xc0) {
        int index=getValidRangeFirst(); int first=getKeyframeTime(index);
        if(first<=integerTime) { index=getValidRangeLast(); int last=getKeyframeTime(index); if(integerTime<last) goto interpolate; }
        midp::System::arraycopy(getKeyframeValue(index),0,value,0,components); return;
    }
interpolate:
    calcIndicesAndTimes(*this,integerTime,repeat);
    int current=m_current; const float* first=getKeyframeValue(current); int interpolation=getInterpolationType();
    if(interpolation==STEP) { midp::System::arraycopy(first,0,value,0,components); return; }
    int next=m_next; const float* last=getKeyframeValue(next); float t=getInterpolant(*this,current,time,0);
    switch(interpolation) {
    case LINEAR: interpolateValueLINEAR(t,first,last,value,components); break;
    case SLERP: interpolateValueSLERP(t,first,last,value); break;
    case SPLINE: case SQUAD: {
        updateTangentCache(); float* incoming=getIncomingTangent(current); float* outgoing=getOutgoingTangent(next);
        interpolateValueSPLINE(t,first,last,incoming,outgoing,value,components); break;
    }
    }
}

float KeyframeSequence::sample(float time,int offset) { float value; sample(time,offset,&value); return value; }

void KeyframeSequence::sample(float time,int offset,midp::array<float>& value) { sample(time,offset,&value[0]); }

int KeyframeSequence::getComponentCount() const { return m_components; }

int KeyframeSequence::getKeyframeCount() const { return m_keyframes; }

int KeyframeSequence::getInterpolationType() const { return m_interpolation; }

int KeyframeSequence::getDuration() const { return m_duration; }

int KeyframeSequence::getRepeatMode() const { return m_repeat; }

int KeyframeSequence::getValidRangeFirst() const { return m_first; }

int KeyframeSequence::getValidRangeLast() const { return m_last; }

void KeyframeSequence::setDuration(int value) { if(m_duration!=value) { m_duration=value; invalidateDeltaTimeCache(); } }

void KeyframeSequence::setRepeatMode(int value) { if(m_repeat!=value) { m_repeat=value; invalidateDeltaTimeCache(); } }

}
