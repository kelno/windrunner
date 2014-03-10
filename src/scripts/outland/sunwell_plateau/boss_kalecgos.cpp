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
SDName: Boss_Kalecgos
SD%Complete: 95
SDComment:
SDCategory: Sunwell_Plateau
EndScriptData */

#include "precompiled.h"
#include "def_sunwell_plateau.h"

enum Quotes
{
    //Kalecgos dragon form
    SAY_EVIL_AGGRO          = -1580000,
    SAY_EVIL_SPELL1         = -1580001,
    SAY_EVIL_SPELL2         = -1580002,
    SAY_EVIL_SLAY1          = -1580003,
    SAY_EVIL_SLAY2          = -1580004,
    SAY_EVIL_ENRAGE         = -1580005,

    //Kalecgos humanoid form
    SAY_GOOD_AGGRO          = -1580006,
    SAY_GOOD_NEAR_DEATH     = -1580007,
    SAY_GOOD_NEAR_DEATH2    = -1580008,
    SAY_GOOD_PLRWIN         = -1580009,

    //Sathrovarr
    SAY_SATH_AGGRO          = -1580010,
    SAY_SATH_DEATH          = -1580011,
    SAY_SATH_SPELL1         = -1580012,
    SAY_SATH_SPELL2         = -1580013,
    SAY_SATH_SLAY1          = -1580014,
    SAY_SATH_SLAY2          = -1580015,
    SAY_SATH_ENRAGE         = -1580016
};

enum SpellIds
{
    AURA_SUNWELL_RADIANCE       =   45769,
    AURA_SPECTRAL_EXHAUSTION    =   44867,
    AURA_SPECTRAL_REALM         =   46021,
    AURA_SPECTRAL_INVISIBILITY  =   44801,
    AURA_DEMONIC_VISUAL         =   44800,

    SPELL_SPECTRAL_BLAST        =   44869,
    SPELL_TELEPORT_SPECTRAL     =   46019,
    SPELL_ARCANE_BUFFET         =   45018,
    SPELL_FROST_BREATH          =   44799,
    SPELL_TAIL_LASH             =   45122,

    SPELL_BANISH                =   44836,
    SPELL_TRANSFORM_KALEC       =   44670,
    SPELL_ENRAGE                =   44807,

    SPELL_CORRUPTION_STRIKE     =   45029,
    SPELL_AGONY_CURSE           =   45032,
    SPELL_SHADOW_BOLT           =   45031,

    SPELL_HEROIC_STRIKE         =   29426/*45026*/,
    SPELL_REVITALIZE            =   45027
};

#define GO_FAILED   "Vous ne pouvez pas faire cela maintenant."

#define FLY_X   1679
#define FLY_Y   900
#define FLY_Z   82

#define CENTER_X    1705
#define CENTER_Y    930
#define RADIUS      30

#define DRAGON_REALM_Z  53.079
#define DEMON_REALM_Z   -74.558

uint32 WildMagic[]= { 44978, 45001, 45002, 45004, /*45006, */45010 };

struct boss_kalecgosAI : public ScriptedAI
{
    boss_kalecgosAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        SathGUID = 0;
        ForceFieldGUID = 0;
        Wall1GUID = 0;
        Wall2GUID = 0;
        pulledOnce = false;
        hasEnded = true;
        
