#include "Common.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Database/DatabaseEnv.h"
#include "ItemEnchantmentMgr.h"
#include "World.h"
#include "DBCStores.h"
#include "SpellMgr.h"
#include "Bag.h"
#include "ScriptMgr.h"
#include "LootItemStorage.h"
#include "TradeData.h"
#include "QueryPackets.h"

void AddItemsSetItem(Player*player,Item *item)
{
    ItemTemplate const *proto = item->GetTemplate();
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if(!set)
    {
        TC_LOG_ERROR("FIXME","Item set %u for item (id %u) not found, mods not applied.",setid,proto->ItemId);
        return;
    }

    if( set->required_skill_id && player->GetSkillValue(set->required_skill_id) < set->required_skill_value )
        return;

    ItemSetEffect *eff = nullptr;

    for(auto & x : player->ItemSetEff)
    {
        if(x && x->setid == setid)
        {
            eff = x;
            break;
        }
    }

    if(!eff)
    {
        eff = new ItemSetEffect;
        memset(eff,0,sizeof(ItemSetEffect));
        eff->setid = setid;

        size_t x = 0;
        for(; x < player->ItemSetEff.size(); x++)
            if(!player->ItemSetEff[x])
                break;

        if(x < player->ItemSetEff.size())
            player->ItemSetEff[x]=eff;
        else
            player->ItemSetEff.push_back(eff);
    }

    ++eff->item_count;

    for(uint32 x=0;x<8;x++)
    {
        if(!set->spells [x])
            continue;
        //not enough for  spell
        if(set->items_to_triggerspell[x] > eff->item_count)
            continue;

        uint32 z=0;
        for(;z<8;z++)
            if(eff->spells[z] && eff->spells[z]->Id==set->spells[x])
                break;

        if(z < 8)
            continue;

        //new spell
        for(auto & spell : eff->spells)
        {
            if(!spell)                             // free slot
            {
                SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(set->spells[x]);
                if(!spellInfo)
                {
                    TC_LOG_ERROR("FIXME","WORLD: unknown spell id %u in items set %u effects", set->spells[x],setid);
                    break;
                }

                // spell casted only if fit form requirement, in other case will casted at form change
                player->ApplyEquipSpell(spellInfo,nullptr,true);
                spell = spellInfo;
                break;
            }
        }
    }
}

void RemoveItemsSetItem(Player*player,ItemTemplate const *proto)
{
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if(!set)
    {
        TC_LOG_ERROR("FIXME","Item set #%u for item #%u not found, mods not removed.",setid,proto->ItemId);
        return;
    }

    ItemSetEffect *eff = nullptr;
    size_t setindex = 0;
    for(;setindex < player->ItemSetEff.size(); setindex++)
    {
        if(player->ItemSetEff[setindex] && player->ItemSetEff[setindex]->setid == setid)
        {
            eff = player->ItemSetEff[setindex];
            break;
        }
    }

    // can be in case now enough skill requirement for set appling but set has been appliend when skill requirement not enough
    if(!eff)
        return;

    --eff->item_count;

    for(uint32 x=0;x<8;x++)
    {
        if(!set->spells[x])
            continue;

        // enough for spell
        if(set->items_to_triggerspell[x] <= eff->item_count)
            continue;

        for(auto & spell : eff->spells)
        {
            if(spell && spell->Id==set->spells[x])
            {
                // spell can be not active if not fit form requirement
                player->ApplyEquipSpell(spell,nullptr,false);
                spell=nullptr;
                break;
            }
        }
    }

    if(!eff->item_count)                                    //all items of a set were removed
    {
        assert(eff == player->ItemSetEff[setindex]);
        delete eff;
        player->ItemSetEff[setindex] = nullptr;
    }
}

bool ItemCanGoIntoBag(ItemTemplate const *pProto, ItemTemplate const *pBagProto)
{
    if(!pProto || !pBagProto)
        return false;

    switch(pBagProto->Class)
    {
        case ITEM_CLASS_CONTAINER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_CONTAINER:
                    return true;
                case ITEM_SUBCLASS_SOUL_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_SOUL_SHARDS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_HERB_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_HERBS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENCHANTING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_ENCHANTING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_MINING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_MINING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENGINEERING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_ENGINEERING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_GEM_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_GEMS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_LEATHERWORKING_CONTAINER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_LEATHERWORKING_SUPP))
                        return false;
                    return true;
                default:
                    return false;
            }
        case ITEM_CLASS_QUIVER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_QUIVER:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_ARROWS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_AMMO_POUCH:
                    if(!(pProto->BagFamily & BAG_FAMILY_MASK_BULLETS))
                        return false;
                    return true;
                default:
                    return false;
            }
    }
    return false;
}

