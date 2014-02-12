#include "precompiled.h"
#include "def_hyjal.h"

enum {
    YELL_AGGRO  = 0,
    YELL_DOOMFIRE,
    YELL_AIR_BURST,
    YELL_SLAY,
    YELL_ENRAGE,
    YELL_DEATH,
    YELL_SOULCHARGE,
};

enum {
    SPELL_DENOUEMENT_WISP       = 32124,
    SPELL_ANCIENT_SPARK         = 39349,
    SPELL_PROTECTION_OF_ELUNE   = 38528,

    SPELL_DRAIN_WORLD_TREE      = 39140,
    SPELL_DRAIN_WORLD_TREE_2    = 39141,

    SPELL_FINGER_OF_DEATH       = 31984,                    // 20k dmg
    SPELL_HAND_OF_DEATH         = 35354,                    // 99k dmg
    SPELL_AIR_BURST             = 32014,                    // bump
    SPELL_GRIP_OF_THE_LEGION    = 31972,                    // dot, 2500/sec, 5 min
    
    SPELL_DOOMFIRE_STRIKE       = 31903,                    // summons two creatures
    SPELL_DOOMFIRE_SPAWN        = 32074,                    // spawn visual
    SPELL_DOOMFIRE_VISUAL       = 42344,                    // fire visual
    SPELL_DOOMFIRE_DAMAGE       = 31944,
    SPELL_DOOMFIRE_TRIGGER      = 31943,                    // triggers 31944 every second
    SPELL_DOOMFIRE_TRIG_TRIG    = 31945,                    // triggers 31943 every second (seems to be the one to use on the not walking fire)
    
    SPELL_SOUL_CHARGE_YELLOW    = 32045,
    SPELL_SOUL_CHARGE_GREEN     = 32051,
    SPELL_SOUL_CHARGE_RED       = 32052,
    
    SPELL_UNLEASH_SOUL_YELLOW   = 32054,
    SPELL_UNLEASH_SOUL_GREEN    = 32057,
    SPELL_UNLEASH_SOUL_RED      = 32053,
    
    SPELL_FEAR                  = 31970,
    
    DOOMFIRE_REFRESHTIMER       = 1000,
    DOOMFIRE_SUMMONTIMER        = 1000, //aka speed of the trail
    DOOMFIRE_DESPAWNTIME        = 18000
};

enum {
    CREATURE_DOOMFIRE               = 18095,
    CREATURE_DOOMFIRE_TARGETING     = 18104,
    CREATURE_ANCIENT_WISP           = 17946,
    CREATURE_CHANNEL_TARGET         = 22418
};

float WhispPos[12][3] =
{
    {5437.736816, -3433.736328, 1570.067749},
    {5547.231934, -3370.547607, 1580.132568},
    {5718.386230, -3339.741943, 1594.261230},
    {5623.342285, -3380.486328, 1584.710083},
    {5746.506348, -3439.762695, 1595.568726},
    {5729.705078, -3498.496338, 1599.565308},
    {5670.690918, -3543.238281, 1592.051514},
    {5650.103027, -3595.190918, 1602.952881},
    {5603.528809, -3577.025879, 1583.101318},
    {5504.924316, -3627.164062, 1573.354614},
    {5450.262207, -3482.258545, 1562.047974},
    {5521.988281, -3536.018311, 1562.865112},
};

#define NORDRASSIL_X        5503.713
#define NORDRASSIL_Y       -3523.436
#define NORDRASSIL_Z        1608.781

class Mob_Ancient_Whisp : public CreatureScript
{
public:
    Mob_Ancient_Whisp() : CreatureScript("mob_ancient_whisp") {}
    
    class Mob_Ancient_WhispAI : public CreatureAINew
    {
    public:
        enum events
        {
            EV_CHECK    = 0
        };
        
        Mob_Ancient_WhispAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn)
                addEvent(EV_CHECK, 1000, 1000);
            else
                scheduleEvent(EV_CHECK, 1000);

            aggro = false;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void onMoveInLoS(Unit* /*who*/)
        {
        }
        
