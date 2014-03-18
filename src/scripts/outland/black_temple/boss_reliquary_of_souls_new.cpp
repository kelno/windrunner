#include "precompiled.h"
#include "def_black_temple.h"
#include "CreatureScript.h"
#include "CreatureAINew.h"
#include "Spell.h"

enum ReliquaryOfSoulsData {
    // Essence of Suffering
    TALK_SUFF_SAY_FREED             = 0,
    TALK_SUFF_SAY_AGGRO             = 1,
    TALK_SUFF_SAY_SLAY              = 2,
    TALK_SUFF_SAY_RECAP             = 3,
    TALK_SUFF_SAY_AFTER             = 4,
    TALK_SUFF_EMOTE_ENRAGE          = 5,
    // Essence of Desire
    TALK_DESI_SAY_FREED             = 0,
    TALK_DESI_SAY_SLAY              = 1,
    TALK_DESI_SAY_SPEC              = 2,
    TALK_DESI_SAY_RECAP             = 3,
    TALK_DESI_SAY_AFTER             = 4,
    // Essence of Anger
    TALK_ANGER_SAY_FREED            = 0,
    TALK_ANGER_SAY_SLAY             = 1,
    TALK_ANGER_SAY_SPEC             = 2,
    TALK_ANGER_SAY_BEFORE           = 3,
    TALK_ANGER_SAY_DEATH            = 4,
    
    // Spells
    SPELL_SUMMON_SOUL               = 41537, // trigger 41538
    SPELL_SUMMON_SOUL2              = 41538, // actual summoning

    AURA_OF_SUFFERING               = 41292,
    AURA_OF_SUFFERING_ARMOR         = 42017, // linked aura, need core support
    ESSENCE_OF_SUFFERING_PASSIVE    = 41296, // periodic trigger 41294
    ESSENCE_OF_SUFFERING_PASSIVE2   = 41623,
    SPELL_FIXATE_TARGET             = 41294, // dummy, select target
    SPELL_FIXATE_TAUNT              = 41295, // force taunt
    SPELL_ENRAGE                    = 41305,
    SPELL_SOUL_DRAIN                = 41303,

    AURA_OF_DESIRE                  = 41350,
    AURA_OF_DESIRE_DAMAGE           = 41352,
    SPELL_RUNE_SHIELD               = 41431,
    SPELL_DEADEN                    = 41410,
    SPELL_SOUL_SHOCK                = 41426,

    AURA_OF_ANGER                   = 41337,
    SPELL_SELF_SEETHE               = 41364, // force cast 41520
    SPELL_ENEMY_SEETHE              = 41520,
    SPELL_SOUL_SCREAM               = 41545,
    SPELL_SPITE_TARGET              = 41376, // cast 41377 after 6 sec
    SPELL_SPITE_DAMAGE              = 41377,

    ENSLAVED_SOUL_PASSIVE           = 41535,
    SPELL_SOUL_RELEASE              = 41542,
    SPELL_SUBMERGE                  = 37550, //dropout 'head'
    SPELL_SELFSTUN                  = 53088, //2.5s stun
    
    // Misc
    CREATURE_ENSLAVED_SOUL          = 23469,
    NUMBER_ENSLAVED_SOUL            = 15,
    DATA_SOUL_DEATH                 = 0,
    DATA_SOUL_SPAWN                 = 1,
    DATA_ESSENCE_OF_ANGER_DEATH     = 2,

    CREATURE_RIFT_MARKER            = 23472,
    
    // Phases
    PHASE_NONE                      = 0,
    PHASE_SUFFERING                 = 1,
    PHASE_DESIRE                    = 2,
    PHASE_ANGER                     = 3
};

class Boss_reliquary_of_souls : public CreatureScript
{
public:
    Boss_reliquary_of_souls() : CreatureScript("boss_reliquary_of_souls") {}
    
    class Boss_reliquary_of_soulsAI : public Creature_NoMovementAINew
    {
    public:
        Boss_reliquary_of_soulsAI(Creature* creature) : Creature_NoMovementAINew(creature)
        {
            instance = ((ScriptedInstance*)creature->GetInstanceData());
            essenceGUID = 0;
            phase = PHASE_NONE;
            step = 0;
            timer = 0;
            //me->getAI()->SetCombatMovementAllowed(false); NYI in CreatureAINew
        }
        
        Creature* getRandomRift()
        {
            if(riftMarkers.size() == 0)
                return nullptr;

            std::list<uint64>::iterator itr = riftMarkers.begin();
            std::advance(itr, urand(0, riftMarkers.size() - 1));

            return me->GetMap()->GetCreatureInMap(*itr);
        }