Item::Item()
{
    m_objectType |= TYPEMASK_ITEM;
    m_objectTypeId = TYPEID_ITEM;

#ifdef LICH_KING
    m_updateFlag = UPDATEFLAG_LOWGUID;
#else
    m_updateFlag = (UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID);  // 2.3.2 - 0x18
#endif

    m_valuesCount = ITEM_END;
    m_slot = 0;
    uState = ITEM_NEW;
    uQueuePos = -1;
    m_container = nullptr;
    m_lootGenerated = false;
    mb_in_trade = false;

    m_itemProto = nullptr;
}

Item::~Item()
{
}

bool Item::Create(ObjectGuid::LowType guidlow, uint32 itemid, Player const* owner, ItemTemplate const *itemProto)
{
    if(!itemProto)
        return false;

    Object::_Create( guidlow, 0, HighGuid::Item );

    SetEntry(itemid);
    SetObjectScale(1.0f);
    m_itemProto = itemProto;

    SetGuidValue(ITEM_FIELD_OWNER, owner ? owner->GetGUID() : ObjectGuid::Empty);
    SetGuidValue(ITEM_FIELD_CONTAINED, owner ? owner->GetGUID() : ObjectGuid::Empty);

    SetUInt32Value(ITEM_FIELD_STACK_COUNT, 1);
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_DURABILITY, itemProto->MaxDurability);

    for(int i = 0; i < 5; ++i)
        SetSpellCharges(i,itemProto->Spells[i].SpellCharges);

    SetUInt32Value(ITEM_FIELD_FLAGS, itemProto->Flags);
    SetUInt32Value(ITEM_FIELD_DURATION, abs(itemProto->Duration));

    return true;
}

void Item::UpdateDuration(Player* owner, uint32 diff)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    if (GetUInt32Value(ITEM_FIELD_DURATION)<=diff)
    {
        sScriptMgr->OnItemExpire(owner, GetTemplate());
        owner->DestroyItem(GetBagSlot(), GetSlot(), true);
        return;
    }

    SetUInt32Value(ITEM_FIELD_DURATION, GetUInt32Value(ITEM_FIELD_DURATION) - diff);
    SetState(ITEM_CHANGED);                                 // save new time in database
}

