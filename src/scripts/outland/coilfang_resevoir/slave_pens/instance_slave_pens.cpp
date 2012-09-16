#include "precompiled.h"

struct instance_slave_pens : public ScriptedInstance
{
    instance_slave_pens(Map *map) : ScriptedInstance(map) {}
    
    void Initialize() {}
    
    bool IsEncounterInProgress()
    {
        return false;
    }
    
    void SetData(uint32 type, uint32 data) {}
    
    uint32 GetData(uint32 type) { return 0; }
    
    const char* Save() { return NULL; }
    
    void Load(const char* in) {}
    
    void Update(uint32 diff) {}
    
};

InstanceData* GetInstanceData_instance_slave_pens(Map* pMap)
{
    return new instance_slave_pens(pMap);
}

void AddSC_instance_slave_pens()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "instance_slave_pens";
    newscript->GetInstanceData = &GetInstanceData_instance_slave_pens;
    newscript->RegisterSelf();
}