        void onDamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            damage = 0;
        }
        
        void update(uint32 const diff)
        {
            me->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

            if (!aggro)
            {
                if (Creature* archimonde = _instance->instance->GetCreatureInMap(_instance->GetData64(DATA_ARCHIMONDE)))
                {
                    attackStart(archimonde);
                    doModifyThreat(archimonde, 1000000.0f);
                    aggro = true;
                }
            }

            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_CHECK:
                    if (Creature* archimonde = _instance->instance->GetCreatureInMap(_instance->GetData64(DATA_ARCHIMONDE)))
                    {
                        if (archimonde->IsBelowHPPercent(2.0f) || archimonde->isDead())
                            doCast(me, SPELL_DENOUEMENT_WISP);
                        else
                            doCast(archimonde, SPELL_ANCIENT_SPARK);
                    }
                    
                    scheduleEvent(EV_CHECK, urand (3000, 10000));
                    
                    break;
                }
            }
        }
    
    private:
        ScriptedInstance* _instance;

        bool aggro;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_Ancient_WhispAI(creature);
    }
};

class Mob_Doomfire : public CreatureScript
{
public:
    Mob_Doomfire() : CreatureScript("mob_doomfire") {}

    class Mob_DoomfireAI : public CreatureAINew
    {
    public:
        Mob_DoomfireAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
            _refreshTimer = DOOMFIRE_REFRESHTIMER;
            _lifeTime = DOOMFIRE_DESPAWNTIME;
            me->CastSpell(me, SPELL_DOOMFIRE_VISUAL, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void onDamageTaken(Unit* /*attacker*/, uint32& damage)	{ damage = 0; }

        void update(uint32 const diff)
        {
            me->SetReactState(REACT_PASSIVE);

            if (_lifeTime < 500)
                me->RemoveAura(SPELL_DOOMFIRE_VISUAL, 0); //esthetic

            if(_archimondeGUID)
            { 
                if (_refreshTimer < diff)
                {
                    IncinerateRecklessPlayers();
                    _refreshTimer = DOOMFIRE_REFRESHTIMER;
                } else {
                    _refreshTimer -= diff;
                }
            } else {
                if (_instance)
                    _archimondeGUID = _instance->GetData64(DATA_ARCHIMONDE);
            }
            _lifeTime -= diff;
        }

        void IncinerateRecklessPlayers()
        {
            Map *map = me->GetMap();
            Map::PlayerList const &PlayerList = map->GetPlayers();

            for(Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            {
                if (Player* i_pl = i->getSource())
                {
                    if (i_pl->IsAlive() && !i_pl->isGameMaster() && i_pl->GetDistance(me) <= 3)
                    {
                        //i_pl->CastSpell(i_pl, SPELL_DOOMFIRE_DAMAGE, true, 0, 0, _archimondeGUID); 
                        i_pl->CastSpell(i_pl, SPELL_DOOMFIRE_DAMAGE, true);
                        //soulcharges seems to be gained correctly.
                    }
                }
            }
        }

    private:
        ScriptedInstance* _instance;
        uint64 _archimondeGUID;
        uint32 _refreshTimer;
        uint32 _lifeTime;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_DoomfireAI(creature);
    }
};

/* This mob control the trajectory of the flamme trail and summons doomfires along the way. (His position doesnt matters.) */
class Mob_Doomfire_Targeting : public CreatureScript
{
public:
    Mob_Doomfire_Targeting() : CreatureScript("mob_doomfire_targeting") {}

    class Mob_Doomfire_TargetingAI : public CreatureAINew
    {
    public:
        Mob_Doomfire_TargetingAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
            _CurrentX = me->GetPositionX();
            _CurrentY = me->GetPositionY();
            _CurrentZ = me->GetPositionZ();
            
            if (_CurrentX > 20000.0f && _CurrentY > 20000.0f || _CurrentX < -20000.0f && _CurrentY < -20000.0f) {
                sLog.outString("[ARCHIMONDE] Spawned at (%f, %f), aborting to prevent crash on GetHeight()", _CurrentX, _CurrentY);
                me->DisappearAndDie();
                return;
            }
            
            _LastX = 0;
            _LastY = 0;
            _SummonTimer = DOOMFIRE_SUMMONTIMER;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->CastSpell(me, SPELL_DOOMFIRE_SPAWN, true);
            _archimondeGUID = NULL;
            _Archimonde = NULL;
            _CurrentTarget = FindNewFriend();

