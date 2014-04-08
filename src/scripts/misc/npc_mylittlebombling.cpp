#include "precompiled.h"
#include "ObjectMgr.h"

#define BADABOUM_RANGE 24.0f
#define SPELL_VISUAL_BADABOUM 46225
#define CREATURE_BADABOUM 91448
#define WAITTIME 2500
#define TEXT_HOHI "Coucou !"
#define TEXT_GIMME_TARGET "Donnez moi une cible !"

struct mylittlebomblingAI : public ScriptedAI
{
    short phase;
    uint32 waitTimer;

    enum PHASES {
        IDLE,
        TRACKING,
        HOHI
    };

    mylittlebomblingAI(Creature* creature) : ScriptedAI(creature) 
    {} 

    void Reset() {
        m_creature->SetReactState(REACT_PASSIVE);
        phase = IDLE;
        waitTimer = WAITTIME;
    }
    
    void EnterCombat(Unit* who)
    {}

    void UpdateAI(uint32 const diff)
    {
        switch(phase)
        {
        case IDLE:
            break;
        case TRACKING:
            if(me->GetVictim())
            {
                if (me->GetDistance(me->GetVictim()) < 5)
                {
                    phase = HOHI;
                    me->Say(TEXT_HOHI,LANG_UNIVERSAL,NULL);
                    me->StopMoving();
                }
            } else {
                Reset();
            }
            break;
        case HOHI:
            if (waitTimer < diff)
            {
                Kaboom();
                me->DisappearAndDie();
            } else {
                waitTimer -= diff;
            }
            break;
        }
    }

    void Kaboom()
    {
        DoSpawnCreature(CREATURE_BADABOUM, 0, 0, 0, 0, TEMPSUMMON_TIMED_DESPAWN, 5000); //just for the effect

        Map *map = m_creature->GetMap();
        Map::PlayerList const &PlayerList = map->GetPlayers();

        std::list<Player*> playerlist;

        CellPair p(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();
                
        Trinity::AnyPlayerInObjectRangeCheck p_check(m_creature, BADABOUM_RANGE);
        Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck>  checker(playerlist, p_check);

        TypeContainerVisitor<Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck>, WorldTypeMapContainer > pSearcher(checker);
        cell.Visit(p, pSearcher, *(m_creature->GetMap()));

        if(!playerlist.size())
            return;

        for(std::list<Player*>::iterator i = playerlist.begin(); i != playerlist.end(); ++i)
        {
            if((*i) && !(*i)->isGameMaster())
            {
                me->DealDamage((*i), 500000, 0, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE, 0, false);
            }
        }
    }

    bool GoHurt(const char* Code)
    {
        std::string name = Code;

        if(!normalizePlayerName(name))
            return false;

        Player *target = objmgr.GetPlayer(name.c_str());

        if(target) {
            AttackStart(target);
            phase = TRACKING;
            return true;
        } else {
            return false;
        }
    }

    void JustDied(Unit* /* who */)
    {
        Kaboom();
    }
};

bool GossipHello_mylittlebombling(Player *player, Creature *_Creature)
{    
    if (((mylittlebomblingAI*)_Creature->AI())->phase == mylittlebomblingAI::IDLE)
        player->ADD_GOSSIP_ITEM_EXTENDED( 0, TEXT_GIMME_TARGET, GOSSIP_SENDER_MAIN, 1, "", 0, true);
        
        player->PlayerTalkClass->SendGossipMenu(3,_Creature->GetGUID());

    return true;
}

bool GossipSelectWithCode_mylittlebombling( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* Code )
{
    if (!((mylittlebomblingAI*)_Creature->AI())->GoHurt(Code))
        ((mylittlebomblingAI*)_Creature->AI())->DoSay("Moi pas trouver !",LANG_UNIVERSAL,NULL);

    player->PlayerTalkClass->CloseGossip();
    return true;
}

struct TRINITY_DLL_DECL mylittlebombling_visualAI : public ScriptedAI
{
    mylittlebombling_visualAI(Creature* creature) : ScriptedAI(creature) 
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        me->CastSpell(me, SPELL_VISUAL_BADABOUM, true);
    }

    void EnterCombat(Unit* who)
    {}
};


CreatureAI* GetAI_mylittlebombling(Creature *_Creature)
{
    return new mylittlebomblingAI (_Creature);
}

CreatureAI* GetAI_mylittlebombling_visual(Creature *_Creature)
{
    return new mylittlebombling_visualAI (_Creature);
}

void AddSC_mylittlebombling()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_mylittlebombling";
    newscript->GetAI = &GetAI_mylittlebombling;
    newscript->pGossipHello = &GossipHello_mylittlebombling;
    newscript->pGossipSelectWithCode = &GossipSelectWithCode_mylittlebombling;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_mylittlebombling_visual";
    newscript->GetAI = &GetAI_mylittlebombling_visual;
    newscript->RegisterSelf();
}
