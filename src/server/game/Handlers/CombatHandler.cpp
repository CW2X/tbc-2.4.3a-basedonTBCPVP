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

#include "Common.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "ObjectDefines.h"

void WorldSession::HandleAttackSwingOpcode( WorldPacket & recvData )
{
    ObjectGuid guid;
    recvData >> guid;

    Unit* enemy = ObjectAccessor::GetUnit(*_player, guid);

    if (!enemy) {
        // stop attack state at client
        SendMeleeAttackStop(nullptr);
        return;
    }

    if (!_player->IsValidAttackTarget(enemy))
    {
        // stop attack state at client
        SendMeleeAttackStop(enemy);
        return;
    }

#ifdef LICH_KING

    //! Client explicitly checks the following before sending CMSG_ATTACKSWING packet,
    //! so we'll place the same check here. Note that it might be possible to reuse this snippet
    //! in other places as well.
    if (Vehicle* vehicle = _player->GetVehicle())
    {
        VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(_player);
        ASSERT(seat);
        if (!(seat->m_flags & VEHICLE_SEAT_FLAG_CAN_ATTACK))
        {
            SendAttackStop(pEnemy);
            return;
        }
    }
#endif

    _player->Attack(enemy, true);
}

void WorldSession::HandleAttackStopOpcode( WorldPacket & /*recvData*/ )
{
    GetPlayer()->AttackStop();
}

void WorldSession::HandleSetSheathedOpcode( WorldPacket & recvData )
{
    uint32 sheathed;
    recvData >> sheathed;

    if (sheathed >= MAX_SHEATH_STATE)
    {
        TC_LOG_ERROR("network", "Unknown sheath state %u ?", sheathed);
        return;
    }

    //TC_LOG_DEBUG("network.opcode", "WORLD: Recvd CMSG_SETSHEATHED Message guidlow:%u value1:%u", GetPlayer()->GetGUID().GetCounter(), sheathed );

    GetPlayer()->SetSheath(SheathState(sheathed));
}

void WorldSession::SendMeleeAttackStop(Unit const* enemy)
{
    WorldPacket data( SMSG_ATTACKSTOP, (4+20) );            // we guess size
    data << GetPlayer()->GetPackGUID();
    if (enemy)
        data << enemy->GetPackGUID();
    else
        data << uint8(0);

    data << uint32(0);                                      // unk, can be 1 also
    SendPacket(&data);
}