            if (_instance)
            {
                _archimondeGUID = _instance->GetData64(DATA_ARCHIMONDE);
                if (_archimondeGUID)
                {
                    _Archimonde = Unit::GetUnit((*me), _archimondeGUID);
                    if (_Archimonde)
                    {
                        _LastX = _Archimonde->GetPositionX(); //= set current orientation toward exterior
                        _LastY = _Archimonde->GetPositionY();
                    }
                }
            }
            
            if(!_CurrentTarget)
                turnAround(); //no target found, go the other way. This is to prevent some abuses.
        }

        void update(uint32 const diff)
        {
            me->SetReactState(REACT_PASSIVE);
            if(_archimondeGUID)
            {
                if (!_Archimonde)
                {
                    _Archimonde = Unit::GetUnit((*me), _archimondeGUID);
                }

                if(_Archimonde && _Archimonde->IsAlive() && _Archimonde->IsInCombat())
                {
                    if(_SummonTimer < diff)
                    {
                        _CurrentAngle = GetCurrentAngle();

                        float AngularDifference;
                        if (_Archimonde->GetDistance(_CurrentX, _CurrentY, _CurrentZ) < 70)
                        {
                            //if I already got a target, got to check if it's still valid.
                            if (_CurrentTarget)
                            {
                                float AngleWithTarget = GetAngleWithTarget(_CurrentTarget);
                                AngularDifference = GetAngularDifference(_CurrentAngle, AngleWithTarget);

                                if (fabs(AngularDifference) > 80)
                                    _CurrentTarget = NULL;
                            }

                            if (!_CurrentTarget) 
                            {
                                if (_CurrentTarget = FindNewFriend())
                                {
                                    float AngleWithTarget = GetAngleWithTarget(_CurrentTarget);
                                    AngularDifference = SoftenCurve(GetAngularDifference(_CurrentAngle, AngleWithTarget), 40); //diziz the mothfockin missile agility
                                } else {
                                    AngularDifference = 0; 
                                }
                            }
                        } else { //Oh fock Ive gone too far
                            float AngleWithTarget = GetAngleWithTarget(_Archimonde);
                            AngularDifference = SoftenCurve(GetAngularDifference(_CurrentAngle, AngleWithTarget), 45);
                        }

                        float NewAngle = _CurrentAngle + AngularDifference;
                        float NewX = _CurrentX + cos(DegreeToRadian(NewAngle)) * 6;
                        float NewY = _CurrentY + sin(DegreeToRadian(NewAngle)) * 6;
                        float NewZ = me->GetMap()->GetHeight(NewX, NewY, _CurrentZ);
                        me->UpdateAllowedPositionZ(NewX,NewY,NewZ);
                        //sLog.outString("Doomfire damage: %f %f %f", NewX, NewY, NewZ);
                        if (NewZ < 1500.0f)
                            NewZ = _CurrentZ;

                        Creature* Doomfire = me->SummonCreature(CREATURE_DOOMFIRE, NewX, NewY, NewZ, 0, TEMPSUMMON_TIMED_DESPAWN, DOOMFIRE_DESPAWNTIME);
                        if(Doomfire)
                        {
                            _LastX = _CurrentX;
                            _LastY = _CurrentY;
                            _CurrentX = NewX;
                            _CurrentY = NewY;
                            _CurrentZ = NewZ;
                            _SummonTimer = DOOMFIRE_SUMMONTIMER;
                        }
                    } else { 
                        _SummonTimer -= diff;
                    }
                }
            } else {
                if (_instance)
                    _archimondeGUID = _instance->GetData64(DATA_ARCHIMONDE);
                else
                    _instance = ((ScriptedInstance*)me->GetInstanceData());
            }
        }