        void findRifts()
        {
            riftMarkers.clear();

            CellPair pair(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
            Cell cell(pair);
            cell.data.Part.reserved = ALL_DISTRICT;
            cell.SetNoCreate();

            std::list<Creature*> RiftList;

            Trinity::AllCreaturesOfEntryInRange check(me, CREATURE_RIFT_MARKER, 75);
            Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(RiftList, check);
            TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

            cell.Visit(pair, visitor, *(me->GetMap()));

            for(auto itr : RiftList)
                riftMarkers.push_back(itr->GetGUID());

            if(riftMarkers.size() == 0)
                sLog.outError("Reliquary of Souls : Cannot find any rifts markers.");
        }

        void onReset(bool onSpawn)
        {
            if (instance && instance->GetData(DATA_RELIQUARYOFSOULSEVENT) != DONE)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, NOT_STARTED);

            if (essenceGUID != 0) {
                Creature* essence = Creature::GetCreature(*me, essenceGUID);
                if (essence)
                    essence->DisappearAndDie();
                    
                essenceGUID = 0;
            }

            phase = PHASE_NONE;
            
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
            me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
            
            if (instance)
                instance->RemoveAuraOnAllPlayers(SPELL_ENEMY_SEETHE);
                
            //me->SetNoCallAssistance(true);
        }
        
        void onCombatStart(Unit* victim)
        {
            if(riftMarkers.size() == 0) findRifts();
            me->AddThreat(victim, 10000.0f);
            setZoneInCombat();
            
            if (instance)
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, IN_PROGRESS);
                
            phase = PHASE_SUFFERING;
            step = 0;
            timer = 0;
        }
        
        void summonSoul()
        {
            Creature* rift = getRandomRift();

            if(rift)
                rift->CastSpell(rift,SPELL_SUMMON_SOUL2,true,0,0,me->GetGUID());
        }

        void onDeath(Unit* killer)
        {
            if (instance) {
                instance->SetData(DATA_RELIQUARYOFSOULSEVENT, DONE);
                instance->RemoveAuraOnAllPlayers(SPELL_ENEMY_SEETHE);
            }
        }
        
        void message(uint32 id, uint64 data)
        {
            switch(id)
            {
            case DATA_SOUL_DEATH:
                if(data == 1)
                    soulDeathCount++;;
                break;
            case DATA_SOUL_SPAWN:
                {
                soulCount++;
                Creature* soul = me->GetMap()->GetCreatureInMap(MAKE_NEW_GUID(data, CREATURE_ENSLAVED_SOUL, HIGHGUID_UNIT));
                if (soul)
                {
                    Unit* target = selectUnit(SELECT_TARGET_RANDOM,0,200.0f,true);
                    if(target)
                    {
                        soul->getAI()->attackStart(target);
                        soul->AddThreat(target, 1500.0f);
                    }
                }
                }
                break;
            case DATA_ESSENCE_OF_ANGER_DEATH:
                doCast(me, 7, true); //suicide
                break;
            }
        }
        
        void onMoveInLoS(Unit* who)
        {
            if (!me->IsInCombat() && who->ToPlayer() && who->GetDistance(me) <= 75.0f)
                attackStart(who);
        }
        