        SpellEntry *TempSpell = GET_SPELL(SPELL_SPECTRAL_BLAST);
        if (TempSpell)
            TempSpell->EffectImplicitTargetB[0] = TARGET_UNIT_TARGET_ENEMY;
    }

    ScriptedInstance *pInstance;

    uint32 ArcaneBuffetTimer;
    uint32 FrostBreathTimer;
    uint32 WildMagicTimer;
    uint32 SpectralBlastTimer;
    uint32 TailLashTimer;
    uint32 CheckTimer;
    uint32 TalkTimer;
    uint32 TalkSequence;
    uint32 CloseDoorsTimer;

    bool isFriendly;
    bool isEnraged;
    bool isBanished;
    bool pulledOnce;
    bool hasEnded;

    uint64 SathGUID;
    uint64 ForceFieldGUID;
    uint64 Wall1GUID;
    uint64 Wall2GUID;

    void Reset()
    {
        if(pInstance)
        {
            SathGUID = pInstance->GetData64(DATA_SATHROVARR);
            ForceFieldGUID = pInstance->GetData64(DATA_GO_FORCEFIELD);
            Wall1GUID = pInstance->GetData64(DATA_GO_KALEC_WALL_1);
            Wall2GUID = pInstance->GetData64(DATA_GO_KALEC_WALL_2);
        }

        Unit *Sath = Unit::GetUnit(*m_creature,SathGUID);
        if(Sath) {
            (Sath->ToCreature())->AI()->EnterEvadeMode();
            Sath->ToCreature()->SetReactState(REACT_PASSIVE);
        }

        GameObject *Door = GameObject::GetGameObject(*m_creature, ForceFieldGUID);
        if (Door)
            Door->SetGoState(GO_STATE_ACTIVE);
        GameObject *Wall1 = GameObject::GetGameObject(*m_creature, Wall1GUID);
        if (Wall1 && m_creature->IsAlive())
            Wall1->SetGoState(GO_STATE_READY);
        else if (Wall1 && m_creature->isDead())
            Wall1->SetGoState(GO_STATE_ACTIVE);
        GameObject *Wall2 = GameObject::GetGameObject(*m_creature, Wall2GUID);
        if (Wall2)
            Wall2->SetGoState(GO_STATE_ACTIVE);

        m_creature->setFaction(14);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE + UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetDisableGravity(false);
        m_creature->SetVisibility(VISIBILITY_ON);
        m_creature->SetStandState(PLAYER_STATE_SLEEP);
        m_creature->SetReactState(REACT_AGGRESSIVE);

        ArcaneBuffetTimer = 8000;
        FrostBreathTimer = 15000;
        WildMagicTimer = 10000;
        TailLashTimer = 25000;
        SpectralBlastTimer = 20000+(rand()%5000);
        CheckTimer = SpectralBlastTimer+20000; //after spectral blast
        CloseDoorsTimer = 0;

        TalkTimer = 0;
        TalkSequence = 0;
        isFriendly = false;
        isEnraged = false;
        isBanished = false;
            
        if (pInstance && pInstance->GetData(DATA_KALECGOS_EVENT) == DONE) {
            me->setFaction(35);
            me->SetVisibility(VISIBILITY_OFF);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            
            GameObject *Door = GameObject::GetGameObject(*m_creature, pInstance->GetData64(DATA_GO_FORCEFIELD));
            if(Door) Door->SetGoState(GO_STATE_ACTIVE);
            GameObject *Wall1 = GameObject::GetGameObject(*m_creature, pInstance->GetData64(DATA_GO_KALEC_WALL_1));
            if(Wall1) Wall1->SetGoState(GO_STATE_ACTIVE);
            GameObject *Wall2 = GameObject::GetGameObject(*m_creature, pInstance->GetData64(DATA_GO_KALEC_WALL_2));
            if(Wall2) Wall2->SetGoState(GO_STATE_ACTIVE);
        }
        
        // Raid wipe
        /*if (!hasEnded && pulledOnce) {
            TalkTimer = 1;
            isFriendly = false;
        }*/
    }
    
    void EnterEvadeMode()
    {
        /*if (!hasEnded) {
            TalkTimer = 1;
            isFriendly = false;
        }*/
        
        if (m_creature->IsInCombat() && m_creature->GetVisibility() == VISIBILITY_ON) {
            TalkSequence = 0;
            TalkTimer = 1;
            isFriendly = false;
            m_creature->SetReactState(REACT_PASSIVE);
            
            return;
        }
        
        ScriptedAI::EnterEvadeMode();
    }

    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if(damage >= m_creature->GetHealth() && done_by != m_creature)
            damage = 0;
    }

    void EnterCombat(Unit* who)
    {
        m_creature->SetStandState(PLAYER_STATE_NONE);
        DoScriptText(SAY_EVIL_AGGRO, m_creature);
        GameObject *Door = GameObject::GetGameObject(*m_creature, ForceFieldGUID);
        if(Door) Door->SetGoState(GO_STATE_READY);
        GameObject *Wall1 = GameObject::GetGameObject(*m_creature, Wall1GUID);
        if(Wall1) Wall1->SetGoState(GO_STATE_READY);
        GameObject *Wall2 = GameObject::GetGameObject(*m_creature, Wall2GUID);
        if(Wall2) Wall2->SetGoState(GO_STATE_READY);
        DoZoneInCombat();
        CloseDoorsTimer = 5000;
        
        pulledOnce = true;
        hasEnded = false;

        if(pInstance)
            pInstance->SetData(DATA_KALECGOS_EVENT, IN_PROGRESS);
            
        Unit *Sath = Unit::GetUnit(*m_creature,SathGUID);
        if(Sath)
            Sath->ToCreature()->SetReactState(REACT_AGGRESSIVE);
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(RAND(SAY_EVIL_SLAY1, SAY_EVIL_SLAY2), m_creature);
    }
    
    void MoveInLineOfSight(Unit *pWho) {
        if (pWho->GetTypeId() == TYPEID_PLAYER && m_creature->GetDistance(pWho) <= 30.0f && !m_creature->IsInCombat() && me->getFaction() != 35)
            AttackStart(pWho);
    }

    void MovementInform(uint32 type,uint32 id)
    {
        if (type != POINT_MOTION_TYPE)
            return;
            
        if (id != 1)
            return;
        
        m_creature->SetVisibility(VISIBILITY_OFF);
        if(isFriendly)
            m_creature->setDeathState(JUST_DIED);
        else
        {
            m_creature->GetMotionMaster()->MoveTargetedHome();
            TalkTimer = 30000;
        }
    }

    void GoodEnding()
    {
        switch(TalkSequence)
        {
        case 1:
            m_creature->setFaction(35);
            if (pInstance)
                pInstance->SetData(DATA_KALECGOS_EVENT, DONE);
            TalkTimer = 1000;
            break;
        case 2:
            DoScriptText(SAY_GOOD_PLRWIN, m_creature);
            TalkTimer = 10000;
            break;
        case 3:
            m_creature->SetDisableGravity(true);
            m_creature->GetMotionMaster()->Clear();
            m_creature->GetMotionMaster()->MovePoint(1,FLY_X,FLY_Y,FLY_Z);
            TalkTimer = 600000;
            hasEnded = true;
            break;
        default:
            break;
        }
    }

    void BadEnding()
    {
        switch(TalkSequence)
        {
        case 1:
            DoScriptText(SAY_EVIL_ENRAGE, m_creature);
            TalkTimer = 3000;
            break;
        case 2:
            m_creature->SetDisableGravity(true);
            m_creature->GetMotionMaster()->Clear();
            m_creature->GetMotionMaster()->MovePoint(1,FLY_X,FLY_Y,FLY_Z);
            m_creature->DeleteThreatList();
            TalkTimer = 600000;
            break;
        case 3:
            hasEnded = true;
            EnterEvadeMode();
            break;
        default:
            break;
        }
    }
    
    void DeleteFromThreatList(uint64 TargetGUID)
    {
        for(std::list<HostilReference*>::iterator itr = m_creature->getThreatManager().getThreatList().begin(); itr != m_creature->getThreatManager().getThreatList().end(); ++itr)
        {
            if((*itr)->getUnitGuid() == TargetGUID)
            {
                (*itr)->removeReference();
                break;
            }
        }
    }

    void UpdateAI(const uint32 diff);
};