        Player* FindNewFriend()
        {
            Map *map = me->GetMap();
            Map::PlayerList const &PlayerList = map->GetPlayers();
            std::vector<Player *> target_list;
            Player* NewFriend = NULL;

            for(Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            {
                if (Player* i_pl = i->getSource())
                {	
                    if  (i_pl->isAttackableByAOE() && i_pl->GetDistance(_CurrentX, _CurrentY, _CurrentZ) <= 100)
                    {
                        float AngleWithTarget = GetAngleWithTarget(i_pl);
                        float AngularDifference = GetAngularDifference(_CurrentAngle, AngleWithTarget);
                        if (fabs(AngularDifference) < 90)
                            target_list.push_back(i_pl);
                    }
                }
            }
            if(target_list.size() > 0)
                NewFriend = *(target_list.begin()+rand()%target_list.size());

            return NewFriend; //Player* or NULL
        }

        float SoftenCurve(int givenangle, int maxangle)
        {
            if (givenangle > maxangle)
                return maxangle;
            else if (givenangle < -maxangle)
                return -maxangle;
            else
                return givenangle;
        }

        float GetAngleWithTarget(Unit* target) // from -90 to 270
        {
            float angle;
            if (target->GetPositionX() - _CurrentX > 0)
                angle = RadianToDegree(atan((target->GetPositionY() - _CurrentY)/(target->GetPositionX() - _CurrentX)));
            else
                angle = 180 + RadianToDegree(atan((target->GetPositionY() - _CurrentY)/(target->GetPositionX() - _CurrentX)));
            return angle;
        }

        float GetCurrentAngle() // from -90 to 270
        {	
            float angle;
            if (_CurrentX - _LastX > 0)
                angle = RadianToDegree(atan((_CurrentY - _LastY)/(_CurrentX - _LastX)));
            else
                angle = 180 + RadianToDegree(atan((_CurrentY - _LastY)/(_CurrentX - _LastX)));

            return angle;
        }

        void turnAround()
        {
            _LastX += (_CurrentX - _LastX)*2;
            _LastY += (_CurrentY - _LastY)*2;
        }

        float GetAngularDifference(float ang1, float ang2)
        {
            float diff = ang2 - ang1;
            if (diff > 180)
                diff -= 360;
            else if (diff < -180)
                diff += 360;

            return diff;
        }

        float RadianToDegree(float radian)  { return radian  * (180/3.14159265); }
        float DegreeToRadian(float degree) { return degree / (180/3.14159265); }

    private:
        ScriptedInstance* _instance;
        uint32 _SummonTimer;
        uint64 _archimondeGUID;
        Unit* _Archimonde;

        float _CurrentX;
        float _CurrentY;
        float _CurrentZ;
        float _LastX;
        float _LastY;
        float _CurrentAngle;
        Player* _CurrentTarget;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_Doomfire_TargetingAI(creature);
    }
};

// This is used to sort by distance in order to see who is the closest target, when checking for Finger of Death
struct TargetDistanceOrder : public std::binary_function<const Unit, const Unit, bool>
{
    const Unit* MainTarget;
    TargetDistanceOrder(const Unit* Target) : MainTarget(Target) {};
    // functor for operator "<"
    bool operator()(const Unit* _Left, const Unit* _Right) const
    {
        return (MainTarget->GetDistance(_Left) < MainTarget->GetDistance(_Right));
    }
};

class Boss_Archimonde : public CreatureScript
{
public:
    Boss_Archimonde() : CreatureScript("boss_archimonde") {}
    
    class Boss_ArchimondeAI : public CreatureAINew
    {
    public:
        enum events
        {
            EV_FEAR = 0,
            EV_AIR_BURST,
            EV_GRIP_LEGION,
            EV_DOOMFIRE,
            EV_MELEE_CHECK,
            EV_NORDRASSIL_CHECK,
            EV_ENRAGE,
            EV_ENRAGE_CAST,
            EV_UNLEASH_SOULCHARGE,
            EV_UNDER_10_PERCENT,
            EV_UNDER_10_PERCENT2
        };
    
