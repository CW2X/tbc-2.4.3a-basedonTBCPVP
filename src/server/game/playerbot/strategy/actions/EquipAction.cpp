//
#include "../../playerbot.h"
#include "EquipAction.h"

#include "../values/ItemCountValue.h"

using namespace ai;

bool EquipAction::Execute(Event event)
{
    std::string text = event.getParam();
    if (text == "?")
    {
        TellEquipmentSets();
        return true;
    }

    if (UseEquipmentSet(text))
        return true;

    ItemIds ids = chat->parseItems(text);

    for (ItemIds::iterator i =ids.begin(); i != ids.end(); i++)
    {
        FindItemByIdVisitor visitor(*i);
        EquipItem(&visitor);
    }

    return true;
}

bool EquipAction::UseEquipmentSet(string& _name)
{
#ifdef LICH_KING
    EquipmentSets &sets = bot->GetEquipmentSets();
    for (EquipmentSets::iterator i = sets.begin(); i != sets.end(); i++)
    {
        if (i->second.state == EQUIPMENT_SET_DELETED || i->second.Name != _name)
            continue;

        UseEquipmentSet(i->second);

        std::ostringstream out; out << _name << " set equipped";
        ai->TellMaster(out);
        return true;
    }
#endif
    return false;
}

#ifdef LICH_KING
bool EquipAction::UseEquipmentSet(EquipmentSet& set)
{
    WorldPacket* p = new WorldPacket(CMSG_EQUIPMENT_SET_USE);
    uint8 srcbag = 0;
    for(uint8 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        ObjectGuid guid;
        uint32 itemId = set.Items[slot];
        if (set.IgnoreMask & (1 << slot))
            p->appendPackGUID((uint64(1)));
        else
            p->appendPackGUID(itemId);
        *p << srcbag << slot;
    }
    bot->GetSession()->QueuePacket(p);
    return true;
}
#endif

void EquipAction::TellEquipmentSets()
{
#ifdef LICH_KING
    ai->TellMaster("=== Equipment sets ===");
    EquipmentSets &sets = bot->GetEquipmentSets();
    for (EquipmentSets::iterator i = sets.begin(); i != sets.end(); i++)
    {
        if (i->second.state != EQUIPMENT_SET_DELETED)
            ai->TellMaster(i->second.Name);
    }
#else
    ai->TellMaster("Not supported");
#endif
}

void EquipAction::EquipItem(FindItemVisitor* visitor)
{
    IterateItems(visitor);
    list<Item*> items = visitor->GetResult();
    if (!items.empty()) EquipItem(**items.begin());
}


void EquipAction::EquipItem(Item& item)
{
    uint8 bagIndex = item.GetBagSlot();
    uint8 slot = item.GetSlot();
    uint32 itemId = item.GetTemplate()->ItemId;

    if (item.GetTemplate()->InventoryType == INVTYPE_AMMO)
    {
        bot->SetAmmo(itemId);
    }
    else
    {
        WorldPacket* const packet = new WorldPacket(CMSG_AUTOEQUIP_ITEM, 2);
            *packet << bagIndex << slot;
        bot->GetSession()->QueuePacket(packet);
    }

    std::ostringstream out; out << "equipping " << chat->formatItem(item.GetTemplate());
    ai->TellMaster(out);
}
