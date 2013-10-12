#include "precompiled.h"

struct theinformAI : public ScriptedAI
{
    theinformAI(Creature *c) : ScriptedAI(c) 
    {}

    void DamageTaken(Unit* who, uint32& damage) 
    {
        RandomizeDisplayID();
        damage = 0;
    }

    void RandomizeDisplayID()
    {
        me->SetDisplayId(urand(4,25958));
    }

    void Aggro(Unit* /* who */) {}
};

CreatureAI* GetAI_theinform(Creature *_Creature)
{
    return new theinformAI (_Creature);
}
 
void AddSC_theinform()
{
    Script *newscript; 
 
    newscript = new Script;
    newscript->Name="custom_theinform";
    newscript->GetAI = &GetAI_theinform;
    newscript->RegisterSelf();
}