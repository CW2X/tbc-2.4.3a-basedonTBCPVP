
#include "playerbot.h"
#include "AiFactory.h"
#include "strategy/Engine.h"

#include "strategy/priest/PriestAiObjectContext.h"
#include "strategy/mage/MageAiObjectContext.h"
#include "strategy/warlock/WarlockAiObjectContext.h"
#include "strategy/warrior/WarriorAiObjectContext.h"
#include "strategy/shaman/ShamanAiObjectContext.h"
#include "strategy/paladin/PaladinAiObjectContext.h"
#include "strategy/druid/DruidAiObjectContext.h"
#include "strategy/hunter/HunterAiObjectContext.h"
#include "strategy/rogue/RogueAiObjectContext.h"
#include "Player.h"
#include "PlayerbotAIConfig.h"
#include "RandomPlayerbotMgr.h"


AiObjectContext* AiFactory::createAiObjectContext(Player* player, PlayerbotAI* ai)
{
    switch (player->GetClass())
    {
    case CLASS_PRIEST:
        return new PriestAiObjectContext(ai);
        break;
    case CLASS_MAGE:
        return new MageAiObjectContext(ai);
        break;
    case CLASS_WARLOCK:
        return new WarlockAiObjectContext(ai);
        break;
    case CLASS_WARRIOR:
        return new WarriorAiObjectContext(ai);
        break;
    case CLASS_SHAMAN:
        return new ShamanAiObjectContext(ai);
        break;
    case CLASS_PALADIN:
        return new PaladinAiObjectContext(ai);
        break;
    case CLASS_DRUID:
        return new DruidAiObjectContext(ai);
        break;
    case CLASS_HUNTER:
        return new HunterAiObjectContext(ai);
        break;
    case CLASS_ROGUE:
        return new RogueAiObjectContext(ai);
        break;
    }
    return new AiObjectContext(ai);
}

void AiFactory::CountTalentsPerTab(Player* player, int& tab1, int& tab2, int& tab3)
{
    tab1 = 0;
    tab2 = 0;
    tab3 = 0;
    
#ifdef LICH_KING
    PlayerTalentMap& talentMap = player->GetTalentMap(0);
    for (PlayerTalentMap::iterator i = talentMap.begin(); i != talentMap.end(); ++i)
    {
        uint32 spellId = i->first;
        TalentSpellPos const* talentPos = GetTalentSpellPos(spellId);
        if (!talentPos)
            continue;

        TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentPos->talent_id);
        if (!talentInfo)
            continue;

        uint32 const* talentTabIds = GetTalentTabPages(player->GetClass());
        if (talentInfo->TalentTab == talentTabIds[0]) tab1++;
        if (talentInfo->TalentTab == talentTabIds[1]) tab2++;
        if (talentInfo->TalentTab == talentTabIds[2]) tab3++;
    }
#else
    //We don't have any talents saved on BC so, another way to do this:
    uint32 classMask = player->GetClassMask();
    map<uint32, vector<TalentEntry const*> > spells;
    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const *talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
            continue;

        TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
        if (!talentTabInfo)
            continue;

        if ((classMask & talentTabInfo->ClassMask) == 0)
            continue;

        spells[talentTabInfo->tabpage].push_back(talentInfo);
    }
    auto CountTalents = [&](int tab) {
        uint32 count = 0;
        for (auto talent : spells[tab])
        {
            for (int rank = 0; rank < MAX_TALENT_RANK; ++rank)
            {
                uint32 spellId = talent->RankID[rank];
                if (!spellId)
                    continue;

                if (player->HasSpell(spellId))
                    count++;
            }
        }
        return count;
    };

    tab1 = CountTalents(0);
    tab2 = CountTalents(1);
    tab3 = CountTalents(2);
#endif
}

int AiFactory::GetPlayerSpecTab(Player* player)
{
    int c0 = 0, c1 = 0, c2 = 0;
    AiFactory::CountTalentsPerTab(player, c0, c1, c2);

    if (c0 >= c1 && c0 >= c2)
        return 0;

    if (c1 >= c0 && c1 >= c2)
        return 1;

    return 2;
}

