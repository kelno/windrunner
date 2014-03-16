#include "precompiled.h"
#include "def_sunwell_plateau.h"

enum Quotes
{
    //Alytesh
    YELL_CANFLAGRATION          =   -1580044,
    YELL_SISTER_SACROLASH_DEAD  =   -1580045,
    YELL_ALY_KILL_1             =   -1580046,
    YELL_ALY_KILL_2             =   -1580047,
    YELL_ALY_DEAD               =   -1580048,
    YELL_ENRAGE_ALY             =   -1580049,

    //Sacrolash
    YELL_SHADOW_NOVA            =   -1580050,
    YELL_SISTER_ALYTHESS_DEAD   =   -1580051,
    YELL_SAC_KILL_1             =   -1580052,
    YELL_SAC_KILL_2             =   -1580053,
    SAY_SAC_DEAD                =   -1580054,
    YELL_ENRAGE_SACRO           =   -1580055,

    //Intro
    YELL_INTRO_SAC_1            =   -1580056,
    YELL_INTRO_ALY_2            =   -1580057,
    YELL_INTRO_SAC_3            =   -1580058,
    YELL_INTRO_ALY_4            =   -1580059,
    YELL_INTRO_SAC_5            =   -1580060,
    YELL_INTRO_ALY_6            =   -1580061,
    YELL_INTRO_SAC_7            =   -1580062,
    YELL_INTRO_ALY_8            =   -1580063,

    //Emote
    EMOTE_SHADOW_NOVA           =   -1580064,
    EMOTE_CONFLAGRATION         =   -1580065
};

enum Spells
{
    //Lady Sacrolash spells
    SPELL_DARK_TOUCHED      =   45347,
    SPELL_SHADOW_BLADES     =   45248, //10 secs
    SPELL_DARK_STRIKE       =   45271,
    SPELL_SHADOW_NOVA       =   45329, //30-35 secs
    SPELL_CONFOUNDING_BLOW  =   45256, //25 secs
    SPELL_DUAL_WIELD        =   42459,

    //Shadow Image spells
    SPELL_SHADOW_FURY       =   45270,
    SPELL_IMAGE_VISUAL      =   45263,

    //Misc spells
    SPELL_ENRAGE            =   46587,
    SPELL_EMPOWER           =   45366,
    SPELL_DARK_FLAME        =   45345,
    SPELL_FIREBLAST         =   45232, // if a player escapes from the room

    //Grand Warlock Alythess spells
    SPELL_PYROGENICS        =   45230, //15secs
    SPELL_FLAME_TOUCHED     =   45348,
    SPELL_CONFLAGRATION     =   45342, //30-35 secs
    SPELL_BLAZE             =   45235, //on main target every 3 secs
    SPELL_FLAME_SEAR        =   46771,
    SPELL_BLAZE_SUMMON      =   45236, //187366 GO
    SPELL_BLAZE_BURN        =   45246
};

struct EredarTwin : public ScriptedAI
{
    EredarTwin(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        RespawnTimer = 0;
    }
    
    ScriptedInstance* pInstance;
    bool SisterDead;
    bool Enraged;
    bool isSacrolash; //else is Alythess
    uint32 CheckFBTimer;
    uint32 EnrageTimer;
    uint32 ConflagrationTimer;
    uint32 ConflagOrShadowNovaTimer;
    uint32 RespawnTimer;

    void HandleTouchedSpells(Unit* target, uint32 TouchedType)
    {
        if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsPet())
            return;