void Item::SaveToDB(SQLTransaction& trans)
{
    ObjectGuid::LowType guid = GetGUID().GetCounter();
    switch (uState)
    {
        case ITEM_NEW:
        case ITEM_CHANGED:
        {
#ifdef TRINITY_DEBUG
            int32 smallIntMax = 99999;
            int32 mediumIntMax = 99999999;

            ASSERT(GetEntry() < mediumIntMax, "too high %u", GetEntry());
            ASSERT(GetCount() < smallIntMax, "too high %u", GetCount());
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                ASSERT(GetInt32Value(ITEM_FIELD_SPELL_CHARGES + i) < smallIntMax/2, "too high %u", GetInt32Value(ITEM_FIELD_SPELL_CHARGES + i));

            for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; i++)
            {
                ASSERT(GetEnchantmentId(EnchantmentSlot(i)) < mediumIntMax, "too high %u", GetEnchantmentId(EnchantmentSlot(i)));
                ASSERT(GetEnchantmentDuration(EnchantmentSlot(i)) < mediumIntMax, "too high %u", GetEnchantmentDuration(EnchantmentSlot(i)));
                ASSERT(GetEnchantmentCharges(EnchantmentSlot(i)) < smallIntMax, "too high %u", GetEnchantmentCharges(EnchantmentSlot(i)));
            }
            ASSERT(GetItemSuffixFactor() < smallIntMax, "too high %u", GetItemSuffixFactor());
            ASSERT(std::abs(GetItemRandomPropertyId()) < smallIntMax, "too high %u", GetItemRandomPropertyId());
            ASSERT(GetUInt32Value(ITEM_FIELD_DURABILITY) < smallIntMax, "too high %u", GetUInt32Value(ITEM_FIELD_DURABILITY));
#endif
            uint8 index = 0; 
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(uState == ITEM_NEW ? CHAR_REP_ITEM_INSTANCE : CHAR_UPD_ITEM_INSTANCE);
            stmt->setUInt32(index++, GetGUID());
            stmt->setUInt32(index++,GetOwnerGUID().GetCounter());
            stmt->setUInt32(index++, GetEntry());
            stmt->setUInt64(index++, GetGuidValue(ITEM_FIELD_CONTAINED));
            stmt->setUInt32(index++, GetUInt32Value(ITEM_FIELD_CREATOR));
            stmt->setUInt32(index++, GetUInt32Value(ITEM_FIELD_GIFTCREATOR));
            stmt->setUInt16(index++, GetCount());
            stmt->setUInt16(index++, GetUInt32Value(ITEM_FIELD_DURATION));
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                stmt->setUInt16(index++, GetInt32Value(ITEM_FIELD_SPELL_CHARGES+i));
            
            stmt->setUInt32(index++, GetUInt32Value(ITEM_FIELD_FLAGS));

            for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; i++)
            {
                stmt->setUInt32(index++, (GetEnchantmentId(EnchantmentSlot(i))));
                stmt->setUInt32(index++, (GetEnchantmentDuration(EnchantmentSlot(i))));
                stmt->setUInt16(index++, (GetEnchantmentCharges(EnchantmentSlot(i))));
            }

            stmt->setUInt16(index++, GetItemSuffixFactor());
            stmt->setInt16(index++, GetItemRandomPropertyId());
            stmt->setUInt16(index++, GetUInt32Value(ITEM_FIELD_DURABILITY));
            stmt->setUInt32(index++, GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID));

            if(uState == ITEM_CHANGED)
                stmt->setUInt32(index++, guid);

            trans->Append(stmt);

            if ((uState == ITEM_CHANGED) && HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_WRAPPED))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GIFT_OWNER);
                stmt->setUInt32(0, GetOwnerGUID().GetCounter());
                stmt->setUInt32(1, guid);
                trans->Append(stmt);
            }
        } break;
        case ITEM_REMOVED:
        {
            if (GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID) > 0 )
                trans->PAppend("DELETE FROM item_text WHERE id = '%u'", GetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID));

            trans->PAppend("DELETE FROM item_instance WHERE guid = '%u'", guid);
            if(HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_WRAPPED))
                trans->PAppend("DELETE FROM character_gifts WHERE item_guid = '%u'", GetGUID().GetCounter());

            // Delete the items if this is a container
            if (!loot.isLooted())
                sLootItemStorage->RemoveStoredLootForContainer(GetGUID().GetCounter());

            delete this;
            return;
        }
        case ITEM_UNCHANGED:
            break;
    }
    SetState(ITEM_UNCHANGED);
}