void AiFactory::AddDefaultCombatStrategies(Player* player, PlayerbotAI* const facade, Engine* engine)
{
    int tab = GetPlayerSpecTab(player);

    engine->addStrategies({ "racials", "chat", "default", "aoe", "potions", "cast time", "conserve mana", "duel", "pvp" });

    switch (player->GetClass())
    {
        case CLASS_PRIEST:
            if (tab == 2)
            {
                engine->addStrategies({ "dps", "threat" });
                if (player->GetLevel() > 19)
                    engine->addStrategy("dps debuff");
            }
            else
                engine->addStrategy("heal");

            engine->addStrategies({ "dps assist", "flee" });
            break;
        case CLASS_MAGE:
            if (tab == 0)
                engine->addStrategies({ "arcane", "threat" });
            else if (tab == 1)
                engine->addStrategies({ "fire", "fire aoe", "threat" });
            else
                engine->addStrategies({ "frost", "frost aoe", "threat" });

            engine->addStrategies({ "dps assist", "flee" });
            break;
        case CLASS_WARRIOR:
            if (tab == 2)
                engine->addStrategies({ "tank", "tank aoe" });
            else
                engine->addStrategies({"dps", "dps assist", "threat"});
            break;
        case CLASS_SHAMAN:
            if (tab == 0)
                engine->addStrategies({ "caster", "caster aoe", "bmana", "threat", "flee" });
            else if (tab == 2)
                engine->addStrategies({ "heal", "bmana", "flee" });
            else
                engine->addStrategies({ "dps", "melee aoe", "bdps", "threat" });

            engine->addStrategy("dps assist");
            break;
        case CLASS_PALADIN:
            if (tab == 1)
                engine->addStrategies({ "tank", "tank aoe", "barmor" });
            else
                engine->addStrategies({ "dps", "bdps", "threat", "dps assist" });
            break;
        case CLASS_DRUID:
            if (tab == 0)
            {
                engine->addStrategies({ "caster", "caster aoe", "threat", "flee", "dps assist" });
                if (player->GetLevel() > 19)
                    engine->addStrategy("caster debuff");
            }
            else if (tab == 2)
                engine->addStrategies({ "heal", "flee", "dps assist" });
            else
                engine->addStrategies({ "bear", "tank aoe", "threat", "flee" });
            break;
        case CLASS_HUNTER:
            engine->addStrategies({ "dps", "bdps", "threat", "dps assist" });
            if (player->GetLevel() > 19)
                engine->addStrategy("dps debuff");
            break;
        case CLASS_ROGUE:
            engine->addStrategies({ "dps", "threat", "dps assist" });
            break;
        case CLASS_WARLOCK:
            if (tab == 1)
                engine->addStrategies({"tank", "threat"});
            else
                engine->addStrategies({ "dps", "threat" });

            if (player->GetLevel() > 19)
                engine->addStrategy("dps debuff");

            engine->addStrategies({ "flee", "dps assist" });
            break;
    }

    if (sRandomPlayerbotMgr.IsRandomBot(player) && !player->GetGroup())
    {
        engine->ChangeStrategy(sPlayerbotAIConfig.randomBotCombatStrategies);
        if (player->GetClass() == CLASS_DRUID && player->GetLevel() < 20)
            engine->addStrategies({ "bear", "threat" });
    }
}

Engine* AiFactory::createCombatEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* AiObjectContext) {
    Engine* engine = new Engine(facade, AiObjectContext);
    AddDefaultCombatStrategies(player, facade, engine);
    return engine;
}

void AiFactory::AddDefaultNonCombatStrategies(Player* player, PlayerbotAI* const facade, Engine* nonCombatEngine)
{
    int tab = GetPlayerSpecTab(player);

    switch (player->GetClass()){
        case CLASS_PALADIN:
            if (tab == 1)
                nonCombatEngine->addStrategies({ "bthreat", "tank aoe" });
            else
                nonCombatEngine->addStrategies({ "bdps", "dps assist" });
            break;
        case CLASS_HUNTER:
            nonCombatEngine->addStrategies({ "bdps", "dps assist" });
            break;
        case CLASS_SHAMAN:
            nonCombatEngine->addStrategy("bmana");
            break;
        case CLASS_MAGE:
            if (tab == 1)
                nonCombatEngine->addStrategy("bdps");
            else
                nonCombatEngine->addStrategy("bmana");

            nonCombatEngine->addStrategy("dps assist");
            break;
        case CLASS_DRUID:
            if (tab == 1)
                nonCombatEngine->addStrategy("tank aoe");
            else
                nonCombatEngine->addStrategy("dps assist");
            break;
        case CLASS_WARRIOR:
            if (tab == 2)
                nonCombatEngine->addStrategy("tank aoe");
            else
                nonCombatEngine->addStrategy("dps assist");
            break;
        default:
            nonCombatEngine->addStrategy("dps assist");
            break;
    }
    nonCombatEngine->addStrategies({ "nc", "food", "stay", "chat",
            "default", "quest", "loot", "gather", "duel", "emote", "lfg" });

    if (sRandomPlayerbotMgr.IsRandomBot(player) && !player->GetGroup())
    {
        nonCombatEngine->ChangeStrategy(sPlayerbotAIConfig.randomBotNonCombatStrategies);
    }

}

Engine* AiFactory::createNonCombatEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* AiObjectContext) {
    Engine* nonCombatEngine = new Engine(facade, AiObjectContext);

    AddDefaultNonCombatStrategies(player, facade, nonCombatEngine);
    return nonCombatEngine;
}

void AiFactory::AddDefaultDeadStrategies(Player* player, PlayerbotAI* const facade, Engine* deadEngine)
{
    deadEngine->addStrategies({ "dead", "stay", "chat", "default", "follow" });
    if (sRandomPlayerbotMgr.IsRandomBot(player) && !player->GetGroup())
    {
        deadEngine->removeStrategy("follow");
    }
}

Engine* AiFactory::createDeadEngine(Player* player, PlayerbotAI* const facade, AiObjectContext* AiObjectContext) {
    Engine* deadEngine = new Engine(facade, AiObjectContext);
    AddDefaultDeadStrategies(player, facade, deadEngine);
    return deadEngine;
}
