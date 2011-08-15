#ifndef MANGOS_CHARGEMOVEMENTGENERATOR_H
#define MANGOS_CHARGEMOVEMENTGENERATOR_H

#include "WaypointMovementGenerator.h"
#include "PathFinder.h"

// TODO: figure out nice speed for charge
#define CHARGE_SPEED 25.0f

template<class T, class U>
class ChargeMovementGeneratorMedium : public MovementGeneratorMedium<T, U>, public PathMovementBase<T>
{
public:
    ChargeMovementGeneratorMedium(Unit* target, const uint32 triggeredSpellId);

    void Initialize(T &u);
    void Interrupt(T &);
    void Finalize(T &);
    void Reset(T &u);
    bool Update(T &u, const uint32 &diff);

    void LoadPath(T &u);

    MovementGeneratorType GetMovementGeneratorType() const { return CHARGE_MOTION_TYPE; }

private:
    Unit* m_target;
    const uint32 m_triggeredSpellId;

    void MoveToNextNode(Traveller<T> &traveller);
    
    DestinationHolder< Traveller<T> > i_destinationHolder;
    PathInfo* i_path;
    uint32 i_currentNode;
};

template<class T>
class ChargeMovementGenerator : public ChargeMovementGeneratorMedium<T, ChargeMovementGenerator<T> >
{
public:
    ChargeMovementGenerator(Unit* target, const uint32 triggeredSpellId)
        : ChargeMovementGeneratorMedium<T, ChargeMovementGenerator<T> >(target, triggeredSpellId) {}
        
    MovementGeneratorType GetMovementGeneratorType() { return CHARGE_MOTION_TYPE; }
};

#endif
