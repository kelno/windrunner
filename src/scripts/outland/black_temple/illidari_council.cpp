/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Illidari_Council
SD%Complete: 95
SDComment: Circle of Healing not working properly.
SDCategory: Black Temple
EndScriptData */

#include "precompiled.h"
#include "def_black_temple.h"
#include "Spell.h"

//Speech'n'Sounds
#define SAY_GATH_SLAY           -1564085
#define SAY_GATH_SLAY_COMNT     -1564089
#define SAY_GATH_DEATH          -1564093
#define SAY_GATH_SPECIAL1       -1564077
#define SAY_GATH_SPECIAL2       -1564081

#define SAY_VERA_SLAY           -1564086
#define SAY_VERA_COMNT          -1564089
#define SAY_VERA_DEATH          -1564094
#define SAY_VERA_SPECIAL1       -1564078
#define SAY_VERA_SPECIAL2       -1564082

#define SAY_MALA_SLAY           -1564087
#define SAY_MALA_COMNT          -1564090
#define SAY_MALA_DEATH          -1564095
#define SAY_MALA_SPECIAL1       -1564079
#define SAY_MALA_SPECIAL2       -1564083

#define SAY_ZERE_SLAY           -1564088
#define SAY_ZERE_COMNT          -1564091
#define SAY_ZERE_DEATH          -1564096
#define SAY_ZERE_SPECIAL1       -1564080
#define SAY_ZERE_SPECIAL2       -1564084

#define ERROR_INST_DATA           "SD2 ERROR: Instance Data for Black Temple not set properly; Illidari Council event will not function properly."

#define AKAMAID                 23089

struct CouncilYells
{
    int32 entry;
    uint32 timer;
};

static CouncilYells CouncilAggro[]=
{
    {-1564069, 5000},                                       // Gathios
    {-1564070, 5500},                                       // Veras
    {-1564071, 5000},                                       // Malande
    {-1564072, 0},                                          // Zerevor
};

// Need to get proper timers for this later
static CouncilYells CouncilEnrage[]=
{
    {-1564073, 2000},                                       // Gathios
    {-1564074, 6000},                                       // Veras
    {-1564075, 5000},                                       // Malande
    {-1564076, 0},                                          // Zerevor
};

// High Nethermancer Zerevor's spells
#define SPELL_FLAMESTRIKE          41481
#define SPELL_BLIZZARD             41482
#define SPELL_ARCANE_BOLT          41483
#define SPELL_ARCANE_EXPLOSION     41524
#define SPELL_DAMPEN_MAGIC         41478

// Lady Malande's spells
#define SPELL_EMPOWERED_SMITE      41471
#define SPELL_CIRCLE_OF_HEALING    41455
#define SPELL_REFLECTIVE_SHIELD    41475
#define SPELL_DIVINE_WRATH         41472
#define SPELL_HEAL_VISUAL          24171

// Gathios the Shatterer's spells
#define SPELL_BLESS_PROTECTION     41450
#define SPELL_BLESS_SPELLWARD      41451
#define SPELL_CONSECRATION         41541
#define SPELL_HAMMER_OF_JUSTICE    41468
#define SPELL_SEAL_OF_COMMAND      41469
#define SPELL_JUDGEMENT_OF_COMMAND 41470
#define SPELL_SEAL_OF_BLOOD        41459
#define SPELL_JUDGEMENT_OF_BLOOD   41461
#define SPELL_CHROMATIC_AURA       41453
#define SPELL_DEVOTION_AURA        41452

// Veras Darkshadow's spells
#define SPELL_DEADLY_POISON        41485
#define SPELL_DEADLY_POISON_TRIGGER 41480
#define SPELL_ENVENOM              41487
#define SPELL_VANISH               41479

#define SPELL_BERSERK              45078

struct mob_blood_elf_council_voice_triggerAI : public ScriptedAI
{
    mob_blood_elf_council_voice_triggerAI(Creature* c) : ScriptedAI(c)
    {
        for(uint8 i = 0; i < 4; ++i)
            Council[i] = 0;
    }

    uint64 Council[4];

    uint32 EnrageTimer;
    uint32 AggroYellTimer;

    uint8 YellCounter;                                      // Serves as the counter for both the aggro and enrage yells

    bool EventStarted;