bool Item::LoadFromDB(ObjectGuid::LowType guid, ObjectGuid owner_guid, Field* fields, uint32 entry)
{
    // create item before any checks for store correct guid
    // and allow use "FSetState(ITEM_REMOVED); SaveToDB();" for deleting item from DB
    Object::_Create(guid, 0, HighGuid::Item);

    /*PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ITEM_INSTANCE);
    stmt->setUInt32(0, guid);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_ERROR("sql.sql","ERROR: Item (GUID: %u owner: %u) not found in table `item_instance`, can't load. ", guid, owner_guid.GetCounter());
        return false;
    }

    Field* fields = result->Fetch();*/

    SetEntry(entry);
    SetObjectScale(1.0f);
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(GetEntry());
    if (!proto)
    {
        TC_LOG_ERROR("sql.sql", "ERROR: Item (GUID: %u owner: %u) has an invalid entry %u `item_instance`, can't load. ", guid, owner_guid.GetCounter(), GetEntry());
        return false;
    }

    // set owner (not if item is only loaded for gbank/auction/mail
    if (owner_guid)
        SetOwnerGUID(owner_guid);

    uint8 index = 0;
    //SetGuidValue(ITEM_FIELD_CONTAINED, ObjectGuid(fields[index++].GetUInt64())); //todo: switch this to uint32
    SetGuidValue(ITEM_FIELD_CREATOR, ObjectGuid(HighGuid::Player, fields[index++].GetUInt32()));
    SetGuidValue(ITEM_FIELD_GIFTCREATOR, ObjectGuid(HighGuid::Player, fields[index++].GetUInt32()));
    SetUInt32Value(ITEM_FIELD_STACK_COUNT, fields[index++].GetUInt16());
    SetUInt32Value(ITEM_FIELD_DURATION, fields[index++].GetUInt32());
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        SetInt32Value(ITEM_FIELD_SPELL_CHARGES + i, fields[index++].GetInt16());

    SetUInt32Value(ITEM_FIELD_FLAGS, fields[index++].GetUInt32());
    for (uint8 i = 0; i < MAX_ENCHANTMENT_SLOT; i++)
    {
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT + i * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, fields[index++].GetUInt32());
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT + i * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, fields[index++].GetUInt32());
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT + i * MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, fields[index++].GetUInt16());
    }

    SetUInt32Value(ITEM_FIELD_PROPERTY_SEED, fields[index++].GetUInt16());
    SetUInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, fields[index++].GetInt16());
    SetUInt32Value(ITEM_FIELD_DURABILITY, fields[index++].GetUInt16());
    SetUInt32Value(ITEM_FIELD_ITEM_TEXT_ID, fields[index++].GetUInt32());
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, proto->MaxDurability);

    bool need_save = false;                                 // need explicit save data at load fixes

    //TC SetText(fields[index++].GetString());

    // overwrite possible wrong/corrupted guid
    ObjectGuid new_item_guid = ObjectGuid(HighGuid::Item, 0, guid);
    if(GetGuidValue(OBJECT_FIELD_GUID) != new_item_guid)
    {
        SetGuidValue(OBJECT_FIELD_GUID, ObjectGuid(HighGuid::Item, 0, guid));
        need_save = true;
    }

    m_itemProto = proto;

    // recalculate suffix factor
    if(GetItemRandomPropertyId() < 0)
    {
        if(UpdateItemSuffixFactor())
            need_save = true;
    }

    // Remove bind flag for items vs NO_BIND set
    if (IsSoulBound() && proto->Bonding == NO_BIND)
    {
        ApplyModFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_SOULBOUND, false);
        need_save = true;
    }

    // update duration if need, and remove if not need
    if((proto->Duration == 0) != (GetUInt32Value(ITEM_FIELD_DURATION) == 0))
    {
        SetUInt32Value(ITEM_FIELD_DURATION,proto->Duration);
        need_save = true;
    }

    // set correct owner
    if(owner_guid != 0 && GetOwnerGUID() != owner_guid)
    {
        SetOwnerGUID(owner_guid);
        need_save = true;
    }

    if(need_save)                                           // normal item changed state set not work at loading
    {
        //"UPDATE item_instance SET guid = ?, owner_guid = ?, duration = ?, flags = ?, durability = ? WHERE guid = ?"
        PreparedStatement* _stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_INSTANCE_ON_LOAD);
        _stmt->setUInt32(0, guid);
        _stmt->setUInt32(1, owner_guid);
        _stmt->setUInt32(2, GetUInt32Value(ITEM_FIELD_DURATION));
        _stmt->setUInt32(3, GetUInt32Value(ITEM_FIELD_FLAGS));
        _stmt->setUInt32(4, GetUInt32Value(ITEM_FIELD_DURABILITY));
        CharacterDatabase.Execute(_stmt);
    }

    return true;
}

/*static*/ void Item::DeleteFromDB(SQLTransaction& trans, ObjectGuid::LowType itemGuid)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
    stmt->setUInt32(0, itemGuid);
    trans->Append(stmt);
}

void Item::DeleteFromDB(SQLTransaction& trans)
{
    DeleteFromDB(trans, GetGUID().GetCounter());

    // Delete the items if this is a container
    if (!loot.isLooted())
        sLootItemStorage->RemoveStoredLootForContainer(GetGUID().GetCounter());
}

/*static*/
void Item::DeleteFromInventoryDB(SQLTransaction& trans, ObjectGuid::LowType itemGuid)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_INVENTORY_BY_ITEM);
    stmt->setUInt32(0, itemGuid);
    trans->Append(stmt);
}