        Boss_ArchimondeAI(Creature* creature) : CreatureAINew(creature), Summons(me)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_FEAR, 42000, 42000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_AIR_BURST, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_GRIP_LEGION, 5000, 25000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_DOOMFIRE, 10000, 10000);
                addEvent(EV_MELEE_CHECK, 5000, 5000);
                addEvent(EV_NORDRASSIL_CHECK, 3000, 3000);
                addEvent(EV_ENRAGE, 600000, 600000);
                addEvent(EV_ENRAGE_CAST, 2000, 2000, EVENT_FLAG_NONE, false);
                addEvent(EV_UNLEASH_SOULCHARGE, 2000, 10000);
                addEvent(EV_UNDER_10_PERCENT, 1000, 1000, EVENT_FLAG_DELAY_IF_CASTING, false);
                addEvent(EV_UNDER_10_PERCENT2, 1000, 1000, EVENT_FLAG_DELAY_IF_CASTING, false);
            }
            else {
                scheduleEvent(EV_FEAR, 42000);
                scheduleEvent(EV_AIR_BURST, 30000);
                scheduleEvent(EV_GRIP_LEGION, 5000, 25000);
                scheduleEvent(EV_DOOMFIRE, 10000);
                scheduleEvent(EV_MELEE_CHECK, 5000);
                scheduleEvent(EV_NORDRASSIL_CHECK, 3000);
                enableEvent(EV_NORDRASSIL_CHECK);
                scheduleEvent(EV_ENRAGE, 600000);
                enableEvent(EV_ENRAGE);
                scheduleEvent(EV_ENRAGE_CAST, 2000);
                disableEvent(EV_ENRAGE_CAST);
                scheduleEvent(EV_UNLEASH_SOULCHARGE, 2000, 10000);
                scheduleEvent(EV_UNDER_10_PERCENT, 1000, 1000);
                disableEvent(EV_UNDER_10_PERCENT);
                scheduleEvent(EV_UNDER_10_PERCENT2, 1000, 1000);
                disableEvent(EV_UNDER_10_PERCENT2);
            }

            _enraged = false;
            _under10Percent = false;
            _checkTimer = 1000;