    void Reset()
    {
        EnrageTimer = 900000;                               // 15 minutes
        AggroYellTimer = 500;

        YellCounter = 0;

        EventStarted = false;
    }

    // finds and stores the GUIDs for each Council member using instance data system.
    void LoadCouncilGUIDs()
    {
        if(ScriptedInstance* pInstance = ((ScriptedInstance*)m_creature->GetInstanceData()))
        {
            Council[0] = pInstance->GetData64(DATA_GATHIOSTHESHATTERER);
            Council[1] = pInstance->GetData64(DATA_VERASDARKSHADOW);
            Council[2] = pInstance->GetData64(DATA_LADYMALANDE);
            Council[3] = pInstance->GetData64(DATA_HIGHNETHERMANCERZEREVOR);
        }else error_log(ERROR_INST_DATA);
    }

    void Aggro(Unit* who) {}

    void AttackStart(Unit* who) {}
    void MoveInLineOfSight(Unit* who) {}

    void UpdateAI(const uint32 diff)
    {
        if(!EventStarted)
            return;

        if(YellCounter > 3)
            return;

        if(AggroYellTimer)
        {
            if(AggroYellTimer <= diff)
        {
            if(Unit* pMember = Unit::GetUnit(*m_creature, Council[YellCounter]))
            {
                DoScriptText(CouncilAggro[YellCounter].entry, pMember);
                AggroYellTimer = CouncilAggro[YellCounter].timer;
            }
            ++YellCounter;
            if(YellCounter > 3)
                YellCounter = 0;                            // Reuse for Enrage Yells
        }else AggroYellTimer -= diff;
        }

        if(EnrageTimer)
        {
            if(EnrageTimer <= diff)
        {
            if(Unit* pMember = Unit::GetUnit(*m_creature, Council[YellCounter]))
            {
                pMember->CastSpell(pMember, SPELL_BERSERK, true);
                DoScriptText(CouncilEnrage[YellCounter].entry, pMember);
                EnrageTimer = CouncilEnrage[YellCounter].timer;
            }
            ++YellCounter;
        }else EnrageTimer -= diff;
        }
    }
};