void Item::DeleteFromInventoryDB(SQLTransaction& trans)
{
    DeleteFromInventoryDB(trans, GetGUID().GetCounter());
}

Player* Item::GetOwner()const
{
    return ObjectAccessor::FindPlayer(GetOwnerGUID());
}

uint32 Item::GetSpell()
{
    ItemTemplate const* proto = GetTemplate();

    switch (proto->Class)
    {
        case ITEM_CLASS_WEAPON:
            switch (proto->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:     return  196;
                case ITEM_SUBCLASS_WEAPON_AXE2:    return  197;
                case ITEM_SUBCLASS_WEAPON_BOW:     return  264;
                case ITEM_SUBCLASS_WEAPON_GUN:     return  266;
                case ITEM_SUBCLASS_WEAPON_MACE:    return  198;
                case ITEM_SUBCLASS_WEAPON_MACE2:   return  199;
                case ITEM_SUBCLASS_WEAPON_POLEARM: return  200;
                case ITEM_SUBCLASS_WEAPON_SWORD:   return  201;
                case ITEM_SUBCLASS_WEAPON_SWORD2:  return  202;
                case ITEM_SUBCLASS_WEAPON_STAFF:   return  227;
                case ITEM_SUBCLASS_WEAPON_DAGGER:  return 1180;
                case ITEM_SUBCLASS_WEAPON_THROWN:  return 2567;
                case ITEM_SUBCLASS_WEAPON_SPEAR:   return 3386;
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:return 5011;
                case ITEM_SUBCLASS_WEAPON_WAND:    return 5009;
                default: return 0;
            }
        case ITEM_CLASS_ARMOR:
            switch(proto->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_CLOTH:    return 9078;
                case ITEM_SUBCLASS_ARMOR_LEATHER:  return 9077;
                case ITEM_SUBCLASS_ARMOR_MAIL:     return 8737;
                case ITEM_SUBCLASS_ARMOR_PLATE:    return  750;
                case ITEM_SUBCLASS_ARMOR_SHIELD:   return 9116;
                default: return 0;
            }
    }
    return 0;
}

int32 Item::GenerateItemRandomPropertyId(uint32 item_id)
{
    ItemTemplate const *itemProto = sObjectMgr->GetItemTemplate(item_id);

    if(!itemProto)
        return 0;

    // item must have one from this field values not null if it can have random enchantments
    if((!itemProto->RandomProperty) && (!itemProto->RandomSuffix))
        return 0;

    // item can have not null only one from field values
    if((itemProto->RandomProperty) && (itemProto->RandomSuffix))
    {
        TC_LOG_ERROR("sql.sql","Item template %u have RandomProperty==%u and RandomSuffix==%u, but must have one from field =0",itemProto->ItemId,itemProto->RandomProperty,itemProto->RandomSuffix);
        return 0;
    }

    // RandomProperty case
    if(itemProto->RandomProperty)
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomProperty);
        ItemRandomPropertiesEntry const *random_id = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if(!random_id)
        {
            TC_LOG_ERROR("sql.sql","Enchantment id #%u used but it doesn't have records in 'ItemRandomProperties.dbc'",randomPropId);
            return 0;
        }

        return random_id->ID;
    }
    // RandomSuffix case
    else
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomSuffix);
        ItemRandomSuffixEntry const *random_id = sItemRandomSuffixStore.LookupEntry(randomPropId);
        if(!random_id)
        {
            TC_LOG_ERROR("sql.sql","Enchantment id #%u used but it doesn't have records in sItemRandomSuffixStore.",randomPropId);
            return 0;
        }

        return -int32(random_id->ID);
    }
}

void Item::SetItemRandomProperties(int32 randomPropId)
{
    if(!randomPropId)
        return;

    if(randomPropId > 0)
    {
        ItemRandomPropertiesEntry const *item_rand = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if(item_rand)
        {
            if(GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != int32(item_rand->ID))
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,item_rand->ID);
                SetState(ITEM_CHANGED);
            }
            for(uint32 i = PROP_ENCHANTMENT_SLOT_2; i < PROP_ENCHANTMENT_SLOT_2 + 3; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_2],0,0);
        }
    }
    else
    {
        ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(-randomPropId);
        if(item_rand)
        {
            if( GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != -int32(item_rand->ID) ||
                !GetItemSuffixFactor())
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,-int32(item_rand->ID));
                UpdateItemSuffixFactor();
                SetState(ITEM_CHANGED);
            }

            for(uint32 i = PROP_ENCHANTMENT_SLOT_0; i < PROP_ENCHANTMENT_SLOT_0 + 3; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_0],0,0);
        }
    }
}

