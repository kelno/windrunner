#include "precompiled.h"

struct linformeAI : public ScriptedAI
{
    linformeAI(Creature *c) : ScriptedAI(c) 
    {
        RandomizeDisplayID();
    }

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

CreatureAI* GetAI_linforme(Creature *_Creature)
{
    return new linformeAI (_Creature);
}
 
void AddSC_linforme()
{
    Script *newscript; 
 
    newscript = new Script;
    newscript->Name="linforme";
    newscript->GetAI = &GetAI_linforme;
    newscript->RegisterSelf();
}