struct mob_illidari_councilAI : public ScriptedAI
{
    mob_illidari_councilAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        for(uint8 i = 0; i < 4; ++i)
            Council[i] = 0;
    }

    ScriptedInstance* pInstance;

    uint64 Council[4];

    uint32 CheckTimer;
    uint32 EndEventTimer;

    uint8 DeathCount;

    bool EventBegun;

    void Reset()
    {
        CheckTimer    = 2000;
        EndEventTimer = 0;

        DeathCount = 0;

        Creature* pMember = NULL;
        for(uint8 i = 0; i < 4; ++i)
        {
            if(pMember = (Unit::GetCreature((*m_creature), Council[i])))
            {
                if(!pMember->isAlive())
                {
                    pMember->RemoveCorpse();
                    pMember->Respawn();
                }
                pMember->AI()->EnterEvadeMode();
            }
        }

        if(pInstance && pInstance->GetData(DATA_ILLIDARICOUNCILEVENT) != DONE)
        {
            pInstance->SetData(DATA_ILLIDARICOUNCILEVENT, NOT_STARTED);
            if(Creature* VoiceTrigger = (Unit::GetCreature(*m_creature, pInstance->GetData64(DATA_BLOOD_ELF_COUNCIL_VOICE))))
                VoiceTrigger->AI()->EnterEvadeMode();
        }

        EventBegun = false;

        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetDisplayId(11686);
    }

    void Aggro(Unit *who) {}
    void AttackStart(Unit* who) {}
    void MoveInLineOfSight(Unit* who) {}

    void StartEvent(Unit *target)
    {
        if(!pInstance) return;

        if(target && target->isAlive())
        {
            Council[0] = pInstance->GetData64(DATA_GATHIOSTHESHATTERER);
            Council[1] = pInstance->GetData64(DATA_HIGHNETHERMANCERZEREVOR);
            Council[2] = pInstance->GetData64(DATA_LADYMALANDE);
            Council[3] = pInstance->GetData64(DATA_VERASDARKSHADOW);

            // Start the event for the Voice Trigger
            if(Creature* VoiceTrigger = (Unit::GetCreature(*m_creature, pInstance->GetData64(DATA_BLOOD_ELF_COUNCIL_VOICE))))
            {
                ((mob_blood_elf_council_voice_triggerAI*)VoiceTrigger->AI())->LoadCouncilGUIDs();
                ((mob_blood_elf_council_voice_triggerAI*)VoiceTrigger->AI())->EventStarted = true;
            }

            for(uint8 i = 0; i < 4; ++i)
            {
                Unit* Member = NULL;
                if(Council[i])
                {
                    Member = Unit::GetUnit((*m_creature), Council[i]);
                    if(Member && Member->isAlive())
                        (Member->ToCreature())->AI()->AttackStart(target);
                }
            }

            pInstance->SetData(DATA_ILLIDARICOUNCILEVENT, IN_PROGRESS);

            EventBegun = true;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!EventBegun) return;

        if(EndEventTimer)
        {
            if(EndEventTimer <= diff)
            {
                if(DeathCount > 3)
                {
                    if(pInstance)
                    {
                        if(Creature* VoiceTrigger = (Unit::GetCreature(*m_creature, pInstance->GetData64(DATA_BLOOD_ELF_COUNCIL_VOICE))))
                            VoiceTrigger->DealDamage(VoiceTrigger, VoiceTrigger->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        pInstance->SetData(DATA_ILLIDARICOUNCILEVENT, DONE);
                        //m_creature->SummonCreature(AKAMAID,746.466980f,304.394989f,311.90208f,6.272870f,TEMPSUMMON_DEAD_DESPAWN,0);
                    }
                    m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    return;
                }

                Creature* pMember = (Unit::GetCreature(*m_creature, Council[DeathCount]));
                if(pMember && pMember->isAlive())
                    pMember->DealDamage(pMember, pMember->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                ++DeathCount;
                EndEventTimer = 1500;
            }else EndEventTimer -= diff;
        }

        if(CheckTimer)
        {
            if(CheckTimer <= diff)
            {
                uint8 EvadeCheck = 0;
                for(uint8 i = 0; i < 4; ++i)
                {
                    if(Council[i])
                    {
                        if(Creature* Member = (Unit::GetCreature((*m_creature), Council[i])))
                        {
                            // This is the evade/death check.
                            if(Member->isAlive() && !Member->getVictim())
                                ++EvadeCheck;                   //If all members evade, we reset so that players can properly reset the event
                            else if(!Member->isAlive())         // If even one member dies, kill the rest, set instance data, and kill self.
                            {
                                EndEventTimer = 1000;
                                CheckTimer = 0;
                                return;
                            }
                        }
                    }
                }

                if(EvadeCheck > 3)
                    Reset();

                CheckTimer = 2000;
            }else CheckTimer -= diff;
        }

    }
};

struct boss_illidari_councilAI : public ScriptedAI
{
    boss_illidari_councilAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        for(uint8 i = 0; i < 4; ++i)
            Council[i] = 0;
        LoadedGUIDs = false;
    }

    uint64 Council[4];

    ScriptedInstance* pInstance;

    bool LoadedGUIDs;

    void Aggro(Unit* who)
    {
        if(pInstance)
        {
            Creature* Controller = (Unit::GetCreature(*m_creature, pInstance->GetData64(DATA_ILLIDARICOUNCIL)));
            if(Controller)
                ((mob_illidari_councilAI*)Controller->AI())->StartEvent(who);
        }
        else
        {
            error_log(ERROR_INST_DATA);
            EnterEvadeMode();
            return;
        }
        DoZoneInCombat();
        // Load GUIDs on first aggro because the creature guids are only set as the creatures are created in world-
        // this means that for each creature, it will attempt to LoadGUIDs even though some of the other creatures are
        // not in world, and thus have no GUID set in the instance data system. Putting it in aggro ensures that all the creatures
        // have been loaded and have their GUIDs set in the instance data system.
        if(!LoadedGUIDs)
            LoadGUIDs();
    }
    
    bool TryDoCast(Unit *victim, uint32 spellId, bool triggered = false)
    {
        if(m_creature->IsNonMeleeSpellCasted(false)) return false;

        DoCast(victim,spellId,triggered);
        return true;
    }

    void EnterEvadeMode()
    {
        for(uint8 i = 0; i < 4; ++i)
        {
            if(Unit* pUnit = Unit::GetUnit(*m_creature, Council[i]))
                if(pUnit != m_creature && pUnit->getVictim())
                {
                    AttackStart(pUnit->getVictim());
                    return;
                }
        }
        ScriptedAI::EnterEvadeMode();
    }

    void DamageTaken(Unit* done_by, uint32 &damage)
    {
        if(done_by == m_creature)
            return;

        damage /= GetAliveCount();
        for(uint8 i = 0; i < 4; ++i)
        {
            if(Creature* pUnit = Unit::GetCreature(*m_creature, Council[i]))
                if(pUnit != m_creature && pUnit->isAlive())
                {
                    if(damage <= pUnit->GetHealth())
                    {
                        pUnit->LowerPlayerDamageReq(damage);
                        pUnit->SetHealth(pUnit->GetHealth() - damage);
                    }
                    else
                    {
                        pUnit->LowerPlayerDamageReq(damage);
                        pUnit->Kill(pUnit, false);
                    }
                }
        }
    }
    
    uint8 GetAliveCount() // Returns number of alive council members, to share damage
    {
        uint8 count = 0;
        for(uint8 i = 0; i < 4; ++i)
        {
            if (Creature* pUnit = Unit::GetCreature(*m_creature, Council[i]))
            {
                if (pUnit->isAlive())
                    count++;
            }
        }
        
        if (!count)
            count = 4;
        
        return count;
    }

    void LoadGUIDs()
    {
        if(!pInstance)
        {
            error_log(ERROR_INST_DATA);
            return;
        }

        Council[0] = pInstance->GetData64(DATA_LADYMALANDE);
        Council[1] = pInstance->GetData64(DATA_HIGHNETHERMANCERZEREVOR);
        Council[2] = pInstance->GetData64(DATA_GATHIOSTHESHATTERER);
        Council[3] = pInstance->GetData64(DATA_VERASDARKSHADOW);

        LoadedGUIDs = true;
    }
};

