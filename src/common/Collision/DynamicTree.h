
#ifndef _DYNTREE_H
#define _DYNTREE_H

#include "Define.h"

namespace G3D
{
    class Ray;
    class Vector3;
}

class GameObjectModel;
struct DynTreeImpl;

class TC_COMMON_API DynamicMapTree
{
    DynTreeImpl *impl;

public:

    DynamicMapTree();
    ~DynamicMapTree();

    bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2,
                         float z2, uint32 phasemask) const;

    bool getIntersectionTime(uint32 phasemask, const G3D::Ray& ray,
                             const G3D::Vector3& endPos, float& maxDist) const;

    bool getObjectHitPos(uint32 phasemask, const G3D::Vector3& pPos1,
                         const G3D::Vector3& pPos2, G3D::Vector3& pResultHitPos,
                         float pModifyDist) const;

    /**
    Returns closest z position downwards within maxSearchDist
    Returns -G3D::finf() if nothing found
    */
    float getHeight(float x, float y, float z, float maxSearchDist, uint32 phasemask) const;
    /**
    Returns closest z position upwards within maxSearchDist
    Returns -G3D::finf() if nothing found
    */
    float getCeil(float x, float y, float z, float maxSearchDist, uint32 phasemask) const;

    void insert(GameObjectModel const&);
    void remove(GameObjectModel const&);
    bool contains(GameObjectModel const&) const;

    void balance();
    void update(uint32 diff);
};

#endif // _DYNTREE_H