        switch(TouchedType)
        {
        case SPELL_FLAME_TOUCHED:
            if(!target->HasAura(SPELL_DARK_FLAME, 0))
            {
                if(target->HasAura(SPELL_DARK_TOUCHED, 0))
                {
                    target->RemoveAurasDueToSpell(SPELL_DARK_TOUCHED);
                    target->CastSpell(target, SPELL_DARK_FLAME, true);
                }else target->CastSpell(target, SPELL_FLAME_TOUCHED, true);
            }
            break;
        case SPELL_DARK_TOUCHED:
            if(!target->HasAura(SPELL_DARK_FLAME, 0))
            {
                if(target->HasAura(SPELL_FLAME_TOUCHED, 0))
                {
                    target->RemoveAurasDueToSpell(SPELL_FLAME_TOUCHED);
                    target->CastSpell(target, SPELL_DARK_FLAME, true);
                }else target->CastSpell(target, SPELL_DARK_TOUCHED, true);
            }
            break;
        }
    }

    void SpellHitTarget(Unit* target,const SpellEntry* spell)
    {
        switch(spell->Id)
        {
        case SPELL_SHADOW_BLADES:
        case SPELL_SHADOW_NOVA:
        case SPELL_CONFOUNDING_BLOW:
        case SPELL_SHADOW_FURY:
            HandleTouchedSpells(target, SPELL_DARK_TOUCHED);
            break;
        case SPELL_CONFLAGRATION:
        case SPELL_FLAME_SEAR:
            HandleTouchedSpells(target, SPELL_FLAME_TOUCHED);
            break;
        case SPELL_BLAZE:
            me->CastSpell(target, SPELL_BLAZE_SUMMON, true);
            break;
        }
    }

    void EnterCombat(Unit *who)
    {
        DoZoneInCombat();

        if(pInstance)
        {
            Unit* Sister = Unit::GetUnit((*m_creature),pInstance->GetData64(isSacrolash ? DATA_ALYTHESS : DATA_SACROLASH));
            if (Sister && Sister->IsAlive() && !(Sister->GetVictim()))
                (Sister->ToCreature())->AI()->AttackStart(who);

            pInstance->SetData(DATA_EREDAR_TWINS_EVENT, IN_PROGRESS);
        }
    }

    void EnterEvadeMode()
    {
        me->SetVisibility(VISIBILITY_OFF);
        RespawnTimer = 30000;
        me->SetReactState(REACT_PASSIVE);
        ScriptedAI::EnterEvadeMode();

        if (pInstance && pInstance->GetData(DATA_EREDAR_TWINS_EVENT) != DONE)
            pInstance->SetData(DATA_EREDAR_TWINS_EVENT, NOT_STARTED);
    }

    //return true if any players in threatlist outside the room
    bool CheckPlayersForFireblast()
    {
        std::list<HostilReference*>& threatlist = me->getThreatManager().getThreatList();
        for (auto itr : threatlist)
            if (itr->getTarget()->GetDistance(1816.406250, 625.544495, 33.403782) >= 50.0f)
                return true;

        return false;
    }

    void JustDied(Unit* Killer)
    {
        if (SisterDead)
        {
            DoScriptText(isSacrolash ? SAY_SAC_DEAD : YELL_ALY_DEAD, m_creature);

            if(pInstance) {
                pInstance->SetData(DATA_EREDAR_TWINS_EVENT, DONE);
                pInstance->RemoveAuraOnAllPlayers(SPELL_DARK_TOUCHED);
                pInstance->RemoveAuraOnAllPlayers(SPELL_FLAME_TOUCHED);
            }
        }
        else
            m_creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
    }

    virtual void Reset()
    {
        Enraged = false;

        if (!m_creature->IsInCombat()) {
            ConflagOrShadowNovaTimer = 30000; //this should be erased in subclasses
            EnrageTimer = 360000;
            CheckFBTimer = 200;

            SisterDead = false;
        }

        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_HASTE_SPELLS, true);

        if (pInstance) {
            if (pInstance->GetData(DATA_EREDAR_TWINS_EVENT) != DONE) 
            {
                Unit* Sister = Unit::GetUnit((*m_creature),pInstance->GetData64(isSacrolash ? DATA_ALYTHESS : DATA_SACROLASH));
                if (Sister)
                    if (Sister->isDead())
                        (Sister->ToCreature())->Respawn();
                    else {
                        if(Sister->GetVictim())
                            m_creature->getThreatManager().addThreat(Sister->GetVictim(),0.0f);
                    }
            }

            pInstance->RemoveAuraOnAllPlayers(SPELL_DARK_TOUCHED);
            pInstance->RemoveAuraOnAllPlayers(SPELL_FLAME_TOUCHED);

            if (pInstance->GetData(DATA_EREDAR_TWINS_EVENT) != DONE)
                pInstance->SetData(DATA_EREDAR_TWINS_EVENT, NOT_STARTED);
            else
            {
                if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                     muru->SetReactState(REACT_AGGRESSIVE);
            }
        }
    }

    virtual void UpdateAI(const uint32 diff)
    {
        if (RespawnTimer) 
        {
            if (RespawnTimer <= diff) {
                me->SetVisibility(VISIBILITY_ON);
                me->SetReactState(REACT_AGGRESSIVE);
                RespawnTimer = 0;
            }
            else
                RespawnTimer -= diff;
        }

        if(!SisterDead)
        {
            if (pInstance)
            {
                Unit* Sister = Unit::GetUnit((*m_creature),pInstance->GetData64(isSacrolash ? DATA_ALYTHESS : DATA_SACROLASH));
                if (Sister && Sister->isDead())
                {
                    DoScriptText(isSacrolash ? YELL_SISTER_ALYTHESS_DEAD : YELL_SISTER_SACROLASH_DEAD, m_creature);
                    m_creature->InterruptSpell(CURRENT_GENERIC_SPELL);
                    DoCast(m_creature, SPELL_EMPOWER, true);
                    SisterDead = true;
                }
            }
        }

        if(!me->GetVictim()) //let sub classes update victim themselves
            return;

        //Conflagration is cast by Alythess OR Sacrolash with her sister dead
        //Shadow Nova is cast by Sacrolash OR by Alythess with her sister dead
        if(ConflagOrShadowNovaTimer < diff)
        {
            bool doConflag = !isSacrolash ^ SisterDead; //false = nova
            Unit* target = NULL;

            //pick a target within the top5 top aggro of our sister, tank excepted
            if(!SisterDead)
                if(Unit* Sister = Unit::GetUnit((*m_creature),pInstance->GetData64(isSacrolash ? DATA_ALYTHESS : DATA_SACROLASH)))
                    target = ((ScriptedAI*)Sister->ToCreature()->AI())->SelectUnit(SELECT_TARGET_TOPAGGRO, 5, 100.0, true, true);

            //if no target found pick it ourselves
            if(!target)
                target = SelectUnit(SELECT_TARGET_TOPAGGRO, 5, 100.0, true, true);

            if(!target)
                target = me->GetVictim();

            //cast it then do emotes/yells
            if(target && DoCast(target, doConflag ? SPELL_CONFLAGRATION : SPELL_SHADOW_NOVA) == SPELL_CAST_OK)
            {
                if(doConflag)
                {
                    DoScriptText(EMOTE_CONFLAGRATION, m_creature, target);
                    if(!isSacrolash) //only Alythess yell for Conflag
                        DoScriptText(YELL_CANFLAGRATION, m_creature);
                } else {
                    DoScriptText(EMOTE_SHADOW_NOVA, m_creature, target);
                    if(isSacrolash) //only Sacrolash yell for Shadow Nova
                        DoScriptText(YELL_SHADOW_NOVA, m_creature);
                }
                ConflagOrShadowNovaTimer = 30000;
            }
        } else ConflagOrShadowNovaTimer -= diff;

        if (CheckFBTimer <= diff) {
            if (CheckPlayersForFireblast())
                DoCast(me, SPELL_FIREBLAST);
                
            CheckFBTimer = 400;
        } else CheckFBTimer -= diff;

        if (EnrageTimer < diff && !Enraged)
        {
            DoScriptText(isSacrolash ? YELL_ENRAGE_SACRO : YELL_ENRAGE_ALY, m_creature);
            DoCast(m_creature, SPELL_ENRAGE, true);
            Enraged = true;
        }else EnrageTimer -= diff;
    }
};