struct boss_gathios_the_shattererAI : public boss_illidari_councilAI
{
    boss_gathios_the_shattererAI(Creature *c) : boss_illidari_councilAI(c) {}

    uint32 ConsecrationTimer;
    uint32 HammerOfJusticeTimer;
    uint32 SealTimer;
    uint32 AuraTimer;
    uint32 BlessingTimer;
    uint32 JudgeTimer;

    void Reset()
    {
        ConsecrationTimer = 40000;
        HammerOfJusticeTimer = 10000;
        SealTimer = 40000;
        AuraTimer = 90000;
        BlessingTimer = 60000;
        JudgeTimer = 45000;
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_GATH_SLAY, m_creature);
    }

    void JustDied(Unit *victim)
    {
        DoScriptText(SAY_GATH_DEATH, m_creature);
    }

    Unit* SelectCouncilMember()
    {
        Unit* pUnit = m_creature;
        uint32 member = 0;                                  // He chooses Lady Malande most often

        if(rand()%10 == 0)                                  // But there is a chance he picks someone else.
            member = urand(1, 3);

        if(member != 2)                                     // No need to create another pointer to us using Unit::GetUnit
            pUnit = Unit::GetUnit((*m_creature), Council[member]);
        return pUnit;
    }

    void CastAuraOnCouncil()
    {
        uint32 spellid = 0;
        switch(rand()%2)
        {
            case 0: spellid = SPELL_DEVOTION_AURA;   break;
            case 1: spellid = SPELL_CHROMATIC_AURA;  break;
        }
        for(uint8 i = 0; i < 4; ++i)
        {
            Unit* pUnit = Unit::GetUnit((*m_creature), Council[i]);
            if(pUnit)
                pUnit->CastSpell(pUnit, spellid, true, 0, 0, m_creature->GetGUID());
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(BlessingTimer < diff)
        {
            if(!m_creature->IsNonMeleeSpellCasted(false))
            {
                if(Unit* pUnit = SelectCouncilMember())
                {
                    switch(rand()%2)
                    {
                        case 0: DoCast(pUnit, SPELL_BLESS_SPELLWARD);  break;
                        case 1: DoCast(pUnit, SPELL_BLESS_PROTECTION); break;
                    }
                }
                BlessingTimer = 60000;
            }
        }else BlessingTimer -= diff;

        if(JudgeTimer < diff)
        {
           if(!m_creature->IsNonMeleeSpellCasted(false))
           {
                if(m_creature->HasAura(SPELL_SEAL_OF_COMMAND,0))
                {
                    if(TryDoCast(m_creature->getVictim(),SPELL_JUDGEMENT_OF_COMMAND))
                    {
                        m_creature->RemoveAurasDueToSpell(SPELL_SEAL_OF_COMMAND);
                        JudgeTimer = 45000;
                    }
                }
                if(m_creature->HasAura(SPELL_SEAL_OF_BLOOD,0))
                {
                    if(TryDoCast(m_creature->getVictim(),SPELL_JUDGEMENT_OF_BLOOD))
                    {
                        m_creature->RemoveAurasDueToSpell(SPELL_SEAL_OF_BLOOD);
                        JudgeTimer = 45000;
                    }
                }
                JudgeTimer = 15000;
           }
        }else JudgeTimer -= diff;

        if(ConsecrationTimer < diff)
        {
            if(TryDoCast(m_creature, SPELL_CONSECRATION))
                ConsecrationTimer = 40000;
        }else ConsecrationTimer -= diff;

        if(HammerOfJusticeTimer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 40, true))
            {
                // is in ~10-40 yd range
                if(m_creature->GetDistance2d(target) > 10)
                {
                    if(TryDoCast(target, SPELL_HAMMER_OF_JUSTICE))
                        HammerOfJusticeTimer = 20000;
                }
            }
        }else HammerOfJusticeTimer -= diff;

        if(SealTimer < diff)
        {
            if(!m_creature->IsNonMeleeSpellCasted(false))
            {
                switch(rand()%2)
                {
                    case 0: DoCast(m_creature, SPELL_SEAL_OF_COMMAND);  break;
                    case 1: DoCast(m_creature, SPELL_SEAL_OF_BLOOD);    break;
                }
                SealTimer = 30000;
            }
        }else SealTimer -= diff;

        if(AuraTimer < diff)
        {
            if(!m_creature->IsNonMeleeSpellCasted(false))
            {
                CastAuraOnCouncil();
                AuraTimer = 60000;
            }
        }else AuraTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