bool Item::UpdateItemSuffixFactor()
{
    uint32 suffixFactor = GenerateEnchSuffixFactor(GetEntry());
    if(GetItemSuffixFactor()==suffixFactor)
        return false;
    SetUInt32Value(ITEM_FIELD_PROPERTY_SEED,suffixFactor);
    return true;
}

void Item::SetState(ItemUpdateState state, Player *forplayer)
{
    if (uState == ITEM_NEW && state == ITEM_REMOVED)
    {
        // pretend the item never existed
        RemoveItemFromUpdateQueueOf(forplayer);
        delete this;
        return;
    }

    if (state != ITEM_UNCHANGED)
    {
        // new items must stay in new state until saved
        if (uState != ITEM_NEW) 
            uState = state;

        AddItemToUpdateQueueOf(forplayer);
    }
    else
    {
        // unset in queue
        // the item must be removed from the queue manually
        uQueuePos = -1;
        uState = ITEM_UNCHANGED;
    }
}

void Item::AddItemToUpdateQueueOf(Player *player)
{
    if (IsInUpdateQueue()) return;

    if (!player)
    {
        player = GetOwner();
        if (!player)
        {
            TC_LOG_ERROR("item","Item::AddItemToUpdateQueueOf - GetPlayer didn't find a player matching owner's guid (%u)!",GetOwnerGUID().GetCounter());
            return;
        }
    }

    if (player->GetGUID() != GetOwnerGUID())
    {
        TC_LOG_ERROR("item","Item::AddItemToUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!",GetOwnerGUID().GetCounter(), player->GetGUID().GetCounter());
        return;
    }

    if (player->m_itemUpdateQueueBlocked) return;

    player->m_itemUpdateQueue.push_back(this);
    uQueuePos = player->m_itemUpdateQueue.size()-1;
}

void Item::RemoveItemFromUpdateQueueOf(Player *player)
{
    if (!IsInUpdateQueue()) return;

    if (!player)
    {
        player = GetOwner();
        if (!player)
        {
            TC_LOG_ERROR("item","Item::RemoveItemFromUpdateQueueOf - GetPlayer didn't find a player matching owner's guid (%u)!",GetOwnerGUID().GetCounter());
            return;
        }
    }

    if (player->GetGUID() != GetOwnerGUID())
    {
        TC_LOG_ERROR("item","Item::RemoveItemFromUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!",GetOwnerGUID().GetCounter(), player->GetGUID().GetCounter());
        return;
    }

    if (player->m_itemUpdateQueueBlocked) return;

    player->m_itemUpdateQueue[uQueuePos] = nullptr;
    uQueuePos = -1;
}

uint8 Item::GetBagSlot() const
{
    return m_container ? m_container->GetSlot() : uint8(INVENTORY_SLOT_BAG_0);
}

bool Item::IsEquipped() const
{
    return !IsInBag() && m_slot < EQUIPMENT_SLOT_END;
}

bool Item::CanBeTraded(bool /*mail*/, bool /*trade*/) const
{
    if (m_lootGenerated)
        return false;
#ifdef LICH_KING
    if ((!mail || !IsBoundAccountWide()) && (IsSoulBound() && (!HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE) || !trade)))
        return false;
#else
    if(IsSoulBound())
        return false;
#endif
    if (IsBag() && (Player::IsBagPos(GetPos()) || !ToBag()->IsEmpty()))
        return false;

    if(Player* owner = GetOwner())
    {
        if (owner->CanUnequipItem(GetPos(), false) != EQUIP_ERR_OK)
            return false;
        if (owner->GetLootGUID() == GetGUID())
            return false;
    }

    if (IsBoundByEnchant())
        return false;

    return true;
}

bool Item::IsBoundByEnchant() const
{
    // Check all enchants for soulbound
    for(uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
            continue;

        if(enchantEntry->slot & ENCHANTMENT_CAN_SOULBOUND)
            return true;
    }
    return false;
}