struct boss_sacrolashAI : public EredarTwin
{
    boss_sacrolashAI(Creature *c) : EredarTwin(c), summons(c)
    {
        isSacrolash = true;
    }

    uint32 ShadowbladesTimer;
    uint32 ConfoundingblowTimer;
    uint32 ShadowimageTimer;
    
    SummonList summons;

    void Reset()
    {
        EredarTwin::Reset();

        ShadowbladesTimer = 10000;
        ConfoundingblowTimer = 25000;
        ShadowimageTimer = 5000 + rand()%5000;
        ConflagOrShadowNovaTimer = 30000;
		
        // Alythess spells
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45230, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45348, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45342, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45235, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 46771, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45236, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45246, true);
        
        summons.DespawnAll();
    }
    
    void JustSummoned(Creature* pSummon)
    {
        summons.Summon(pSummon);
    }
    
    void SummonedCreatureDespawn(Creature* pSummon)
    {
        summons.Despawn(pSummon);
    }

    void KilledUnit(Unit *victim)
    {
        if (rand()%4 == 0)
            DoScriptText(RAND(YELL_SAC_KILL_1,YELL_SAC_KILL_2), me);
    }

    void UpdateAI(const uint32 diff)
    {
        EredarTwin::UpdateAI(diff);

        if (!UpdateVictim())
            return;

        if(ConfoundingblowTimer < diff)
        {
            if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                if(DoCast(target, SPELL_CONFOUNDING_BLOW) == SPELL_CAST_OK)
                    ConfoundingblowTimer = 20000 + (rand()%2000);

        }else ConfoundingblowTimer -=diff;

        if(ShadowimageTimer < diff)
        {
            Unit* target = NULL;
            Creature* summon = NULL;
            uint32 lifeTime = 10000+rand()%2000;
            for(int i = 0; i < 3; i++)
            {
                target = SelectUnit(SELECT_TARGET_RANDOM, 1, 10.0f, 50.0f, true);
                if (!target)
                    target = SelectUnit(SELECT_TARGET_RANDOM, 0, 100.0, true);
                summon = DoSpawnCreature(MOB_SHADOW_IMAGE, 0, 0, 0, 0, TEMPSUMMON_CORPSE_DESPAWN, lifeTime);
                if(summon && target) {
                    summon->AI()->AttackStart(target);
                    summon->AddThreat(target, 50000.0f);
                }
            }
            ShadowimageTimer = lifeTime;
        }else ShadowimageTimer -=diff;

        if(ShadowbladesTimer < diff)
        {
            if(DoCast(m_creature, SPELL_SHADOW_BLADES) == SPELL_CAST_OK)
                ShadowbladesTimer = 10000;
        }else ShadowbladesTimer -=diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_sacrolash(Creature *_Creature)
{
    return new boss_sacrolashAI (_Creature);
};

struct boss_alythessAI : public EredarTwin
{
    boss_alythessAI(Creature *c) : EredarTwin(c)
    {
        isSacrolash = false;
        IntroStepCounter = 10;
    }

    uint32 IntroStepCounter;
    uint32 IntroYellTimer;

    uint32 ConflagrationTimer;
    uint32 BlazeTimer;
    uint32 PyrogenicsTimer;
    uint32 FlamesearTimer;
    uint32 EnrageTimer;
    uint32 RespawnTimer;
    uint32 CheckFBTimer;

    void Reset()
    {
        EredarTwin::Reset();

        me->AI()->SetCombatMovementAllowed(false);

        BlazeTimer = 100;
        PyrogenicsTimer = 15000;
        FlamesearTimer = 15000;
        ConflagOrShadowNovaTimer = 15000;
        IntroYellTimer = 0;
        IntroStepCounter = 0;
		
        // Sacrolash spells
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45347, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45248, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45271, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45329, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45256, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 45348, true);
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (!who || m_creature->GetVictim())
            return;

        if (m_creature->canAttack(who) && who->isInAccessiblePlaceFor(m_creature) && m_creature->IsHostileTo(who)) {

            //float attackRadius = m_creature->GetAttackDistance(who);
            if (m_creature->IsWithinDistInMap(who, 45.0f) && m_creature->GetDistanceZ(who) <= CREATURE_Z_ATTACK_RANGE && m_creature->IsWithinLOSInMap(who))
            {
                if (!m_creature->IsInCombat())
                {
                    DoStartNoMovement(who);
                    EnterCombat(who);
                }
            }
        }
        else if (IntroStepCounter == 10 && m_creature->IsWithinLOSInMap(who) && m_creature->IsWithinDistInMap(who, 30))
            IntroStepCounter = 0;
    }

    void KilledUnit(Unit *victim)
    {
        if (rand()%4 == 0)
            DoScriptText(RAND(YELL_ALY_KILL_1,YELL_ALY_KILL_2), me);
    }

    uint32 IntroStep(uint32 step)
    {
        if(!pInstance) return -1;
        Creature* Sacrolash = me->GetCreature(*me,pInstance->GetData64(DATA_SACROLASH));
        if(!Sacrolash) return -1;
        switch (step)
        {
        case 0  : return 0;
        case 1  : DoScriptText(YELL_INTRO_SAC_1, Sacrolash);  return 1000;
        case 2  : DoScriptText(YELL_INTRO_ALY_2, m_creature); return 1000;
        case 3  : DoScriptText(YELL_INTRO_SAC_3, Sacrolash);  return 2000;
        case 4  : DoScriptText(YELL_INTRO_ALY_4, m_creature); return 1000;
        case 5  : DoScriptText(YELL_INTRO_SAC_5, Sacrolash);  return 2000;
        case 6  : DoScriptText(YELL_INTRO_ALY_6, m_creature); return 1000;
        case 7  : DoScriptText(YELL_INTRO_SAC_7, Sacrolash);  return 3000;
        case 8  : DoScriptText(YELL_INTRO_ALY_8, m_creature); return 900000;
        default : return 10000;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(IntroStepCounter < 9)
        {
            if(IntroStepCounter == 0 && IntroYellTimer == 0)
                if(me->FindNearestPlayer(60.0f))
                    IntroYellTimer = 1; //start intro

            if(IntroYellTimer)
            {
                if(IntroYellTimer <= diff)
                    IntroYellTimer = IntroStep(++IntroStepCounter);
                else IntroYellTimer -= diff;
            }
        }
        
        EredarTwin::UpdateAI(diff);

        if (!UpdateVictim())
            return;

        if (FlamesearTimer < diff)
        {
            if(DoCast(m_creature, SPELL_FLAME_SEAR) == SPELL_CAST_OK)
                FlamesearTimer = 10000 + rand()%3000;
        } else FlamesearTimer -=diff;

        if (PyrogenicsTimer < diff)
        {
            DoCast(m_creature, SPELL_PYROGENICS,true);
            PyrogenicsTimer = 15000;
        } else PyrogenicsTimer -= diff;

        if (BlazeTimer < diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_BLAZE) == SPELL_CAST_OK)
                BlazeTimer = 3800;

        } else BlazeTimer -= diff;

        //Alythess still hits in melee
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_alythess(Creature *_Creature)
{
    return new boss_alythessAI (_Creature);
};

enum ShadowImageType
{
    SHADOW_IMAGE_SHADOWFURY = 0,
    SHADOW_IMAGE_DARKSTRIKE = 1
};

struct mob_shadow_imageAI : public ScriptedAI
{
    mob_shadow_imageAI(Creature *c) : ScriptedAI(c) {}

    bool castedShadowFury;
    uint32 DarkstrikeTimer;
    uint32 ChangeTargetTimer;
    ShadowImageType type;

    void Reset()
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        castedShadowFury = false;
        DarkstrikeTimer = 3000;
        ChangeTargetTimer = 0;
        type = (rand() % 2) ? SHADOW_IMAGE_SHADOWFURY : SHADOW_IMAGE_DARKSTRIKE;
    }

    void SpellHitTarget(Unit* target,const SpellEntry* spell)
    {        
        if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsPet())
            return;

        switch(spell->Id)
        {

        case SPELL_SHADOW_FURY:
        case SPELL_DARK_STRIKE:
            if(!target->HasAura(SPELL_DARK_FLAME, 0))
            {
                if(target->HasAura(SPELL_FLAME_TOUCHED, 0))
                {
                    target->RemoveAurasDueToSpell(SPELL_FLAME_TOUCHED);
                    target->CastSpell(target, SPELL_DARK_FLAME, true);
                }else target->CastSpell(target,SPELL_DARK_TOUCHED,true);
            }
            break;
        }
    }

    void OnSpellFinish(Unit *caster, uint32 spellId, Unit *target, bool ok) 
    {
        if(spellId == SPELL_SHADOW_FURY)
            m_creature->DealDamage(m_creature, m_creature->GetHealth());
    }

    void UpdateAI(const uint32 diff)
    {
        if(!m_creature->HasAura(SPELL_IMAGE_VISUAL))
            DoCast(m_creature, SPELL_IMAGE_VISUAL);

        switch(type)
        {
        case SHADOW_IMAGE_SHADOWFURY:
            if(!castedShadowFury && me->IsWithinMeleeRange(m_creature->GetVictim())) 
            {
                DoCast(m_creature, SPELL_SHADOW_FURY);
                castedShadowFury = true;
            }

            break;
        case SHADOW_IMAGE_DARKSTRIKE:
            if (ChangeTargetTimer <= diff) {
                if (Creature* sacrolash = me->FindCreatureInGrid(MOB_LADY_SACROLASH, 100.0f, true)) 
                {
                    DoResetThreat();
                    Unit* target = ((ScriptedAI*)sacrolash->AI())->SelectUnit(SELECT_TARGET_RANDOM, 0, 60.0f, true);
                    if (target) {
                        AttackStart(target);
                        me->AddThreat(target, 500000.0f);
                    }
                }
                
                ChangeTargetTimer = 1000 + rand()%9000; // 1-10 sec
            } else ChangeTargetTimer -= diff;

            //try to cast dark strike every 800ms
            if(DarkstrikeTimer <= diff)
            {
                DoCast(m_creature->GetVictim(), SPELL_DARK_STRIKE);
                DarkstrikeTimer = 800;
            } else DarkstrikeTimer -= diff;
            break;
        }
    }
};

CreatureAI* GetAI_mob_shadow_image(Creature *_Creature)
{
    return new mob_shadow_imageAI (_Creature);
};

void AddSC_boss_eredar_twins()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="boss_sacrolash";
    newscript->GetAI = &GetAI_boss_sacrolash;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_alythess";
    newscript->GetAI = &GetAI_boss_alythess;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_shadow_image";
    newscript->GetAI = &GetAI_mob_shadow_image;
    newscript->RegisterSelf();
}
