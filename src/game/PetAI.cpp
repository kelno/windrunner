/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
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

#include "PetAI.h"
#include "Errors.h"
#include "Pet.h"
#include "Player.h"
#include "Database/DBCStores.h"
#include "Spell.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"
#include "Creature.h"
#include "World.h"
#include "Util.h"
#include "CreatureAINew.h"

int PetAI::Permissible(const Creature *creature)
{
    if( creature->isPet())
        return PERMIT_BASE_SPECIAL;

    return PERMIT_BASE_NO;
}

PetAI::PetAI(Creature *c) : CreatureAI(c), i_tracker(TIME_INTERVAL_LOOK), distanceCheckTimer(3000)
{
    m_AllySet.clear();
    UpdateAllies();
}

bool PetAI::_needToStop() const
{
    // This is needed for charmed creatures, as once their target was reset other effects can trigger threat
    if(me->isCharmed() && me->getVictim() == me->GetCharmer())
        return true;

    if (me->GetOwner() && me->GetOwner()->ToPlayer() && me->ToPet() && me->ToPet()->isControlled() && me->getVictim()->IsJustCCed() && me->getVictim()->GetEntry() != 10) // Training dummy exception
        return true;

    return !me->canAttack(me->getVictim());
}

void PetAI::_stopAttack()
{
    if( !me->isAlive() )
    {
        DEBUG_LOG("Creature stoped attacking cuz his dead [guid=%u]", me->GetGUIDLow());
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MoveIdle();
        me->CombatStop();
        me->getHostilRefManager().deleteReferences();

        return;
    }

    me->AttackStop();
    me->InterruptNonMeleeSpells(false);
    me->SendAttackStop(); // Should stop pet's attack button from flashing
    me->GetCharmInfo()->SetIsCommandAttack(false);
    ClearCharmInfoFlags();
    HandleReturnMovement();
}

void PetAI::UpdateAI(const uint32 diff)
{
    if (!me->isAlive())
        return;

    Unit* owner = me->GetCharmerOrOwner();

    if(m_updateAlliesTimer <= diff)
        // UpdateAllies self set update timer
        UpdateAllies();
    else
        m_updateAlliesTimer -= diff;

    if (me->getVictim() && me->getVictim()->isAlive())
    {
        if( _needToStop() )
        {
            DEBUG_LOG("Pet AI stoped attacking [guid=%u]", me->GetGUIDLow());
            _stopAttack();
            return;
        }

        // Check before attacking to prevent pets from leaving stay position
        if (me->GetCharmInfo()->HasCommandState(COMMAND_STAY))
        {
            if (me->GetCharmInfo()->IsCommandAttack() || (me->GetCharmInfo()->IsAtStay() && me->IsWithinMeleeRange(me->getVictim())))
                DoMeleeAttackIfReady();
        }
        else
            DoMeleeAttackIfReady();
    }
    else
    {
        if (me->HasReactState(REACT_AGGRESSIVE) || me->GetCharmInfo()->IsAtStay())
        {
            // Every update we need to check targets only in certain cases
            // Aggressive - Allow auto select if owner or pet don't have a target
            // Stay - Only pick from pet or owner targets / attackers so targets won't run by
            //   while chasing our owner. Don't do auto select.
            // All other cases (ie: defensive) - Targets are assigned by AttackedBy(), OwnerAttackedBy(), OwnerAttacked(), etc.
            Unit* nextTarget = SelectNextTarget(me->HasReactState(REACT_AGGRESSIVE));

            if (nextTarget)
                AttackStart(nextTarget);
            else
                HandleReturnMovement();
        }
        else
            HandleReturnMovement();
    }

    if(!me->GetCharmInfo())
        return;

    if (me->GetGlobalCooldown() == 0 && !me->hasUnitState(UNIT_STAT_CASTING))
    {
        bool inCombat = me->getVictim();

        //Autocast
        for (uint8 i = 0; i < me->GetPetAutoSpellSize(); i++)
        {
            uint32 spellID = me->GetPetAutoSpellOnPos(i);
            if (!spellID)
                continue;

            SpellEntry const *spellInfo = spellmgr.LookupSpell(spellID);
            if (!spellInfo)
                continue;

            // ignore some combinations of combat state and combat/noncombat spells
            if (!inCombat)
            {
                if (!IsPositiveSpell(spellInfo->Id))
                    continue;
            }
            else
            {
                if (IsNonCombatSpell(spellInfo))
                    continue;
            }

            Spell *spell = new Spell(me, spellInfo, false, 0);

            if(inCombat && !me->hasUnitState(UNIT_STAT_FOLLOW) && spell->CanAutoCast(me->getVictim()))
            {
                //m_targetSpellStore.push_back(std::make_pair<Unit*, Spell*>(i_pet.getVictim(), spell));
				m_targetSpellStore.push_back(std::make_pair(me->getVictim(), spell));
                continue;
            }
            else
            {
                bool spellUsed = false;
                for(std::set<uint64>::iterator tar = m_AllySet.begin(); tar != m_AllySet.end(); ++tar)
                {
                    Unit* Target = ObjectAccessor::GetUnit(*me,*tar);

                    //only buff targets that are in combat, unless the spell can only be cast while out of combat
                    if(!Target)
                        continue;

                    if(spell->CanAutoCast(Target))
                    {
                        //m_targetSpellStore.push_back(std::make_pair<Unit*, Spell*>(Target, spell));
                        m_targetSpellStore.push_back(std::make_pair(Target, spell));
                        spellUsed = true;
                        break;
                    }
                }
                if (!spellUsed)
                    delete spell;
            }
        }

        //found units to cast on to
        if(!m_targetSpellStore.empty())
        {
            uint32 index = urand(0, m_targetSpellStore.size() - 1);

            Spell* spell  = m_targetSpellStore[index].second;
            Unit*  target = m_targetSpellStore[index].first;

            m_targetSpellStore.erase(m_targetSpellStore.begin() + index);

            SpellCastTargets targets;
            targets.setUnitTarget( target );

            if( !me->HasInArc(M_PI, target) )
            {
                me->SetInFront(target);
                if( target->GetTypeId() == TYPEID_PLAYER )
                    me->SendUpdateToPlayer( target->ToPlayer() );

                if(owner && owner->GetTypeId() == TYPEID_PLAYER)
                    me->SendUpdateToPlayer( owner->ToPlayer() );
            }

            me->AddCreatureSpellCooldown(spell->m_spellInfo->Id);
            if(me->isPet())
                ((Pet*)me)->CheckLearning(spell->m_spellInfo->Id);

            spell->prepare(&targets);
        }
        while (!m_targetSpellStore.empty())
        {
            delete m_targetSpellStore.begin()->second;
            m_targetSpellStore.erase(m_targetSpellStore.begin());
        }

        if(me->isPet() && ((Pet*)me)->getPetType() == MINI_PET)
        {
            Minipet_DistanceCheck(diff);
        }
    }
}