bool Item::IsFitToSpellRequirements(SpellInfo const* spellInfo) const
{
    ItemTemplate const* proto = GetTemplate();

    if (spellInfo->EquippedItemClass != -1)                 // -1 == any item class
    {
        if(spellInfo->EquippedItemClass != int32(proto->Class))
            return false;                                   //  wrong item class

        if(spellInfo->EquippedItemSubClassMask != 0)        // 0 == any subclass
        {
            if((spellInfo->EquippedItemSubClassMask & (1 << proto->SubClass)) == 0)
                return false;                               // subclass not present in mask
        }
    }

    if(spellInfo->EquippedItemInventoryTypeMask != 0)       // 0 == any inventory type
    {
        if((spellInfo->EquippedItemInventoryTypeMask  & (1 << proto->InventoryType)) == 0)
            return false;                                   // inventory type not present in mask
    }

    return true;
}

void Item::SetEnchantment(EnchantmentSlot slot, uint32 id, uint32 duration, uint32 charges)
{
    // Better lost small time at check in comparison lost time at item save to DB.
    if((GetEnchantmentId(slot) == id) && (GetEnchantmentDuration(slot) == duration) && (GetEnchantmentCharges(slot) == charges))
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET,id);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED);
}

void Item::SetEnchantmentDuration(EnchantmentSlot slot, uint32 duration)
{
    if(GetEnchantmentDuration(slot) == duration)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetState(ITEM_CHANGED);
}

void Item::SetEnchantmentCharges(EnchantmentSlot slot, uint32 charges)
{
    if(GetEnchantmentCharges(slot) == charges)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED);
}

void Item::ClearEnchantment(EnchantmentSlot slot)
{
    if(!GetEnchantmentId(slot))
        return;

    for(uint8 x = 0; x < 3; ++x)
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + x, 0);
    SetState(ITEM_CHANGED);
}

bool Item::GemsFitSockets() const
{
    bool fits = true;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
    {
        uint8 SocketColor = GetTemplate()->Socket[enchant_slot-SOCK_ENCHANTMENT_SLOT].Color;

        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
        {
            if(SocketColor) fits &= false;
            continue;
        }

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
        {
            if(SocketColor) fits &= false;
            continue;
        }

        uint8 GemColor = 0;

        uint32 gemid = enchantEntry->GemID;
        if(gemid)
        {
            ItemTemplate const* gemProto = sObjectMgr->GetItemTemplate(gemid);
            if(gemProto)
            {
                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if(gemProperty)
                    GemColor = gemProperty->color;
            }
        }

        fits &= (GemColor & SocketColor) ? true : false;
    }
    return fits;
}

uint8 Item::GetGemCountWithID(uint32 GemID) const
{
    uint8 count = 0;
    for(uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+3; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if(!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if(!enchantEntry)
            continue;

        if(GemID == enchantEntry->GemID)
            ++count;
    }
    return count;
}

bool Item::IsLimitedToAnotherMapOrZone( uint32 cur_mapId, uint32 cur_zoneId) const
{
    ItemTemplate const* proto = GetTemplate();
    return proto && ( (proto->Map && proto->Map != cur_mapId) || (proto->Area && proto->Area != cur_zoneId) );
}

// Though the client has the information in the item's data field,
// we have to send SMSG_ITEM_TIME_UPDATE to display the remaining
// time.
void Item::SendTimeUpdate(Player* owner)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    WorldPacket data(SMSG_ITEM_TIME_UPDATE, (8+4));
    data << (uint64)GetGUID();
    data << (uint32)GetUInt32Value(ITEM_FIELD_DURATION);
    owner->SendDirectMessage(&data);
}

Item* Item::CreateItem(uint32 item, uint32 count, Player const* player)
{
    if (count < 1)
        return nullptr;                                        //don't create item at zero count

    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(item);

    if (pProto)
    {
        if (count > pProto->Stackable)
            count = pProto->Stackable;

        assert(count !=0 && "pProto->Stackable==0 but checked at loading already");

        Item *pItem = NewItemOrBag(pProto);
        if (pItem->Create(sObjectMgr->GetGenerator<HighGuid::Item>().Generate(), item, player, pProto))
        {
            pItem->SetCount(count);
            return pItem;
        }
        else
            delete pItem;
    }
    return nullptr;
}

