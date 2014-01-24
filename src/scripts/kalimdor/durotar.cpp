/* ScriptData
SDName: Durotar
SD%Complete: 100
SDComment: Quest support: 5441
SDCategory: Durotar
EndScriptData */

/* ContentData
npc_lazy_peon
EndContentData */

#include "precompiled.h"

/*######
## npc_lazy_peon
######*/

enum LazyPeonYells
{
    SAY_SPELL_HIT                               = -1645010   //Ow! OK, I''ll get back to work, $N!'
};

enum LazyPeon
{
    GO_LUMBERPILE                               = 175784,

    SPELL_BUFF_SLEEP                            = 17743,
    SPELL_AWAKEN_PEON                           = 19938
};

struct npc_lazy_peonAI : public ScriptedAI
{
    npc_lazy_peonAI(Creature *c) : ScriptedAI(c) {}

    uint32 m_uiRebuffTimer;
    uint32 m_homeTimer;
    uint32 workTimer;
    
    uint8 workCount;
    
    bool work;

    void Reset ()
    {
        work = false;
        m_uiRebuffTimer = 0;
        workCount = 0;
    }

    void MovementInform(uint32, uint32 id)
    {
        if (id == 0) {
            if (!m_creature->HasAura(SPELL_BUFF_SLEEP))
                DoCast(m_creature, SPELL_BUFF_SLEEP);
            workCount = 0;
        }
        else if (id == 1) {
            work = true;
            m_homeTimer = 10000;
            workTimer = 1000;
        }
    }
    
    void EnterCombat(Unit* pWho) {}

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if (spell->Id == SPELL_AWAKEN_PEON && caster->GetTypeId() == TYPEID_PLAYER
            && CAST_PLR(caster)->GetQuestStatus(5441) == QUEST_STATUS_INCOMPLETE)
        {
            (caster->ToPlayer())->KilledMonster(m_creature->GetEntry(), m_creature->GetGUID());
            DoScriptText(SAY_SPELL_HIT, m_creature, caster);
            m_creature->RemoveAllAuras();
            m_creature->SetSheath(SHEATH_STATE_MELEE);
            GameObject* Lumberpile = m_creature->FindGOInGrid(GO_LUMBERPILE, 20);
            if(Lumberpile)
                m_creature->GetMotionMaster()->MovePoint(1, Lumberpile->GetPositionX()-1, Lumberpile->GetPositionY(), Lumberpile->GetPositionZ());
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (work) {
            if (workTimer <= uiDiff) {
                m_creature->HandleEmoteCommand(EMOTE_STATE_WORK_NOSHEATHE);
                workCount++;
                workTimer = 1000;
            }
            else
                workTimer -= uiDiff;
                
            if (workCount == 5)
                work = false;
        }
        
        if (m_uiRebuffTimer <= uiDiff)
        {
            DoCast(m_creature, SPELL_BUFF_SLEEP);
            m_uiRebuffTimer = 90000;
        }
        else
            m_uiRebuffTimer -= uiDiff;
            
        if (m_homeTimer) {
            if (m_homeTimer <= uiDiff) {
                float x, y, z, o;
                m_creature->SetSheath(SHEATH_STATE_UNARMED);
                m_creature->GetHomePosition(x, y, z, o);
                m_creature->GetMotionMaster()->MovePoint(0, x, y, z);
                m_homeTimer = 0;
            }
            else
                m_homeTimer -= uiDiff;
        }
            
        if (!UpdateVictim())
            return;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_lazy_peon(Creature* pCreature)
{
    return new npc_lazy_peonAI(pCreature);
}

void AddSC_durotar()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "npc_lazy_peon";
    newscript->GetAI = &GetAI_npc_lazy_peon;
    newscript->RegisterSelf();
}