void PetAI::UpdateAllies()
{
    Unit* owner = me->GetCharmerOrOwner();
    Group *pGroup = NULL;

    m_updateAlliesTimer = 10000;                            //update friendly targets every 10 seconds, lesser checks increase performance

    if(!owner)
        return;
    else if(owner->GetTypeId() == TYPEID_PLAYER)
        pGroup = (owner->ToPlayer())->GetGroup();

    //only pet and owner/not in group->ok
    if(m_AllySet.size() == 2 && !pGroup)
        return;
    //owner is in group; group members filled in already (no raid -> subgroupcount = whole count)
    if(pGroup && !pGroup->isRaidGroup() && m_AllySet.size() == (pGroup->GetMembersCount() + 2))
        return;

    m_AllySet.clear();
    m_AllySet.insert(me->GetGUID());
    if(pGroup)                                              //add group
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();
            if(!Target || !pGroup->SameSubGroup(owner->ToPlayer(), Target))
                continue;

            if(Target->GetGUID() == owner->GetGUID())
                continue;

            m_AllySet.insert(Target->GetGUID());
        }
    }
    else                                                    //remove group
        m_AllySet.insert(owner->GetGUID());
}

void PetAI::KilledUnit(Unit* victim)
{
    // Called from Unit::Kill() in case where pet or owner kills something
    // if owner killed this victim, pet may still be attacking something else
    if (me->getVictim() && me->getVictim() != victim)
        return;

    // Clear target just in case. May help problem where health / focus / mana
    // regen gets stuck. Also resets attack command.
    // Can't use _stopAttack() because that activates movement handlers and ignores
    // next target selection
    me->AttackStop();
    me->InterruptNonMeleeSpells(false);
    me->SendAttackStop();  // Stops the pet's 'Attack' button from flashing

    // Before returning to owner, see if there are more things to attack
    if (Unit* nextTarget = SelectNextTarget(false))
        AttackStart(nextTarget);
    else
        HandleReturnMovement(); // Return
}

void PetAI::AttackStart(Unit* target)
{
    // Overrides Unit::AttackStart to correctly evaluate Pet states

    // Check all pet states to decide if we can attack this target
    if (!CanAttack(target))
        return;

    // Only chase if not commanded to stay or if stay but commanded to attack
    DoAttack(target, (!me->GetCharmInfo()->HasCommandState(COMMAND_STAY) || me->GetCharmInfo()->IsCommandAttack()));
}

