/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: Boss_Warlord_Najentus
SD%Complete: 95
SDComment:
SDCategory: Black Temple
EndScriptData */

#include "precompiled.h"
#include "def_black_temple.h"

enum NajentusSay {
    SAY_AGGRO                      = -1564000,
    SAY_NEEDLE1                    = -1564001,
    SAY_NEEDLE2                    = -1564002,
    SAY_SLAY1                      = -1564003,
    SAY_SLAY2                      = -1564004,
    SAY_SPECIAL1                   = -1564005,
    SAY_SPECIAL2                   = -1564006,
    SAY_ENRAGE1                    = -1564007,            //is this text actually in use?
    SAY_ENRAGE2                    = -1564008,
    SAY_DEATH                      = -1564009
};

enum NajentusSpells {
    SPELL_NEEDLE_SPINE             = 39992, //dummy aura
    SPELL_NEEDLE_SPINE2            = 39835, // physical damage + script effect
    SPELL_NEEDLE_SPINE_EXPLOSION   = 39968, // AoE damages, linked via spell_linked_spell
    SPELL_TIDAL_BURST              = 39878,
    SPELL_TIDAL_SHIELD             = 39872,
    SPELL_IMPALING_SPINE           = 39837,
    SPELL_CREATE_NAJENTUS_SPINE    = 39956,
    SPELL_HURL_SPINE               = 39948,
    SPELL_BERSERK                  = 45078
};

#define TIMER_NEEDLE_SPINE           2000 + rand()%500
#define TIMER_NEEDLE_SPINE_START     5000
#define TIMER_TIDAL_SHIELD           60000
#define TIMER_SPECIAL_YELL           25000 + (rand()%76)*1000 // 25 - 100
#define TIMER_SPECIAL_YELL_START     TIMER_SPECIAL_YELL + 20000
#define TIMER_ENRAGE                 480000
#define TIMER_IMPALING_SPINE         21000

#define CREATURE_INVISIBLE_ANNOUNCER 30000 // To announce gate opening

enum NajentusGobjects {
    GOBJECT_SPINE                  = 185584
};

struct boss_najentusAI : public ScriptedAI
{
    boss_najentusAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    ScriptedInstance* pInstance;

    uint32 NeedleSpineTimer;
    uint32 EnrageTimer;
    uint32 SpecialYellTimer;
    uint32 TidalShieldTimer;
    uint32 ImpalingSpineTimer;

    uint64 SpineTargetGUID;

    void Reset()
    {
        EnrageTimer = TIMER_ENRAGE;
        SpecialYellTimer = TIMER_SPECIAL_YELL_START;
        TidalShieldTimer = TIMER_TIDAL_SHIELD;

        NeedleSpineTimer = TIMER_NEEDLE_SPINE_START;
        ImpalingSpineTimer = TIMER_IMPALING_SPINE;

        SpineTargetGUID = 0;

        if(pInstance && m_creature->IsAlive())
            pInstance->SetData(DATA_HIGHWARLORDNAJENTUSEVENT, NOT_STARTED);
    }

    void KilledUnit(Unit *victim)
    {
        switch(rand()%2)
        {
        case 0: DoScriptText(SAY_SLAY1, m_creature); break;
        case 1: DoScriptText(SAY_SLAY2, m_creature); break;
        }
    }

    void JustDied(Unit *victim)
    {
        if(pInstance)
        {
            DoSpawnCreature(CREATURE_INVISIBLE_ANNOUNCER,0,0,0,0, TEMPSUMMON_TIMED_DESPAWN, 30000);
            pInstance->SetData(DATA_HIGHWARLORDNAJENTUSEVENT, DONE);
        }

        DoScriptText(SAY_DEATH, m_creature);
    }

