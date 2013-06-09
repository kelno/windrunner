#include "precompiled.h"
#include "../../game/Player.h"
#include "../../game/Map.h"
#include "../../game/MapReference.h"
#include "../../game/Chat.h"
#include "../../game/Language.h"

void ScriptedInstance::CastOnAllPlayers(uint32 spellId)
{
    Map::PlayerList const& players = instance->GetPlayers();

    if (!players.isEmpty())
    {
        for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* plr = itr->getSource())
                plr->CastSpell(plr, spellId, true);                
        }
    }
}

void ScriptedInstance::RemoveAuraOnAllPlayers(uint32 spellId)
{
    Map::PlayerList const& players = instance->GetPlayers();

    if (!players.isEmpty())
    {
        for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            if (Player* plr = itr->getSource())
                plr->RemoveAurasDueToSpell(spellId);
        }
    }
}

void ScriptedInstance::MonsterPulled(Creature* creature, Unit* puller)
{
    
}

void ScriptedInstance::PlayerDied(Player* player)
{
    
}

void ScriptedInstance::SendScriptInTestNoLootMessageToAll()
{
    Map::PlayerList const& players = instance->GetPlayers();

    if (!players.isEmpty()) {
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
            if (Player* plr = itr->getSource())
                ChatHandler(plr).SendSysMessage(LANG_SCRIPT_IN_TEST_NO_LOOT);
        }
    }
}

/**
   Constructor for DialogueHelper

   @param   pDialogueArray The static const array of DialogueEntry holding the information about the dialogue. This array MUST be terminated by {0,0,0}
 */
DialogueHelper::DialogueHelper(DialogueEntry const* pDialogueArray, Creature *origin) :
    m_pInstance(NULL),
    m_pDialogueArray(pDialogueArray),
    m_pCurrentEntry(NULL),
    m_pDialogueTwoSideArray(NULL),
    m_pCurrentEntryTwoSide(NULL),
    m_uiTimer(0),
    m_bIsFirstSide(true),
    m_origin(origin)
{}

/**
   Constructor for DialogueHelper (Two Sides)

   @param   pDialogueTwoSideArray The static const array of DialogueEntryTwoSide holding the information about the dialogue. This array MUST be terminated by {0,0,0,0,0}
 */
DialogueHelper::DialogueHelper(DialogueEntryTwoSide const* pDialogueTwoSideArray, Creature *origin) :
    m_pInstance(NULL),
    m_pDialogueArray(NULL),
    m_pCurrentEntry(NULL),
    m_pDialogueTwoSideArray(pDialogueTwoSideArray),
    m_pCurrentEntryTwoSide(NULL),
    m_uiTimer(0),
    m_bIsFirstSide(true),
    m_origin(origin)
{}

/**
   Function to start a (part of a) dialogue

   @param   iTextEntry The TextEntry of the dialogue that will be started (must be always the entry of first side)
 */
void DialogueHelper::StartNextDialogueText(int32 iTextEntry)
{
    // Find iTextEntry
    bool bFound = false;

    if (m_pDialogueArray)                                   // One Side
    {
        for (DialogueEntry const* pEntry = m_pDialogueArray; pEntry->iTextEntry; ++pEntry)
        {
            if (pEntry->iTextEntry == iTextEntry)
            {
                m_pCurrentEntry = pEntry;
                bFound = true;
                break;
            }
        }
    }
    else                                                    // Two Sides
    {
        for (DialogueEntryTwoSide const* pEntry = m_pDialogueTwoSideArray; pEntry->iTextEntry; ++pEntry)
        {
            if (pEntry->iTextEntry == iTextEntry)
            {
                m_pCurrentEntryTwoSide = pEntry;
                bFound = true;
                break;
            }
        }
    }

    if (!bFound)
    {
        error_log("Script call DialogueHelper::StartNextDialogueText, but textEntry %i is not in provided dialogue (on map id %u)", iTextEntry, m_pInstance ? m_pInstance->instance->GetId() : 0);
        return;
    }

    DoNextDialogueStep();
}

/// Internal helper function to do the actual say of a DialogueEntry
void DialogueHelper::DoNextDialogueStep()
{
    // Last Dialogue Entry done?
    if ((m_pCurrentEntry && !m_pCurrentEntry->iTextEntry) || (m_pCurrentEntryTwoSide && !m_pCurrentEntryTwoSide->iTextEntry))
    {
        m_uiTimer = 0;
        return;
    }

    // Get Text, SpeakerEntry and Timer
    int32 iTextEntry = 0;
    uint32 uiSpeakerEntry = 0;

    if (m_pDialogueArray)                               // One Side
    {
        uiSpeakerEntry = m_pCurrentEntry->uiSayerEntry;
        iTextEntry = m_pCurrentEntry->iTextEntry;

        m_uiTimer = m_pCurrentEntry->uiTimer;
    }
    else                                                // Two Sides
    {
        // Second Entries can be 0, if they are the entry from first side will be taken
        uiSpeakerEntry = !m_bIsFirstSide && m_pCurrentEntryTwoSide->uiSayerEntryAlt ? m_pCurrentEntryTwoSide->uiSayerEntryAlt : m_pCurrentEntryTwoSide->uiSayerEntry;
        iTextEntry = !m_bIsFirstSide && m_pCurrentEntryTwoSide->iTextEntryAlt ? m_pCurrentEntryTwoSide->iTextEntryAlt : m_pCurrentEntryTwoSide->iTextEntry;

        m_uiTimer = m_pCurrentEntryTwoSide->uiTimer;
    }

    // Simulate Case
    if (uiSpeakerEntry && iTextEntry < 0)
    {
        // Use Speaker if directly provided
        Creature* pSpeaker = GetSpeakerByEntry(uiSpeakerEntry);
        if (m_pInstance && !pSpeaker)                       // Get Speaker from instance
        	if (Creature *speaker = SelectCreatureInGrid(m_origin, uiSpeakerEntry, 100.0f))
                pSpeaker = speaker;

        if (pSpeaker)
            DoScriptText(iTextEntry, pSpeaker);
    }

    JustDidDialogueStep(m_pDialogueArray ?  m_pCurrentEntry->iTextEntry : m_pCurrentEntryTwoSide->iTextEntry);

    // Increment position
    if (m_pDialogueArray)
        ++m_pCurrentEntry;
    else
        ++m_pCurrentEntryTwoSide;
}

/// Call this function within any DialogueUpdate method. This is required for saying next steps in a dialogue
void DialogueHelper::DialogueUpdate(uint32 uiDiff)
{
    if (m_uiTimer)
    {
        if (m_uiTimer <= uiDiff)
            DoNextDialogueStep();
        else
            m_uiTimer -= uiDiff;
    }
}