void PetAI::OwnerAttackedBy(Unit* attacker)
{
    // Called when owner takes damage. This function helps keep pets from running off
    //  simply due to owner gaining aggro.

    if (!attacker)
        return;

    // Passive pets don't do anything
    if (me->HasReactState(REACT_PASSIVE))
        return;

    // Prevent pet from disengaging from current target
    if (me->getVictim() && me->getVictim()->isAlive())
        return;

    // Continue to evaluate and attack if necessary
    AttackStart(attacker);
}

void PetAI::OwnerAttacked(Unit* target)
{
    // Called when owner attacks something. Allows defensive pets to know
    //  that they need to assist

    // Target might be NULL if called from spell with invalid cast targets
    if (!target)
        return;

    // Passive pets don't do anything
    if (me->HasReactState(REACT_PASSIVE))
        return;

    // Prevent pet from disengaging from current target
    if (me->getVictim() && me->getVictim()->isAlive())
        return;

    // Continue to evaluate and attack if necessary
    AttackStart(target);
}

Unit* PetAI::SelectNextTarget(bool allowAutoSelect) const
{
    // Provides next target selection after current target death.
    // This function should only be called internally by the AI
    // Targets are not evaluated here for being valid targets, that is done in _CanAttack()
    // The parameter: allowAutoSelect lets us disable aggressive pet auto targeting for certain situations

    // Passive pets don't do next target selection
    if (me->HasReactState(REACT_PASSIVE))
        return NULL;

    // Check pet attackers first so we don't drag a bunch of targets to the owner
    if (Unit* myAttacker = me->getAttackerForHelper())
        return myAttacker;

    // Not sure why we wouldn't have an owner but just in case...
    if (!me->GetCharmerOrOwner())
        return NULL;

    // Check owner attackers
    if (Unit* ownerAttacker = me->GetCharmerOrOwner()->getAttackerForHelper())
        return ownerAttacker;

    // Neither pet or owner had a target and aggressive pets can pick any target
    // To prevent aggressive pets from chain selecting targets and running off, we
    //  only select a random target if certain conditions are met.
    if (me->HasReactState(REACT_AGGRESSIVE) && allowAutoSelect)
    {
        if (!me->GetCharmInfo()->IsReturning() || me->GetCharmInfo()->IsFollowing() || me->GetCharmInfo()->IsAtStay())
            if (Unit* nearTarget = me->ToCreature()->SelectNearestHostileUnitInAggroRange(true))
                return nearTarget;
    }

    // Default - no valid targets
    return NULL;
}