            me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_YELLOW);
            me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_RED);
            me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_GREEN);

            _chargeCount = 0;

            if (_instance)
                _instance->SetData(DATA_ARCHIMONDEEVENT, NOT_STARTED);
        }
        
        void message(uint32 type, uint32 data)
        {
            if (type == 0) {
                talk(YELL_SLAY);
                switch (data) {
                case CLASS_PRIEST:
                case CLASS_PALADIN:
                case CLASS_WARLOCK:
                    doCast(me, SPELL_SOUL_CHARGE_RED, true);
                    break;
                case CLASS_MAGE:
                case CLASS_ROGUE:
                case CLASS_WARRIOR:
                    doCast(me, SPELL_SOUL_CHARGE_YELLOW, true);
                    break;
                case CLASS_DRUID:
                case CLASS_SHAMAN:
                case CLASS_HUNTER:
                    doCast(me, SPELL_SOUL_CHARGE_GREEN, true);
                    break;
                }
            }
        }

        void onSummon(Creature* summoned)
        {
            Summons.Summon(summoned);
        }

        void onSummonDespawn(Creature* unit)
        {
            Summons.Despawn(unit);
        }

        void evade()
        {
            Summons.DespawnEntry(CREATURE_ANCIENT_WISP);
            CreatureAINew::evade();
        }

        void onDeath(Unit* /*killer*/)
        {
            talk(YELL_DEATH);
            if (_instance)
                _instance->SetData(DATA_ARCHIMONDEEVENT, DONE);
        }
        
        void onCombatStart(Unit* /*victim*/)
        {
            if (_instance)
                _instance->SetData(DATA_ARCHIMONDEEVENT, IN_PROGRESS);
        }

        void attackStart(Unit* victim)
        {
            if (!victim)
                return;

            if (victim->GetEntry() == CREATURE_ANCIENT_WISP)
            {
                attackStart(selectUnit(SELECT_TARGET_RANDOM, 0, 1000.0f, true));
                return;
            }
            CreatureAINew::attackStart(victim);
        }

        void update(uint32 const diff)
        {
            if (!me->IsInCombat()) {
                if (_checkTimer <= diff) {
                    // Visibility check
                    if ((_instance->GetData(DATA_AZGALOREVENT) < DONE) && ((me->GetVisibility() != VISIBILITY_OFF) || (me->getFaction() != 35))) {
                        me->SetVisibility(VISIBILITY_OFF);
                        me->setFaction(35);
                    }
                    else if ((_instance->GetData(DATA_AZGALOREVENT) >= DONE) && ((me->GetVisibility() != VISIBILITY_ON) || (me->getFaction() == 35))) {
                        me->setFaction(1720);
                        me->SetVisibility(VISIBILITY_ON);
                    }
                    
                    // Refresh channeling visual
                    if (!me->m_currentSpells[CURRENT_CHANNELED_SPELL]) {
                        if (Creature* trigger = me->SummonCreature(CREATURE_CHANNEL_TARGET, NORDRASSIL_X, NORDRASSIL_Y, NORDRASSIL_Z, 0, TEMPSUMMON_MANUAL_DESPAWN, 0))
                            doCast(trigger, SPELL_DRAIN_WORLD_TREE);
                    }
                    
                    if (Creature* trigger = me->SummonCreature(CREATURE_CHANNEL_TARGET, NORDRASSIL_X, NORDRASSIL_Y, NORDRASSIL_Z, 0, TEMPSUMMON_TIMED_DESPAWN, 2000)) {
                        trigger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        trigger->SetUInt32Value(UNIT_FIELD_DISPLAYID, 11686);
                        trigger->CastSpell(me, SPELL_DRAIN_WORLD_TREE_2, true);
                    }
                    
                    me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_YELLOW);
                    me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_RED);
                    me->RemoveAurasDueToSpell(SPELL_SOUL_CHARGE_GREEN);
                    
                    _checkTimer = 1000;
                }
                else
                    _checkTimer -= diff;
                
                return;
            }
            
            if (!updateVictim())
                return;
            
            if (me->IsBelowHPPercent(10.0f) && !_under10Percent) {
                _under10Percent = true;
                enableEvent(EV_UNDER_10_PERCENT);
                enableEvent(EV_UNDER_10_PERCENT2);
                doCast(me->GetVictim(), SPELL_PROTECTION_OF_ELUNE, true);
            }
                
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_FEAR:
                    doCast(me->GetVictim(), SPELL_FEAR);
                    scheduleEvent(EV_FEAR, 42000);
                    delayEvent(EV_AIR_BURST, 5000);
                    break;
                case EV_AIR_BURST:
                    talk(YELL_AIR_BURST);
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 1, 150.0f, true), SPELL_AIR_BURST);
                    scheduleEvent(EV_AIR_BURST, 25000, 40000);
                    delayEvent(EV_FEAR, 5000);
                    break;
                case EV_GRIP_LEGION:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_GRIP_OF_THE_LEGION);
                    scheduleEvent(EV_GRIP_LEGION, 5000, 25000);
                    break;
                case EV_DOOMFIRE:
                    //spawn at random position at 13m
                    float x, y, z, tX, tY, tZ;
                    me->GetPosition(x,y,z);
                    tX = x + cos(rand()%6) * 13;
                    tY = y + sin(rand()%6) * 13;

                    tZ = z + 1.5f;
                    me->UpdateGroundPositionZ(tX, tY, tZ);
                    tZ += 1.0f;
                    if (me->SummonCreature(CREATURE_DOOMFIRE_TARGETING, tX, tY, tZ, 0, TEMPSUMMON_TIMED_DESPAWN, (rand()%5000 + 15000)))
                        sLog.outDebug("[ARCHIMONDE] Spawned Doomfire at %f %f %f", x, y, z);
                    else
                        sLog.outDebug("[ARCHIMONDE] Failed to spawn Doomfire");
                    talk(YELL_DOOMFIRE);
                    
                    scheduleEvent(EV_DOOMFIRE, 10000);
                    break;
                case EV_MELEE_CHECK:
                    if (_canUseFingerOfDeath())
                        doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FINGER_OF_DEATH);
                        
                    scheduleEvent(EV_MELEE_CHECK, 5000);
                    break;
                case EV_NORDRASSIL_CHECK:
                    if (me->GetDistance(NORDRASSIL_X, NORDRASSIL_Y, NORDRASSIL_Z) < 75.0f)
                        scheduleEvent(EV_ENRAGE, 1);
                    
                    scheduleEvent(EV_NORDRASSIL_CHECK, 2000);
                        
                    break;
                case EV_ENRAGE:
                    disableEvent(EV_NORDRASSIL_CHECK);
                    disableEvent(EV_ENRAGE);
                    enableEvent(EV_ENRAGE_CAST);
                    talk(YELL_ENRAGE);
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MoveIdle();
                    _enraged = true;
                    break;
                case EV_ENRAGE_CAST:
                    doCast(me->GetVictim(), SPELL_HAND_OF_DEATH);
                    scheduleEvent(EV_ENRAGE_CAST, 2000);
                    break;
                case EV_UNLEASH_SOULCHARGE:
                {
                    std::list<uint32> unleashSpells;
                    if (me->HasAura(SPELL_SOUL_CHARGE_GREEN))
                        unleashSpells.push_back(SPELL_UNLEASH_SOUL_GREEN);
                    if (me->HasAura(SPELL_SOUL_CHARGE_RED))
                        unleashSpells.push_back(SPELL_UNLEASH_SOUL_RED);
                    if (me->HasAura(SPELL_SOUL_CHARGE_YELLOW))
                        unleashSpells.push_back(SPELL_UNLEASH_SOUL_YELLOW);
                    
                    if (unleashSpells.empty()) {
                        scheduleEvent(EV_UNLEASH_SOULCHARGE, 2000, 10000);
                        break;
                    }
                        
                    Trinity::RandomResizeList(unleashSpells, 1);
                    doCast(me->GetVictim(), unleashSpells.front(), true);
                    
                    scheduleEvent(EV_UNLEASH_SOULCHARGE, 2000, 10000);
                    switch (unleashSpells.front()) {
                    case SPELL_UNLEASH_SOUL_GREEN:
                        me->RemoveSingleAuraFromStack(SPELL_SOUL_CHARGE_GREEN, 0);
                        break;
                    case SPELL_UNLEASH_SOUL_RED:
                        me->RemoveSingleAuraFromStack(SPELL_SOUL_CHARGE_RED, 0);
                        break;
                    case SPELL_UNLEASH_SOUL_YELLOW:
                        me->RemoveSingleAuraFromStack(SPELL_SOUL_CHARGE_YELLOW, 0);
                        break;
                    }
                    
                    break;
                }
                case EV_UNDER_10_PERCENT:
                    doCast(me, SPELL_HAND_OF_DEATH);
                    scheduleEvent(EV_UNDER_10_PERCENT, 3000);
                    break;
                case EV_UNDER_10_PERCENT2:
                    for (uint8 i = 0; i < 3; ++i)
                    {
                        uint8 j = rand()%13;
                        me->SummonCreature(CREATURE_ANCIENT_WISP, WhispPos[j][0] + ((2 * rand()%1000) / 1000.0f), WhispPos[j][1] + ((2 * rand()%1000) / 1000.0f), WhispPos[j][2], 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000);
                    }
                    scheduleEvent(EV_UNDER_10_PERCENT2, 3000, 5000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
        
    private:
        bool _canUseFingerOfDeath()
        {
            Unit* victim = me->GetVictim();
            if (victim && me->IsWithinDistInMap(victim, me->GetAttackDistance(victim)))
                return false;
                
            if (victim && victim->IsAlive()) {
                float x, y, z, zHeightMap;
                me->GetPosition(x, y, z);
                zHeightMap = me->GetMap()->GetHeight(x, y, z);

                // Victim is flying
                if ((z - zHeightMap) > 5.0f)
                    return false;
            }
            
            std::list<HostilReference*>& threatList = me->getThreatManager().getThreatList();
            if (threatList.empty())
                return false;
                
            std::list<Unit*> targets;
            std::list<HostilReference*>::iterator itr = threatList.begin();
            for (; itr != threatList.end(); ++itr) {
                Unit* unit = Unit::GetUnit((*me), (*itr)->getUnitGuid());
                if (unit && unit->IsAlive())
                    targets.push_back(unit);
            }

            if (targets.empty())
                return false;

            targets.sort(TargetDistanceOrder(me));
            Unit* target = targets.front();
            if (target) {
                if (!me->IsWithinDistInMap(target, me->GetAttackDistance(target)) && abs(me->GetPositionZ() - target->GetPositionZ()) < 5.0f)
                    return true; // Cast Finger of Death
                else // This target is closest, he is our new tank
                    me->AddThreat(target, doGetThreat(me->GetVictim()));
            }

            return false;
        }
    
        ScriptedInstance* _instance;
        SummonList Summons;
        
        bool _enraged;
        bool _under10Percent;
        
        uint32 _checkTimer;
        uint32 _chargeCount;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_ArchimondeAI(creature);
    }
};

void AddSC_boss_archimonde_new()
{
    sScriptMgr.addScript(new Mob_Ancient_Whisp());
    sScriptMgr.addScript(new Mob_Doomfire());
    sScriptMgr.addScript(new Mob_Doomfire_Targeting());
    sScriptMgr.addScript(new Boss_Archimonde());
}