struct boss_high_nethermancer_zerevorAI : public boss_illidari_councilAI
{
    boss_high_nethermancer_zerevorAI(Creature *c) : boss_illidari_councilAI(c) {}

    uint32 BlizzardTimer;
    uint32 FlamestrikeTimer;
    uint32 ArcaneBoltTimer;
    uint32 DampenMagicTimer;
    //uint32 Cooldown;
    uint32 ArcaneExplosionTimer;

    void Reset()
    {
        BlizzardTimer = 30000 + rand()%20 * 1000;
        FlamestrikeTimer = 30000 + rand()%20 * 1000;
        ArcaneBoltTimer = 500;
        DampenMagicTimer = 200;
        ArcaneExplosionTimer = 14000;
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_ZERE_SLAY, m_creature);
    }

    void JustDied(Unit *victim)
    {
        DoScriptText(SAY_ZERE_DEATH, m_creature);
    }
    
    void AttackStart(Unit* who)
    {
        if (m_creature->Attack(who, true))
        {
            m_creature->AddThreat(who, 0.0f);

            if (!InCombat)
            {
                InCombat = true;
                Aggro(who);
            }

            DoStartMovement(who, 13, 0);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(DampenMagicTimer < diff)
        {
                m_creature->InterruptNonMeleeSpells(false);
                DoCast(m_creature, SPELL_DAMPEN_MAGIC, true);
                DampenMagicTimer = 67200;                       // 1.12 minute
                ArcaneBoltTimer += 1000;                        // Give the Mage some time to spellsteal Dampen.
        }else DampenMagicTimer -= diff;

        if(ArcaneExplosionTimer < diff)
        {
            bool InMeleeRange = false;
            Unit *target = NULL;
            std::list<HostilReference*>& m_threatlist = m_creature->getThreatManager().getThreatList();
            for (std::list<HostilReference*>::iterator i = m_threatlist.begin(); i!= m_threatlist.end();++i)
            {
                Unit* pUnit = Unit::GetUnit((*m_creature), (*i)->getUnitGuid());
                                                            //if in melee range
                if(pUnit && pUnit->IsWithinDistInMap(m_creature, 5))
                {
                    InMeleeRange = true;
                    target = pUnit;
                    break;
                }
            }
            
            if(InMeleeRange)
            {
                DoCast(target, SPELL_ARCANE_EXPLOSION);
                ArcaneExplosionTimer = 10000;
            }
            else        // If no player around, do not cast, and ofc do not reset timer, set it to 1 sec instead
            {
                ArcaneExplosionTimer = 1000;
            }
        }else ArcaneExplosionTimer -= diff;

        if(ArcaneBoltTimer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_ARCANE_BOLT);
            ArcaneBoltTimer = 3000;
        }else ArcaneBoltTimer -= diff;

        if(BlizzardTimer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
            {
                DoCast(target, SPELL_BLIZZARD);
                BlizzardTimer = 45000 + rand()%46 * 1000;
                FlamestrikeTimer += 10000;
            }
        }else BlizzardTimer -= diff;

        if(FlamestrikeTimer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
            {
                DoCast(target, SPELL_FLAMESTRIKE);
                FlamestrikeTimer = 55000 + rand()%46 * 1000;
                BlizzardTimer += 10000;
            }
        }else FlamestrikeTimer -= diff;
    }
};

