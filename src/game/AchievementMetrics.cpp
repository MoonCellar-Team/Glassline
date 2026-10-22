#include "game/AchievementMetrics.h"

#include <string.h>

#include "runtime/midp/DataInputStream.h"
#include "runtime/midp/DataOutputStream.h"

AchievementMetrics::AchievementMetrics() {
    memset(this, 0, sizeof(AchievementMetrics));
}

AchievementMetrics& AchievementMetrics::operator+=(const AchievementMetrics& other) {
    m_deaths += other.m_deaths;
    m_kills += other.m_kills;
    m_rivalKills += other.m_rivalKills;
    m_disarms += other.m_disarms;
    m_enemyFalls += other.m_enemyFalls;
    m_bags += other.m_bags;
    m_badLandings += other.m_badLandings;
    m_unk1c += other.m_unk1c;
    m_runDist += other.m_runDist;
    m_wallRunDist += other.m_wallRunDist;
    m_climbDist += other.m_climbDist;
    m_ziplineDist += other.m_ziplineDist;
    m_balanceDist += other.m_balanceDist;
    m_slideDist += other.m_slideDist;
    m_fallDist += other.m_fallDist;
    m_timeMillis += other.m_timeMillis;
    return *this;
}

void AchievementMetrics::write(midp::DataOutputStream& out) {
    out.writeInt(m_deaths);
    out.writeInt(m_kills);
    out.writeInt(m_rivalKills);
    out.writeInt(m_disarms);
    out.writeInt(m_enemyFalls);
    out.writeInt(m_bags);
    out.writeInt(m_badLandings);
    out.writeInt(m_unk1c);
    out.writeFloat(m_runDist);
    out.writeFloat(m_wallRunDist);
    out.writeFloat(m_climbDist);
    out.writeFloat(m_ziplineDist);
    out.writeFloat(m_balanceDist);
    out.writeFloat(m_slideDist);
    out.writeFloat(m_fallDist);
    out.writeInt(m_timeMillis);
}

void AchievementMetrics::read(midp::DataInputStream& in) {
    if (in.available() < (int)sizeof(AchievementMetrics)) {
        return;
    }
    m_deaths = in.readInt();
    m_kills = in.readInt();
    m_rivalKills = in.readInt();
    m_disarms = in.readInt();
    m_enemyFalls = in.readInt();
    m_bags = in.readInt();
    m_badLandings = in.readInt();
    m_unk1c = in.readInt();
    m_runDist = in.readFloat();
    m_wallRunDist = in.readFloat();
    m_climbDist = in.readFloat();
    m_ziplineDist = in.readFloat();
    m_balanceDist = in.readFloat();
    m_slideDist = in.readFloat();
    m_fallDist = in.readFloat();
    m_timeMillis = in.readInt();
}