void PetAI::HandleReturnMovement()
{
    // Handles moving the pet back to stay or owner

    // Prevent activating movement when under control of spells
    // such as "Eyes of the Beast"
    //if (me->isCharmed())
        //return;

    if (me->GetCharmInfo()->HasCommandState(COMMAND_STAY))
    {
        if (!me->GetCharmInfo()->IsAtStay() && !me->GetCharmInfo()->IsReturning())
        {
            // Return to previous position where stay was clicked
            float x, y, z;

            me->GetCharmInfo()->GetStayPosition(x, y, z);
            ClearCharmInfoFlags();
            me->GetCharmInfo()->SetIsReturning(true);
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MovePoint(me->GetGUIDLow(), x, y, z);
        }
    }
    else // COMMAND_FOLLOW
    {
        if (!me->GetCharmInfo()->IsFollowing() && !me->GetCharmInfo()->IsReturning())
        {
            ClearCharmInfoFlags();
            me->GetCharmInfo()->SetIsReturning(true);
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveFollow(me->GetCharmerOrOwner(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        }
    }
}

void PetAI::DoAttack(Unit* target, bool chase)
{
    // Handles attack with or without chase and also resets flags
    // for next update / creature kill

    if (me->Attack(target, true))
    {
        if (Unit* owner = me->GetOwner())
            owner->SetInCombatWith(target);

        // Play sound to let the player know the pet is attacking something it picked on its own
        if (me->HasReactState(REACT_AGGRESSIVE) && !me->GetCharmInfo()->IsCommandAttack())
            me->SendPetAIReaction(me->GetGUID());

        if (chase)
        {
            bool oldCmdAttack = me->GetCharmInfo()->IsCommandAttack(); // This needs to be reset after other flags are cleared
            ClearCharmInfoFlags();
            me->GetCharmInfo()->SetIsCommandAttack(oldCmdAttack); // For passive pets commanded to attack so they will use spells
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveChase(target);
        }
        else // (Stay && ((Aggressive || Defensive) && In Melee Range)))
        {
            ClearCharmInfoFlags();
            me->GetCharmInfo()->SetIsAtStay(true);
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
        }
    }
}

void PetAI::MovementInform(uint32 moveType, uint32 data)
{
    // Receives notification when pet reaches stay or follow owner
    switch (moveType)
    {
        case POINT_MOTION_TYPE:
        {
            // Pet is returning to where stay was clicked. data should be
            // pet's GUIDLow since we set that as the waypoint ID
            if (data == me->GetGUIDLow() && me->GetCharmInfo()->IsReturning())
            {
                ClearCharmInfoFlags();
                me->GetCharmInfo()->SetIsAtStay(true);
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveIdle();
            }
            break;
        }
        case FOLLOW_MOTION_TYPE:
        {
            // If data is owner's GUIDLow then we've reached follow point,
            // otherwise we're probably chasing a creature
            if (me->GetCharmerOrOwner() && me->GetCharmInfo() && data == me->GetCharmerOrOwner()->GetGUIDLow() && me->GetCharmInfo()->IsReturning())
            {
                ClearCharmInfoFlags();
                me->GetCharmInfo()->SetIsFollowing(true);
            }
            break;
        }
        default:
            break;
    }
}

bool PetAI::CanAttack(Unit* target)
{
    // Evaluates wether a pet can attack a specific target based on CommandState, ReactState and other flags
    // IMPORTANT: The order in which things are checked is important, be careful if you add or remove checks

    // Hmmm...
    if (!target || !me->GetCharmInfo())
        return false;

    if (!target->isAlive())
    {
        // Clear target to prevent getting stuck on dead targets
        me->AttackStop();
        me->InterruptNonMeleeSpells(false);
        me->SendAttackStop();
        return false;
    }

    // Passive - passive pets can attack if told to
    if (me->HasReactState(REACT_PASSIVE))
        return me->GetCharmInfo()->IsCommandAttack();

    // Returning - pets ignore attacks only if owner clicked follow
    if (me->GetCharmInfo()->IsReturning())
        return !me->GetCharmInfo()->IsCommandFollow();

    // Stay - can attack if target is within range or commanded to
    if (me->GetCharmInfo()->HasCommandState(COMMAND_STAY))
        return (me->IsWithinMeleeRange(target) || me->GetCharmInfo()->IsCommandAttack());

    //  Pets attacking something (or chasing) should only switch targets if owner tells them to
    if (me->getVictim() && me->getVictim() != target)
    {
        // Check if our owner selected this target and clicked "attack"
        Unit* ownerTarget = NULL;
        if (Player* owner = me->GetCharmerOrOwner()->ToPlayer())
            ownerTarget = owner->GetSelectedUnit();
        else
            ownerTarget = me->GetCharmerOrOwner()->getVictim();

        if (ownerTarget && me->GetCharmInfo()->IsCommandAttack())
            return (target->GetGUID() == ownerTarget->GetGUID());
    }

    // Follow
    if (me->GetCharmInfo()->HasCommandState(COMMAND_FOLLOW))
        return !me->GetCharmInfo()->IsReturning();

    // default, though we shouldn't ever get here
    return false;
}

void PetAI::ClearCharmInfoFlags()
{
    // Quick access to set all flags to FALSE

    CharmInfo* ci = me->GetCharmInfo();

    if (ci)
    {
        ci->SetIsAtStay(false);
        ci->SetIsCommandAttack(false);
        ci->SetIsCommandFollow(false);
        ci->SetIsFollowing(false);
        ci->SetIsReturning(false);
    }
}

void PetAI::AttackedBy(Unit* attacker)
{
    // Called when pet takes damage. This function helps keep pets from running off
    //  simply due to gaining aggro.

    if (!attacker)
        return;

    // Passive pets don't do anything
    if (me->HasReactState(REACT_PASSIVE))
        return;

    // Prevent pet from disengaging from current target
    if (me->getVictim() && me->getVictim()->isAlive())
        return;

    // Continue to evaluate and attack if necessary
    AttackStart(attacker);
}

void PetAI::Minipet_DistanceCheck(uint32 diff)
{
    Unit* owner = me->GetOwner();
    if (!owner)
        return;
    if (distanceCheckTimer <= diff)
    {
        distanceCheckTimer = 2000;
        float masterSpeed = owner->GetSpeed(MOVE_RUN);
        float masterSpeedRate = masterSpeed / baseMoveSpeed[MOVE_RUN];
        float masterDistance = me->GetDistance(owner);
        if(masterDistance >= 20)
        {
            me->SetSpeed(MOVE_RUN, masterSpeedRate * (masterDistance / 15.f));
        } else if (me->GetSpeed(MOVE_RUN) > masterSpeed) {
            me->SetSpeed(MOVE_RUN, masterSpeedRate);
        }
    } else distanceCheckTimer -= diff;
}