        void update(uint32 const diff)
        {
            if (phase == PHASE_NONE)
                return;
                
            if (me->getThreatManager().getThreatList().empty()) {
                evade();
                return;
            }
            
            Creature* essence;
            if (essenceGUID != 0) {
                essence = Creature::GetCreature(*me, essenceGUID);
                if (!essence) {
                    evade();
                    return;
                }
            }
            
            if (timer <= diff) {
                switch (step) {
                case 0:
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY2H);
                    timer = 3000;
                    break;
                case 1: //open ribs
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SUBMERGE);
                    doCast(me, SPELL_SUBMERGE);
                    timer = 2500; //2800 avant
                    break;
                case 2: //summon essence
                    if (Creature* summon = me->SummonCreature(23417 + phase, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0)) {
                        me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                        if (summon->getAI()) {
                            //summon->getAI()->attackStart(selectUnit(SELECT_TARGET_TOPAGGRO, 0));
                            Unit* target = selectUnit(SELECT_TARGET_NEAREST,0,200.0f,true);
                            summon->getAI()->attackStart(target);
                            essenceGUID = summon->GetGUID();
                            summon->SetSummoner(me);
                            me->GetMotionMaster()->MoveIdle();
                        }
                    }
                    else
                        evade();
                    timer = 5000;
                    break;
                case 3: // wait for essence to be done or die if this was last essence
                    if(phase == PHASE_ANGER)
                        return;

                    if (!essence->IsAlive()) //debugging purpose for now
                    {
                        sLog.outError("RoS : Essence is dead (phase = %u), skipping animation",phase);
                        step = 5; //goto despawn essence
                        return;
                    }
                    if (essence->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE)) {
                        //mergeThreatList(essence);
                        essence->DeleteThreatList();
                        essence->SetReactState(REACT_PASSIVE);
                        essence->GetMotionMaster()->MoveFollow(me, 0, 0);
                    }
                    else
                        return;
                    timer = 1000;
                    break;
                case 4: // wait for essence to reach me & close ribs
                    timer = 500;
                    if (essence->IsWithinDistInMap(me, 10.0f)) {
                        timer = 2000;
                        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                        essence->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_SUBMERGE); // Rotate and disappear
                    }
                    else { // Really necessary?
                        //mergeThreatList(essence);
                        essence->DeleteThreatList();
                        essence->SetReactState(REACT_PASSIVE);
                        essence->GetMotionMaster()->MoveFollow(me, 0, 0);
                        return;
                    }
                    break;
                case 5: //despawn essence
                    if (phase == PHASE_SUFFERING) {
                        if (essence->getAI())
                            essence->getAI()->talk(TALK_SUFF_SAY_AFTER);
                    }
                    else {
                        if (essence->getAI())
                            essence->getAI()->talk(TALK_DESI_SAY_AFTER);
                    }
                    
                    essence->SetVisibility(VISIBILITY_OFF);
                    essence->setDeathState(DEAD);
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                    essenceGUID = 0;
                    soulCount = 0;
                    soulDeathCount = 0;
                    timer = 3000;
                    break;
                case 6: //summon souls
                    if (soulCount < NUMBER_ENSLAVED_SOUL-2) {
                        for(uint8 i = 0; i < 3; i++)
                            summonSoul();
                            
                        timer = 3000;
                        return;
                    }
                    break;
                case 7: //wait for souls deaths to continue to next phase
                    if (soulDeathCount >= soulCount) {
                        step = 1;
                        phase++;
                        timer = 5000;
                    }
                    return;
                default:
                    break;
                }
                
                step++;
            }
            else
                timer -= diff;
        }
        
        
    private:
        ScriptedInstance* instance;
        std::list<uint64> riftMarkers;

        uint64 essenceGUID;
        
        uint32 timer;
        uint32 soulCount;
        uint32 soulDeathCount;
        
        uint8 phase;
        uint8 step;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_reliquary_of_soulsAI(creature);
    }
};

//Phase 1
class Boss_essence_of_suffering : public CreatureScript
{
public:
    Boss_essence_of_suffering() : CreatureScript("boss_essence_of_suffering") {}
    
    class Boss_essence_of_sufferingAI : public CreatureAINew
    {
    public:
        Boss_essence_of_sufferingAI(Creature* creature) : CreatureAINew(creature) {}
        
        enum events {
            EV_FIXATE       = 0,
            EV_ENRAGE       = 1,
            EV_SOUL_DRAIN   = 2
        };
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_FIXATE, 8000, 8000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_ENRAGE, 45000, 45000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SOUL_DRAIN, 20000, 20000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_FIXATE, 8000);
                scheduleEvent(EV_ENRAGE, 45000);
                scheduleEvent(EV_SOUL_DRAIN, 20000);
            }
            
            me->SetFullTauntImmunity(true);
            //me->SetNoCallAssistance(true);
        }
        
        void evade()
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->evade();
                
            CreatureAINew::evade();
        }
        
        void onDamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            if (damage >= me->GetHealth()) {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAllAuras();
                talk(TALK_SUFF_SAY_RECAP);
            }
        }
        
        void onCombatStart(Unit* victim)
        {
            talk(TALK_SUFF_SAY_FREED);
            setZoneInCombat();
            doCast(me, AURA_OF_SUFFERING, true);
            doCast(me, ESSENCE_OF_SUFFERING_PASSIVE, true);
            doCast(me, ESSENCE_OF_SUFFERING_PASSIVE2, true);
        }
        
        void onKill(Unit* victim)
        {
            talk(TALK_SUFF_SAY_SLAY);
        }
        
        void update(uint32 const diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return;

            if (!updateVictim())
                return;
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_FIXATE:
                {
                    Unit* target = selectUnit(SELECT_TARGET_NEAREST, 0, 30.0f, true);

                    if (target) {
                        target->CastSpell(me, SPELL_FIXATE_TAUNT, true);
                        doResetThreat();
                        me->AddThreat(target, 1000000.0f);
                        
                        if ((rand() % 16) == 0)
                            talk(TALK_SUFF_SAY_AGGRO);
                    }
                    
                    scheduleEvent(EV_FIXATE, 5000);
                    break;
                }
                case EV_ENRAGE:
                    doCast(me, SPELL_ENRAGE);
                    talk(TALK_SUFF_EMOTE_ENRAGE);
                    scheduleEvent(EV_ENRAGE, 30000);
                    break;
                case EV_SOUL_DRAIN:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SOUL_DRAIN);
                    scheduleEvent(EV_SOUL_DRAIN, 20000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_essence_of_sufferingAI(creature);
    }
};

