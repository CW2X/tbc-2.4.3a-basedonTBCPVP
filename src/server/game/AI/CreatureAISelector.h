
#ifndef TRINITY_CREATUREAISELECTOR_H
#define TRINITY_CREATUREAISELECTOR_H

class CreatureAI;
class Creature;
class MovementGenerator;
class GameObjectAI;
class GameObject;

namespace FactorySelector
{
    CreatureAI* SelectAI(Creature*);
    MovementGenerator* SelectMovementGenerator(Unit*);
    GameObjectAI* SelectGameObjectAI(GameObject*);
}
#endif