struct boss_lady_malandeAI : public boss_illidari_councilAI
{
    boss_lady_malandeAI(Creature *c) : boss_illidari_councilAI(c) {}

    uint32 EmpoweredSmiteTimer;
    uint32 CircleOfHealingTimer;
    uint32 DivineWrathTimer;
    uint32 ReflectiveShieldTimer;

    void Reset()
    {
        EmpoweredSmiteTimer = 18000;
        CircleOfHealingTimer = 20000;
        DivineWrathTimer = 40000;
        ReflectiveShieldTimer = 15000;
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_MALA_SLAY, m_creature);
    }

    void JustDied(Unit *victim)
    {
        DoScriptText(SAY_MALA_DEATH, m_creature);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(EmpoweredSmiteTimer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
            {
                DoCast(target, SPELL_EMPOWERED_SMITE);
                EmpoweredSmiteTimer = 38000;
            }
        }else EmpoweredSmiteTimer -= diff;

        if(CircleOfHealingTimer < diff)
        {
            DoCast(m_creature, SPELL_CIRCLE_OF_HEALING);
            CircleOfHealingTimer = 60000;
        }else CircleOfHealingTimer -= diff;

        if(DivineWrathTimer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
            {
                DoCast(target, SPELL_DIVINE_WRATH);
                DivineWrathTimer = 40000 + rand()%41 * 1000;
            }
        }else DivineWrathTimer -= diff;

        if(ReflectiveShieldTimer < diff)
        {
            DoCast(m_creature, SPELL_REFLECTIVE_SHIELD);
            ReflectiveShieldTimer = 65000;
        }else ReflectiveShieldTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

struct boss_veras_darkshadowAI : public boss_illidari_councilAI
{
    boss_veras_darkshadowAI(Creature *c) : boss_illidari_councilAI(c) {}

    uint64 EnvenomTargetGUID;

    uint32 DeadlyPoisonTimer;
    uint32 VanishTimer;
    uint32 AppearEnvenomTimer;
    uint32 EnvenomTimer;

    bool HasVanished;

    void Reset()
    {
        EnvenomTargetGUID = 0;

        DeadlyPoisonTimer = 20000;
        VanishTimer = 10000;
        EnvenomTimer = 3000;

        HasVanished = false;
        m_creature->SetVisibility(VISIBILITY_ON);
        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }
    
    void SpellHitTarget(Unit *target, const SpellEntry *spell)
    {
        if(spell->Id != 41485)
        {
            if(target->GetTypeId() == TYPEID_PLAYER)
            {
                EnvenomTargetGUID = target->GetGUID();
                EnvenomTimer = 3000;
            }
        }
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_VERA_SLAY, m_creature);
    }

    void JustDied(Unit *victim)
    {
        m_creature->SetVisibility(VISIBILITY_ON);
        DoScriptText(SAY_VERA_DEATH, m_creature);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if(!HasVanished)
        {
            if(DeadlyPoisonTimer < diff)
            {
				if (m_creature->getVictim()->GetTypeId() == TYPEID_PLAYER)
					DoCast(m_creature->getVictim(), SPELL_DEADLY_POISON);
                DeadlyPoisonTimer = 15000 + rand()%31 * 1000;
            }else DeadlyPoisonTimer -= diff;

            if(AppearEnvenomTimer < diff)                   // Cast Envenom. This is cast 4 seconds after Vanish is over
            {
                DoCast(m_creature->getVictim(), SPELL_ENVENOM);
                AppearEnvenomTimer = 90000;
            }else AppearEnvenomTimer -= diff;

            if(VanishTimer < diff)                          // Disappear and stop attacking, but follow a random unit
            {
                if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                {
                    VanishTimer = 30000;
                    AppearEnvenomTimer= 28000;
                    HasVanished = true;
                    m_creature->SetVisibility(VISIBILITY_OFF);
                    m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    DoResetThreat();
                                                            // Chase a unit. Check before DoMeleeAttackIfReady prevents from attacking
                    m_creature->AddThreat(target, 500000.0f);
                    m_creature->GetMotionMaster()->MoveChase(target);
                }
            }else VanishTimer -= diff;

            DoMeleeAttackIfReady();
        }
        else
        {
            if(VanishTimer < diff)                          // Become attackable and poison current target
            {
                Unit* target = m_creature->getVictim();
                if (target->GetTypeId() == TYPEID_PLAYER)
					DoCast(target, SPELL_DEADLY_POISON);
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                DoResetThreat();
                m_creature->AddThreat(target, 3000.0f);     // Make Veras attack his target for a while, he will cast Envenom 4 seconds after.
                DeadlyPoisonTimer += 6000;
                VanishTimer = 90000;
                AppearEnvenomTimer = 4000;
                HasVanished = false;
            }else VanishTimer -= diff;

            if(AppearEnvenomTimer < diff)                   // Appear 2 seconds before becoming attackable (Shifting out of vanish)
            {
                m_creature->GetMotionMaster()->Clear();
                m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());
                m_creature->SetVisibility(VISIBILITY_ON);
                AppearEnvenomTimer = 6000;
            }else AppearEnvenomTimer -= diff;
            
            if(EnvenomTimer < diff)
            {
                if(EnvenomTargetGUID)
                {
                    if(Unit* target = Unit::GetUnit((*m_creature),EnvenomTargetGUID))
                    {
                        if(target->HasAura(SPELL_DEADLY_POISON,0))
                        {
                            if(rand()%3 == 0)
                                DoCast(target,SPELL_ENVENOM);
                        }
                    }
                }
            }else EnvenomTimer -= diff;
        }
    }
};

