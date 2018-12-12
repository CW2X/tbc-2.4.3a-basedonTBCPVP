/*
 * Copyright (C) 2018 Konno Productions Project
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RealmDatabase.h"
#include "PreparedStatement.h"

void RealmDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_REALMDATABASE_STATEMENTS);

    PrepareStatement(REALM_SEL_REALMLIST, "SELECT id, name, address, localAddress, localSubnetMask, port, icon, flag, timezone, allowedSecurityLevel, population, gamebuild FROM realmlist WHERE flag <> 3 ORDER BY name", CONNECTION_SYNCH);
    PrepareStatement(REALM_SEL_REALMLIST_SECURITY_LEVEL, "SELECT allowedSecurityLevel from realmlist WHERE id = ?", CONNECTION_SYNCH);
    PrepareStatement(REALM_UPD_UPTIME_PLAYERS, "UPDATE uptime SET uptime = ?, maxplayers = ? WHERE realmid = ? AND starttime = ?", CONNECTION_ASYNC);
    PrepareStatement(REALM_SEL_REALM_CHARACTER_COUNTS, "SELECT realmid, numchars FROM realmcharacters WHERE  acctid = ?", CONNECTION_ASYNC);
    PrepareStatement(REALM_DEL_REALM_CHARACTERS_BY_REALM, "DELETE FROM realmcharacters WHERE acctid = ? AND realmid = ?", CONNECTION_ASYNC);
    PrepareStatement(REALM_DEL_REALM_CHARACTERS, "DELETE FROM realmcharacters WHERE acctid = ?", CONNECTION_ASYNC);
    PrepareStatement(REALM_INS_REALM_CHARACTERS, "INSERT INTO realmcharacters (numchars, acctid, realmid) VALUES (?, ?, ?)", CONNECTION_ASYNC);
    PrepareStatement(REALM_SEL_SUM_REALM_CHARACTERS, "SELECT SUM(numchars) FROM realmcharacters WHERE acctid = ?", CONNECTION_ASYNC);
}

RealmDatabaseConnection::RealmDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo)
{
}

RealmDatabaseConnection::RealmDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo) : MySQLConnection(q, connInfo)
{
}

RealmDatabaseConnection::~RealmDatabaseConnection()
{
}