struct boss_sathrovarrAI : public ScriptedAI
{
    boss_sathrovarrAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        KalecGUID = 0;
        KalecgosGUID = 0;
    }

    ScriptedInstance *pInstance;

    uint32 CorruptionStrikeTimer;
    uint32 AgonyCurseTimer;
    uint32 ShadowBoltTimer;
    uint32 CheckTimer;
    uint32 ResetThreat;

    uint64 KalecGUID;
    uint64 KalecgosGUID;

    bool isEnraged;
    bool isBanished;

    void Reset()
    {
        if(pInstance)
            KalecgosGUID = pInstance->GetData64(DATA_KALECGOS_DRAGON);

        if(KalecGUID)
        {
            if(Unit* Kalec = Unit::GetUnit(*m_creature, KalecGUID))
                Kalec->setDeathState(JUST_DIED);
            KalecGUID = 0;
        }

        ShadowBoltTimer = 7000 + rand()%3 * 1000;
        AgonyCurseTimer = 20000;
        CorruptionStrikeTimer = 13000;
        CheckTimer = 1000;
        ResetThreat = 1000;
        isEnraged = false;
        isBanished = false;

        if(pInstance && pInstance->GetData(DATA_KALECGOS_EVENT) != DONE)
            pInstance->SetData(DATA_KALECGOS_EVENT, NOT_STARTED);
    }

    void EnterCombat(Unit* who)
    {
        Creature *Kalec = m_creature->SummonCreature(MOB_KALEC, m_creature->GetPositionX() + 10, m_creature->GetPositionY() + 5, m_creature->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0);
        if(Kalec)
        {
            KalecGUID = Kalec->GetGUID();
            m_creature->CombatStart(Kalec);
            m_creature->AddThreat(Kalec, 100.0f);
        }
        DoScriptText(SAY_SATH_AGGRO, m_creature);
    }

    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if(damage >= m_creature->GetHealth() && done_by != m_creature)
            damage = 0;
    }

    void KilledUnit(Unit *target)
    {
        if(target->GetGUID() == KalecGUID)
        {
            TeleportAllPlayersBack();
            if(Unit *Kalecgos = Unit::GetUnit(*m_creature, KalecgosGUID))
            {
                ((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->TalkTimer = 1;
                ((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->isFriendly = false;
                ((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->TalkSequence = 0;
            }
            EnterEvadeMode();
            return;
        }
        DoScriptText(RAND(SAY_SATH_SLAY1, SAY_SATH_SLAY2), m_creature);
    }

    void JustDied(Unit *killer)
    {
        DoScriptText(SAY_SATH_DEATH, m_creature);
        m_creature->Relocate(m_creature->GetPositionX(), m_creature->GetPositionY(), DRAGON_REALM_Z, m_creature->GetOrientation());
        WorldPacket data;                       //send update position to client
        m_creature->BuildHeartBeatMsg(&data);
        m_creature->SendMessageToSet(&data,true);
        
        TeleportAllPlayersBack();
        if(Unit *Kalecgos = Unit::GetUnit(*m_creature, KalecgosGUID))
        {
            ((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->TalkTimer = 1;
            ((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->isFriendly = true;
        }
        if(pInstance)
            pInstance->SetData(DATA_KALECGOS_EVENT, DONE);
            
        GameObject *Door = GameObject::GetGameObject(*m_creature, pInstance->GetData64(DATA_GO_FORCEFIELD));
        if(Door) Door->SetGoState(GO_STATE_ACTIVE);
        GameObject *Wall1 = GameObject::GetGameObject(*m_creature, pInstance->GetData64(DATA_GO_KALEC_WALL_1));
        if(Wall1) Wall1->SetGoState(GO_STATE_ACTIVE);
        GameObject *Wall2 = GameObject::GetGameObject(*m_creature, pInstance->GetData64(DATA_GO_KALEC_WALL_2));
        if(Wall2) Wall2->SetGoState(GO_STATE_ACTIVE);
            
        // Remove invisibility auras
        m_creature->RemoveAurasDueToSpell(44800);
        m_creature->RemoveAurasDueToSpell(44801);
    }

    void TeleportAllPlayersBack()
    {
        Map *map = m_creature->GetMap();
        if(!map->IsDungeon()) return;
        Map::PlayerList const &PlayerList = map->GetPlayers();
        Map::PlayerList::const_iterator i;
        for(i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if(Player* i_pl = i->getSource()) {
                i_pl->RemoveAurasDueToSpell(SPELL_AGONY_CURSE);
                if(i_pl->HasAura(AURA_SPECTRAL_REALM))
                    i_pl->RemoveAurasDueToSpell(AURA_SPECTRAL_REALM);
            }
    }
    
    void DeleteFromThreatList(uint64 TargetGUID)
    {
        for(std::list<HostilReference*>::iterator itr = m_creature->getThreatManager().getThreatList().begin(); itr != m_creature->getThreatManager().getThreatList().end(); ++itr)
        {
            if((*itr)->getUnitGuid() == TargetGUID)
            {
                (*itr)->removeReference();
                break;
            }
        }
    }
    
    void DoMeleeAttackIfReady(bool withCorruptionStrike)
    {
        if (withCorruptionStrike)
            DoCast(m_creature->GetVictim(), SPELL_CORRUPTION_STRIKE, true);
            
        UnitAI::DoMeleeAttackIfReady();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;
            
        // Check LoS EVERY update, maybe the current target was teleported back
        if (m_creature->GetVictim()->GetPositionZ() >= -65.0f || !m_creature->GetVictim()->IsWithinLOSInMap(m_creature)) {
            DeleteFromThreatList(m_creature->GetVictim()->GetGUID());
            if(KalecGUID) {
                if(Unit* Kalec = Unit::GetUnit(*m_creature, KalecGUID))
                    m_creature->AI()->AttackStart(Kalec);
            }
        }
            
        // If tank has not the aura anymore, maybe he was teleported back -> start attack on Kalecgos human form
        if (m_creature->GetVictim()->GetTypeId() == TYPEID_PLAYER && !m_creature->GetVictim()->HasAura(AURA_SPECTRAL_REALM)) {
            if(KalecGUID) {
                if(Unit* Kalec = Unit::GetUnit(*m_creature, KalecGUID))
                    m_creature->AI()->AttackStart(Kalec);
            }
        }

        if(CheckTimer < diff)
        {
            if (((m_creature->GetHealth()*100 / m_creature->GetMaxHealth()) < 10) && !isEnraged)
            {
                Unit* Kalecgos = Unit::GetUnit(*m_creature, KalecgosGUID);
                if(Kalecgos)
                {
                    //Kalecgos->CastSpell(Kalecgos, SPELL_ENRAGE, true);
                    Kalecgos->AddAura(SPELL_ENRAGE, Kalecgos);
                    ((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->isEnraged = true;
                }
                //DoCast(m_creature, SPELL_ENRAGE, true);
                me->AddAura(SPELL_ENRAGE, me);
                isEnraged = true;
            }

            if(!isBanished && (m_creature->GetHealth()*100)/m_creature->GetMaxHealth() < 1)
            {
                if(Unit *Kalecgos = Unit::GetUnit(*m_creature, KalecgosGUID))
                {
                    if(((boss_kalecgosAI*)(Kalecgos->ToCreature())->AI())->isBanished)
                    {
                        m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        return;
                    }
                    else
                    {
                        DoCast(m_creature, SPELL_BANISH);
                        isBanished = true;
                    }
                }
                else
                {
                    DoTextEmote("is unable to find Kalecgos", NULL);
                    EnterEvadeMode();
                    return;
                }
            }
            CheckTimer = 1000;
        }else CheckTimer -= diff;

        if(ResetThreat < diff)
        {
            if (( m_creature->GetVictim()->HasAura(AURA_SPECTRAL_EXHAUSTION)) && (m_creature->GetVictim()->GetTypeId() == TYPEID_PLAYER))
            {
                for(std::list<HostilReference*>::iterator itr = m_creature->getThreatManager().getThreatList().begin(); itr != m_creature->getThreatManager().getThreatList().end(); ++itr)
                {
                    if(((*itr)->getUnitGuid()) ==  (m_creature->GetVictim()->GetGUID()))
                    {
                        (*itr)->removeReference();
                        break;
                    }
                }
            }
            ResetThreat = 1000;
        }else ResetThreat -= diff;

        if(ShadowBoltTimer < diff)
        {
            DoScriptText(SAY_SATH_SPELL1, m_creature);
            DoCast(m_creature, SPELL_SHADOW_BOLT);
            ShadowBoltTimer = 7000+(rand()%3000);
        }else ShadowBoltTimer -= diff;

        if(AgonyCurseTimer < diff)
        {
            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
            if(!target) target = m_creature->GetVictim();
            DoCast(target, SPELL_AGONY_CURSE);
            AgonyCurseTimer = 20000;
        }else AgonyCurseTimer -= diff;

        if(CorruptionStrikeTimer < diff)
        {
            DoScriptText(SAY_SATH_SPELL2, m_creature);
            DoMeleeAttackIfReady(true);
            CorruptionStrikeTimer = 13000;
        }else CorruptionStrikeTimer -= diff;

        DoMeleeAttackIfReady(false);
    }
};

struct boss_kalecAI : public ScriptedAI
{
    ScriptedInstance *pInstance;

    uint32 RevitalizeTimer;
    uint32 HeroicStrikeTimer;
    uint32 YellTimer;
    uint32 YellSequence;

    uint64 SathGUID;

    bool isEnraged; // if demon is enraged

    boss_kalecAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    void Reset()
    {
        if(pInstance)
            SathGUID = pInstance->GetData64(DATA_SATHROVARR);

        RevitalizeTimer = 5000;
        HeroicStrikeTimer = 3000;
        YellTimer = 5000;
        YellSequence = 0;

        isEnraged = false;
    }

    void EnterCombat(Unit* who) {}
    
    void HealReceived(Unit* done_by, uint32& addhealth)
    {
        addhealth = 0;
    }

    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if (done_by->ToPlayer() && done_by->ToPlayer()->isGameMaster())
            return;
        if(done_by->GetGUID() != SathGUID)
            damage = 0;
        else if(isEnraged)
            damage *= 3;
        if (done_by->GetGUID() == SathGUID)
            damage *= 1.3f;
        if (damage >= m_creature->GetHealth() && done_by->ToCreature() && done_by->GetGUID() == SathGUID)
            done_by->ToCreature()->AI()->KilledUnit(m_creature);
    }
    
    void JustDied(Unit *pKiller)
    {
        TeleportAllPlayersBack();
    }
    
    void TeleportAllPlayersBack()
    {
        Map *map = m_creature->GetMap();
        if(!map->IsDungeon()) return;
        Map::PlayerList const &PlayerList = map->GetPlayers();
        Map::PlayerList::const_iterator i;
        for(i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if(Player* i_pl = i->getSource()) {
                i_pl->RemoveAurasDueToSpell(SPELL_AGONY_CURSE);
                if(i_pl->HasAura(AURA_SPECTRAL_REALM))
                    i_pl->RemoveAurasDueToSpell(AURA_SPECTRAL_REALM);
            }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if(YellTimer < diff)
        {
            switch(YellSequence)
            {
            case 0:
                DoScriptText(SAY_GOOD_AGGRO, m_creature);
                YellSequence++;
                break;
            case 1:
                if((m_creature->GetHealth()*100)/m_creature->GetMaxHealth() < 50)
                {
                    DoScriptText(SAY_GOOD_NEAR_DEATH, m_creature);
                    YellSequence++;
                }
                break;
            case 2:
                if((m_creature->GetHealth()*100)/m_creature->GetMaxHealth() < 10)
                {
                    DoScriptText(SAY_GOOD_NEAR_DEATH2, m_creature);
                    YellSequence++;
                }
                break;
            default:
                break;
            }
            YellTimer = 5000;
        }

        if(RevitalizeTimer < diff)
        {
            me->InterruptNonMeleeSpells(false);
            DoCast(m_creature, SPELL_REVITALIZE);
            RevitalizeTimer = 5000;
        }else RevitalizeTimer -= diff;

        if(HeroicStrikeTimer < diff)
        {
            DoCast(m_creature->GetVictim(), SPELL_HEROIC_STRIKE);
            HeroicStrikeTimer = 2000;
        }else HeroicStrikeTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

void boss_kalecgosAI::UpdateAI(const uint32 diff)
{
    if(TalkTimer)
    {
        if(!TalkSequence)
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE + UNIT_FLAG_NOT_SELECTABLE);
            m_creature->InterruptNonMeleeSpells(true);
            if (isFriendly)
                m_creature->RemoveAurasDueToSpell(SPELL_ENRAGE);
            m_creature->SetHealth(m_creature->GetMaxHealth());
            m_creature->RemoveAllAuras();
            m_creature->DeleteThreatList();
            m_creature->CombatStop();
            GameObject *Door = GameObject::GetGameObject(*m_creature, ForceFieldGUID);
            if(Door) Door->SetGoState(GO_STATE_ACTIVE);
            GameObject *Wall1 = GameObject::GetGameObject(*m_creature, Wall1GUID);
            if(Wall1) Wall1->SetGoState(!isFriendly);
            GameObject *Wall2 = GameObject::GetGameObject(*m_creature, Wall2GUID);
            if(Wall2) Wall2->SetGoState(GO_STATE_ACTIVE);
            TalkSequence++;
        }
        if(TalkTimer <= diff)
        {
            if(isFriendly)
                GoodEnding();
            else
                BadEnding();
            TalkSequence++;
        }else TalkTimer -= diff;
    }
    else
    {        
        if (!UpdateVictim())
            return;
            
        if (m_creature->GetDistance(1704.22, 924.758, 53.1608) > 35.0f) {
            EnterEvadeMode();
            return;
        }
            
        // Check LoS EVERY update, maybe the current target was teleported in the spectral realm
        if (m_creature->GetVictim()->GetPositionZ() <= 52.5f || !m_creature->GetVictim()->IsWithinLOSInMap(m_creature)) 
        {
            DeleteFromThreatList(m_creature->GetVictim()->GetGUID());
            UpdateVictim();
        }

        if(CheckTimer < diff)
         {
             if (((m_creature->GetHealth()*100 / m_creature->GetMaxHealth()) < 10) && !isEnraged)
             {
                 Unit* Sath = Unit::GetUnit(*m_creature, SathGUID);
                 if(Sath)
                 {
                     //Sath->CastSpell(Sath, SPELL_ENRAGE, true);
                     Sath->AddAura(SPELL_ENRAGE, Sath);
                     ((boss_sathrovarrAI*)(Sath->ToCreature())->AI())->isEnraged = true;
                 }
                 //DoCast(m_creature, SPELL_ENRAGE, true);
                 me->AddAura(SPELL_ENRAGE, me);
                 isEnraged = true;
             }

             if(!isBanished && (m_creature->GetHealth()*100)/m_creature->GetMaxHealth() < 1)
             {
                 if(Unit *Sath = Unit::GetUnit(*m_creature, SathGUID))
                 {
                     if(((boss_sathrovarrAI*)(Sath->ToCreature())->AI())->isBanished)
                     {
                         Sath->DealDamage(Sath, Sath->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                         return;
                     }
                     else
                     {
                         DoCast(m_creature, SPELL_BANISH);
                         isBanished = true;
                     }
                 }
                 else
                 {
                     error_log("TSCR: Didn't find Shathrowar. Kalecgos event reseted.");
                     EnterEvadeMode();
                     return;
                 }
             }
             CheckTimer = 1000;
        }else CheckTimer -= diff;

        if(ArcaneBuffetTimer < diff)
        {
            DoCastAOE(SPELL_ARCANE_BUFFET);
            ArcaneBuffetTimer = 8000;
        }else ArcaneBuffetTimer -= diff;

        if(FrostBreathTimer < diff)
        {
            DoCastAOE(SPELL_FROST_BREATH);
            FrostBreathTimer = 15000;
        }else FrostBreathTimer -= diff;

        if(TailLashTimer < diff)
        {
            DoCastAOE(SPELL_TAIL_LASH);
            TailLashTimer = 15000;
        }else TailLashTimer -= diff;

        if(WildMagicTimer < diff)
        {
            DoCastAOE(WildMagic[rand()%5]);
            WildMagicTimer = 20000;
        }else WildMagicTimer -= diff;

        if(SpectralBlastTimer < diff)
        {
            Unit *target = SelectUnit(1, 0.0f, 50.0f, true, true, false, AURA_SPECTRAL_EXHAUSTION, 0);
            
            if (!target || (target && (target->isDead() || target->GetGUIDLow() == m_creature->GetVictim()->GetGUIDLow() || target->GetPositionZ() <= 52.5f || target->HasAura(AURA_SPECTRAL_EXHAUSTION))))        // Delay selection to next loop if no valid target found
                SpectralBlastTimer = 300;
            else if (target) {
                m_creature->InterruptNonMeleeSpells(true);
                DoCast(target, SPELL_SPECTRAL_BLAST);
                if (target->ToPlayer() && target->ToPlayer()->GetPet()) {
                    Pet *pet = target->ToPlayer()->GetPet();
                    target->CastSpell(pet, SPELL_TELEPORT_SPECTRAL, true);
                    target->AddAura(AURA_SPECTRAL_REALM, pet);
                    pet->Relocate(pet->GetPositionX(), pet->GetPositionY(), pet->GetPositionZ(), pet->GetOrientation());
                }
                DoModifyThreatPercent(target, -100);    // Reset threat so Kalecgos does not follow the player in spectral realm :)
                target->RemoveAurasDueToSpell(SPELL_ARCANE_BUFFET); // FIXME: I'm not sure this is blizzlike
                if (target->HasAura(AURA_SPECTRAL_EXHAUSTION)) {
                    target->RemoveAurasDueToSpell(AURA_SPECTRAL_EXHAUSTION);    // FIXME: If this happens, this is a bug.
                    sLog.outError("Sunwell Plateau/Kalecgos: Spectral Blast target (guid %u) had Spectral exhaustion when teleported!", target->GetGUIDLow());
                }
                SpectralBlastTimer = 20000+(rand()%5000);
            }
        }else SpectralBlastTimer -= diff;

        DoMeleeAttackIfReady();
    }
}

bool GOkalecgos_teleporter(Player *player, GameObject* _GO)
{
    if(player->HasAura(AURA_SPECTRAL_EXHAUSTION))
        player->GetSession()->SendNotification(GO_FAILED);
    else {
        player->CastSpell(player, SPELL_TELEPORT_SPECTRAL, true);
        if (player->GetPet()) {
                player->GetPet()->CastSpell(player->GetPet(), SPELL_TELEPORT_SPECTRAL, true);
                player->AddAura(AURA_SPECTRAL_REALM, player->GetPet());
            }
        player->RemoveAurasDueToSpell(SPELL_ARCANE_BUFFET);
        if (player->HasAura(AURA_SPECTRAL_EXHAUSTION)) {
            player->RemoveAurasDueToSpell(AURA_SPECTRAL_EXHAUSTION);    // FIXME: If this happens, this is a bug.
            sLog.outError("Sunwell Plateau/Kalecgos: Spectral Blast target (guid %u) had Spectral exhaustion when teleported VIA PORTAL!", player->GetGUIDLow());
        }
    }
    return true;
}

CreatureAI* GetAI_boss_kalecgos(Creature *_Creature)
{
    return new boss_kalecgosAI (_Creature);
}

CreatureAI* GetAI_boss_Sathrovarr(Creature *_Creature)
{
    return new boss_sathrovarrAI (_Creature);
}

CreatureAI* GetAI_boss_kalec(Creature *_Creature)
{
    return new boss_kalecAI (_Creature);
}

void AddSC_boss_kalecgos()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_kalecgos";
    newscript->GetAI = &GetAI_boss_kalecgos;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_sathrovarr";
    newscript->GetAI = &GetAI_boss_Sathrovarr;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_kalec";
    newscript->GetAI = &GetAI_boss_kalec;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="kalecgos_teleporter";
    newscript->pGOHello = &GOkalecgos_teleporter;
    newscript->RegisterSelf();
}