//Phase 2
class Boss_essence_of_desire : public CreatureScript
{
public:
    Boss_essence_of_desire() : CreatureScript("boss_essence_of_desire") {}
    
    class Boss_essence_of_desireAI : public CreatureAINew
    {
    public:
        Boss_essence_of_desireAI(Creature* creature) : CreatureAINew(creature) {}
        
        //Debugging 
        void onDeath(Unit* killer) 
        {
            sLog.outError("essence of desire died killed by a %s (guid : %u)",killer->ToCreature() ? "creature" : "player",killer->GetGUIDLow());
        }

        enum events {
            EV_RUNE_SHIELD  = 0,
            EV_DEADEN       = 1,
            EV_SOUL_SHOCK   = 2
        };
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_RUNE_SHIELD, 12000, 12000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_DEADEN, 25000, 25000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SOUL_SHOCK, 6000, 6000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_RUNE_SHIELD, 12000);
                scheduleEvent(EV_DEADEN, 25000);
                scheduleEvent(EV_SOUL_SHOCK, 6000);
            }
            
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
           // me->SetNoCallAssistance(true);
        }
        
        void evade()
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->evade();
                
            CreatureAINew::evade();
        }
        
        void onDamageTaken(Unit* attacker, uint32& damage)
        {
            if (damage >= me->GetHealth()) {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAllAuras();
                talk(TALK_DESI_SAY_RECAP);
            }
            else {
                int32 bp0 = damage / 2;
                me->CastCustomSpell(attacker, AURA_OF_DESIRE_DAMAGE, &bp0, nullptr, nullptr, true);
            }
        }
        
        void onHitBySpell(Unit* caster, SpellEntry const* spell)
        {
            if (me->m_currentSpells[CURRENT_GENERIC_SPELL]) {
                for (uint8 i = 0; i < 3; ++i) {
                    if (spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST) {
                        if (me->m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id == SPELL_SOUL_SHOCK
                                || me->m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id == SPELL_DEADEN)
                            me->InterruptSpell(CURRENT_GENERIC_SPELL, false);
                    }
                }
            }
        }
        
        void onCombatStart(Unit* victim)
        {
            talk(TALK_DESI_SAY_FREED);
            setZoneInCombat();
            doCast(me, AURA_OF_DESIRE, true);
        }
        
        void onKill(Unit* victim)
        {
            talk(TALK_DESI_SAY_SLAY);
        }
        
        void update(uint32 const diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return;

            if (!updateVictim())
                return;
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_RUNE_SHIELD:
                    doCast(me, SPELL_RUNE_SHIELD, true);
                    delayEvent(EV_SOUL_SHOCK, 2000);
                    delayEvent(EV_DEADEN, 2000);
                    scheduleEvent(EV_RUNE_SHIELD, 15000);
                    break;
                case EV_SOUL_SHOCK:
                    doCast(me->GetVictim(), SPELL_SOUL_SHOCK);
                    scheduleEvent(EV_SOUL_SHOCK, 6000);
                    break;
                case EV_DEADEN:
                    doCast(me->GetVictim(), SPELL_DEADEN);
                    scheduleEvent(EV_DEADEN, 30000, 30000);
                    if ((rand() % 2) == 0)
                        talk(TALK_DESI_SAY_SPEC);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_essence_of_desireAI(creature);
    }
};

//Phase 3
class Boss_essence_of_anger : public CreatureScript
{
public:
    Boss_essence_of_anger() : CreatureScript("boss_essence_of_anger") {}
    
    class Boss_essence_of_angerAI : public CreatureAINew
    {
    public:
        Boss_essence_of_angerAI(Creature* creature) : CreatureAINew(creature) 
        { }
        
