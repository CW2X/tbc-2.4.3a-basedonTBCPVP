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

#ifndef _REALMDATABASE_H
#define _REALMDATABASE_H

#include "MySQLConnection.h"

enum RealmDatabaseStatements : uint32
{
    /*  Naming standard for defines:
        {DB}_{SEL/INS/UPD/DEL/REP}_{Summary of data changed}
        When updating more than one field, consider looking at the calling function
        name for a suiting suffix.
    */

    REALM_SEL_REALMLIST,
    REALM_SEL_REALMLIST_SECURITY_LEVEL,

    REALM_UPD_UPTIME_PLAYERS,
    REALM_SEL_REALM_CHARACTER_COUNTS,
    REALM_DEL_REALM_CHARACTERS_BY_REALM,
    REALM_DEL_REALM_CHARACTERS,
    REALM_INS_REALM_CHARACTERS,
    REALM_SEL_SUM_REALM_CHARACTERS,

    MAX_REALMDATABASE_STATEMENTS
};

class TC_DATABASE_API RealmDatabaseConnection : public MySQLConnection
{
public:
    typedef RealmDatabaseStatements Statements;

    //- Constructors for sync and async connections
    RealmDatabaseConnection(MySQLConnectionInfo& connInfo);
    RealmDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo);
    ~RealmDatabaseConnection();

    //- Loads database type specific prepared statements
    void DoPrepareStatements() override;
};

#endif
