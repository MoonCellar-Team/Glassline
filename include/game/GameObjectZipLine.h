#pragma once

#include "game/GameObject.h"
#include "game/MathLine.h"

class MapPalette;

class GameObjectZipLine : public GameObject {
public:
    static const int TYPE = 17;

    GameObjectZipLine(MEdgeMap* map, MapPalette& palette, float x1, float y1, float x2, float y2, float endOffset);
    virtual ~GameObjectZipLine();

    MathLine& getZipLineLine();
    float getEndOffset();

    MathLine m_line;
    float m_endOffset;
};