Item* Item::CloneItem( uint32 count, Player const* player ) const
{
    Item* newItem = CreateItem(GetEntry(), count, player);
    if(!newItem)
        return nullptr;

    newItem->SetUInt32Value( ITEM_FIELD_CREATOR,      GetUInt32Value( ITEM_FIELD_CREATOR ) );
    newItem->SetUInt32Value( ITEM_FIELD_GIFTCREATOR,  GetUInt32Value( ITEM_FIELD_GIFTCREATOR ) );
    newItem->SetUInt32Value( ITEM_FIELD_FLAGS,        GetUInt32Value( ITEM_FIELD_FLAGS ) );
    newItem->SetUInt32Value( ITEM_FIELD_DURATION,     GetUInt32Value( ITEM_FIELD_DURATION ) );
    newItem->SetItemRandomProperties(GetItemRandomPropertyId());
    return newItem;
}

void Item::BuildUpdate(UpdateDataMapType& data_map, UpdatePlayerSet&)
{
    if (Player* owner = GetOwner())
        BuildFieldsUpdate(owner, data_map);

    ClearUpdateMask(false);
}

void Item::AddToObjectUpdate()
{
    if (Player* owner = GetOwner())
        owner->GetMap()->AddUpdateObject(this);
}

void Item::RemoveFromObjectUpdate()
{
    if (Player* owner = GetOwner())
        owner->GetMap()->RemoveUpdateObject(this);
}

ObjectGuid Item::GetOwnerGUID() const 
{ 
    return GetGuidValue(ITEM_FIELD_OWNER); 
}

void Item::SetOwnerGUID(ObjectGuid const& guid) 
{ 
    SetGuidValue(ITEM_FIELD_OWNER, guid); 
}

// Just a "legacy shortcut" for proto->GetSkill()
uint32 Item::GetSkill()
{
    ItemTemplate const* proto = GetTemplate();
    return proto->GetSkill();
}

void Item::SetBinding(bool val) 
{ 
    ApplyModFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_SOULBOUND, val);
}

bool Item::IsSoulBound() const 
{ 
    return HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_SOULBOUND);
}

bool Item::IsBindedNotWith(Player const* player) const
{
    // not binded item
    if (!IsSoulBound())
        return false;

    // own item
    if (GetOwnerGUID() == player->GetGUID())
        return false;

#ifdef LICH_KING
    if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE))
        if (allowedGUIDs.find(player->GetGUID()) != allowedGUIDs.end())
            return false;

    // BOA item case
    if (IsBoundAccountWide())
        return false;
#endif

    return true;
}

bool Item::IsBag() const 
{ 
    return GetTemplate()->InventoryType == INVTYPE_BAG; 
}

// Returns true if Item is a bag AND it is not empty.
// Returns false if Item is not a bag OR it is an empty bag.
bool Item::IsNotEmptyBag() const
{
    if (Bag const* bag = ToBag())
        return !bag->IsEmpty();
    return false;
}

bool Item::IsBroken() const 
{ 
    return GetUInt32Value(ITEM_FIELD_MAXDURABILITY) > 0 && GetUInt32Value(ITEM_FIELD_DURABILITY) == 0; 
}

uint32 Item::GetCount() const 
{ 
    return GetUInt32Value(ITEM_FIELD_STACK_COUNT); 
}

void Item::SetCount(uint32 value) 
{ 
    SetUInt32Value(ITEM_FIELD_STACK_COUNT, value); 
}

int32 Item::GetItemRandomPropertyId() const { return GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID); }
uint32 Item::GetItemSuffixFactor() const { return GetUInt32Value(ITEM_FIELD_PROPERTY_SEED); }
uint32 Item::GetEnchantmentId(EnchantmentSlot slot)       const { return GetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET); }
uint32 Item::GetEnchantmentDuration(EnchantmentSlot slot) const { return GetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET); }
uint32 Item::GetEnchantmentCharges(EnchantmentSlot slot)  const { return GetUInt32Value(ITEM_FIELD_ENCHANTMENT + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET); }
int32 Item::GetSpellCharges(uint8 index) const { return GetInt32Value(ITEM_FIELD_SPELL_CHARGES + index); }
void  Item::SetSpellCharges(uint8 index, int32 value) { SetInt32Value(ITEM_FIELD_SPELL_CHARGES + index, value); }