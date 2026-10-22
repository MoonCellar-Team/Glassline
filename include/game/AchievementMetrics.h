#pragma once

namespace midp {
class DataInputStream;
class DataOutputStream;
}

class AchievementMetrics {
public:
    AchievementMetrics();

    AchievementMetrics& operator+=(const AchievementMetrics& other);
    void write(midp::DataOutputStream& out);
    void read(midp::DataInputStream& in);

    int m_deaths;
    int m_kills;
    int m_rivalKills;
    int m_disarms;
    int m_enemyFalls;
    int m_bags;
    int m_badLandings;
    int m_unk1c;
    float m_runDist;
    float m_wallRunDist;
    float m_climbDist;
    float m_ziplineDist;
    float m_balanceDist;
    float m_slideDist;
    float m_fallDist;
    int m_timeMillis;
};