        enum events {
            EV_CHECK_TANK   = 0,
            EV_SOUL_SCREAM  = 1,
            EV_SPITE        = 2
        };
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_CHECK_TANK, 3000, 3000);
                addEvent(EV_SOUL_SCREAM, 10000, 10000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SPITE, 20000, 20000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_CHECK_TANK, 3000);
                scheduleEvent(EV_SOUL_SCREAM, 10000);
                scheduleEvent(EV_SPITE, 20000);
            }
            
            tankGUID = 0;
            spiteGUIDs.clear();
            //me->SetNoCallAssistance(true);
        }
        
        void evade()
        {
            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->evade();
                
            CreatureAINew::evade();
        }
        
        void onCombatStart(Unit* victim)
        {
            tankGUID = victim->GetGUID();
            victim->ApplySpellImmune(0, IMMUNITY_ID, SPELL_SPITE_TARGET, true); //else we would change target if it's casted on him
            
            talk(TALK_ANGER_SAY_FREED);
            setZoneInCombat();
            doCast(me, AURA_OF_ANGER, true);
        }
        
        void onDeath(Unit* killer)
        {
            Player* tank = me->GetPlayer(tankGUID);
            if(tank) tank->ApplySpellImmune(0, IMMUNITY_ID, SPELL_SPITE_TARGET, false);
            talk(TALK_ANGER_SAY_DEATH);

            if (Creature* reliquary = me->GetSummoner()->ToCreature())
                reliquary->getAI()->message(DATA_ESSENCE_OF_ANGER_DEATH, 0);
        }
        
        void onKill(Unit* victim)
        {
            talk(TALK_ANGER_SAY_SLAY);
        }
        
        void update(uint32 const diff)
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                return;

            if (!updateVictim())
                return;
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_CHECK_TANK:
                    if (me->GetVictim() && me->GetVictim()->GetGUID() != tankGUID) {
                        talk(TALK_ANGER_SAY_BEFORE);
                        doCast(me, SPELL_SELF_SEETHE, true);
                        tankGUID = me->GetVictim()->GetGUID();
                    }
                    scheduleEvent(EV_CHECK_TANK, 2000);
                    break;
                case EV_SOUL_SCREAM:
                    doCast(me->GetVictim(), SPELL_SOUL_SCREAM);
                    if ((rand() % 3) == 0)
                        talk(TALK_ANGER_SAY_SPEC);
                    scheduleEvent(EV_SOUL_SCREAM, 9000, 11000);
                    break;
                case EV_SPITE:
                    doCast(me, SPELL_SPITE_TARGET);
                    talk(TALK_ANGER_SAY_SPEC);
                    scheduleEvent(EV_SPITE, 20000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
        
    private:
        uint64 tankGUID;
        
        std::list<uint64> spiteGUIDs;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_essence_of_angerAI(creature);
    }
};

class Npc_enslaved_soul : public CreatureScript
{
public:
    Npc_enslaved_soul() : CreatureScript("npc_enslaved_soul") {}
    
    class Npc_enslaved_soulAI : public CreatureAINew
    {
    public:
        Npc_enslaved_soulAI(Creature* creature) : CreatureAINew(creature) 
        { 
            instance = ((ScriptedInstance*)creature->GetInstanceData());
            if(instance)
                reliquaryGUID = instance->GetData64(DATA_RELIQUARY_OF_SOULS);
        }
        
        void onReset(bool onSpawn)
        {
            if(onSpawn)
            {
                doCast(me,ENSLAVED_SOUL_PASSIVE, true);
                doCast(me,SPELL_SELFSTUN,true); //2.5s inactivity
                Creature* reliquary = me->GetMap()->GetCreatureInMap(reliquaryGUID);
                if (reliquary)
                    reliquary->getAI()->message(DATA_SOUL_SPAWN, me->GetGUIDLow());
            } else {
                me->DisappearAndDie();
            }
        }

        void onDeath(Unit* killer)
        {
            
            Creature* reliquary = me->GetMap()->GetCreatureInMap(reliquaryGUID);
            if (reliquary)
                reliquary->getAI()->message(DATA_SOUL_DEATH, 1);
                
            doCast(me, SPELL_SOUL_RELEASE, true);
        }        
    private:
        ScriptedInstance* instance;
        uint64 reliquaryGUID;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Npc_enslaved_soulAI(creature);
    };
};

void AddSC_boss_reliquary_of_souls()
{
    sScriptMgr.addScript(new Boss_reliquary_of_souls());
    sScriptMgr.addScript(new Boss_essence_of_suffering());
    sScriptMgr.addScript(new Boss_essence_of_desire());
    sScriptMgr.addScript(new Boss_essence_of_anger());
    sScriptMgr.addScript(new Npc_enslaved_soul());
}