CreatureAI* GetAI_mob_blood_elf_council_voice_trigger(Creature* c)
{
    return new mob_blood_elf_council_voice_triggerAI(c);
}

CreatureAI* GetAI_mob_illidari_council(Creature *_Creature)
{
    return new mob_illidari_councilAI (_Creature);
}

CreatureAI* GetAI_boss_gathios_the_shatterer(Creature *_Creature)
{
    return new boss_gathios_the_shattererAI (_Creature);
}

CreatureAI* GetAI_boss_lady_malande(Creature *_Creature)
{
    return new boss_lady_malandeAI (_Creature);
}

CreatureAI* GetAI_boss_veras_darkshadow(Creature *_Creature)
{
    return new boss_veras_darkshadowAI (_Creature);
}

CreatureAI* GetAI_boss_high_nethermancer_zerevor(Creature *_Creature)
{
    return new boss_high_nethermancer_zerevorAI (_Creature);
}

void AddSC_boss_illidari_council()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="mob_illidari_council";
    newscript->GetAI = &GetAI_mob_illidari_council;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_blood_elf_council_voice_trigger";
    newscript->GetAI = &GetAI_mob_blood_elf_council_voice_trigger;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_gathios_the_shatterer";
    newscript->GetAI = &GetAI_boss_gathios_the_shatterer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_lady_malande";
    newscript->GetAI = &GetAI_boss_lady_malande;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_veras_darkshadow";
    newscript->GetAI = &GetAI_boss_veras_darkshadow;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_high_nethermancer_zerevor";
    newscript->GetAI = &GetAI_boss_high_nethermancer_zerevor;
    newscript->RegisterSelf();
}