    bool hasShield()
    {
        return m_creature->HasAura(SPELL_TIDAL_SHIELD, 0);
    }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if(spell->Id == SPELL_HURL_SPINE && hasShield())
        {
            m_creature->RemoveAurasDueToSpell(SPELL_TIDAL_SHIELD);
            m_creature->CastSpell(m_creature, SPELL_TIDAL_BURST, true);
            NeedleSpineTimer = 10000;
            ImpalingSpineTimer = TIMER_IMPALING_SPINE;
        }
    }

    void EnterCombat(Unit *who)
    {
        if(pInstance)
            pInstance->SetData(DATA_HIGHWARLORDNAJENTUSEVENT, IN_PROGRESS);

        DoScriptText(SAY_AGGRO, m_creature);
        DoZoneInCombat();
    }

    bool RemoveImpalingSpine()
    {
        if(!SpineTargetGUID) return false;
        Unit* target = Unit::GetUnit(*m_creature, SpineTargetGUID);
        if(target && target->HasAura(SPELL_IMPALING_SPINE, 1))
            target->RemoveAurasDueToSpell(SPELL_IMPALING_SPINE);
        SpineTargetGUID=0;
        return true;
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if(TidalShieldTimer < diff)
        {
            m_creature->CastSpell(m_creature, SPELL_TIDAL_SHIELD, true);
            TidalShieldTimer = TIMER_TIDAL_SHIELD;
        }else TidalShieldTimer -= diff;

        DoMeleeAttackIfReady();

        if(hasShield())
            return;

        if(!m_creature->HasAura(SPELL_BERSERK, 0))
        {
            if(EnrageTimer < diff)
            {
                DoScriptText(SAY_ENRAGE2, m_creature);
                m_creature->CastSpell(m_creature, SPELL_BERSERK, true);
            }else EnrageTimer -= diff;
        }

        if(NeedleSpineTimer < diff)
        {
            //m_creature->CastSpell(m_creature, SPELL_NEEDLE_SPINE, true);
            std::list<Unit*> target;
            SelectUnitList(target, 3, SELECT_TARGET_RANDOM, 80.0f, true);
            for(std::list<Unit*>::iterator i = target.begin(); i != target.end(); ++i)
                me->CastSpell(*i, SPELL_NEEDLE_SPINE2, true);
            NeedleSpineTimer = TIMER_NEEDLE_SPINE;
        }else NeedleSpineTimer -= diff;

        if(SpecialYellTimer < diff)
        {
            switch(rand()%2)
            {
            case 0: DoScriptText(SAY_SPECIAL1, m_creature); break;
            case 1: DoScriptText(SAY_SPECIAL2, m_creature); break;
            }
            SpecialYellTimer = TIMER_SPECIAL_YELL;
        }else SpecialYellTimer -= diff;

        if(ImpalingSpineTimer < diff)
        {
            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 80.0, true, true);
            if(!target) target = m_creature->GetVictim();
            if(target)
            {
                m_creature->CastSpell(target, SPELL_IMPALING_SPINE, true);
                SpineTargetGUID = target->GetGUID();
                //must let target summon, otherwise you cannot click the spine
                target->SummonGameObject(GOBJECT_SPINE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), m_creature->GetOrientation(), 0, 0, 0, 0, 30);

                switch(rand()%2)
                {
                case 0: DoScriptText(SAY_NEEDLE1, m_creature); break;
                case 1: DoScriptText(SAY_NEEDLE2, m_creature); break;
                }
                ImpalingSpineTimer = TIMER_IMPALING_SPINE;
            }
        }else ImpalingSpineTimer -= diff;
    }
};

bool GOHello_go_najentus_spine(Player *player, GameObject* _GO)
{
    if(ScriptedInstance* pInstance = (ScriptedInstance*)_GO->GetInstanceData())
        if(Creature* Najentus = Unit::GetCreature(*_GO, pInstance->GetData64(DATA_HIGHWARLORDNAJENTUS)))
            if(((boss_najentusAI*)Najentus->AI())->RemoveImpalingSpine())
            {
                player->CastSpell(player, SPELL_CREATE_NAJENTUS_SPINE, true);
                _GO->SetLootState(GO_NOT_READY);
                _GO->SetRespawnTime(604800); //one week respawn
                _GO->RemoveFromWorld();
            }
    return true;
}

CreatureAI* GetAI_boss_najentus(Creature *_Creature)
{
    return new boss_najentusAI (_Creature);
}

void AddSC_boss_najentus()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_najentus";
    newscript->GetAI = &GetAI_boss_najentus;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_najentus_spine";
    newscript->pGOHello = &GOHello_go_najentus_spine;
    newscript->RegisterSelf();
}

