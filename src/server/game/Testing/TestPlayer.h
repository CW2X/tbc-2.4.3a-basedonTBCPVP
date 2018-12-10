#ifndef TEST_PLAYER_H
#define TEST_PLAYER_H

#include "Player.h"

//same as Player but without db saving
class TC_GAME_API TestPlayer : public Player
{
public:
    explicit TestPlayer(WorldSession *session) : Player(session) {}
    virtual ~TestPlayer() {}

    virtual void SaveToDB(bool create = false) override {}
    virtual void SaveInventoryAndGoldToDB(SQLTransaction trans) override {}
    virtual void SaveGoldToDB(SQLTransaction trans) override {}
    virtual void SaveDataFieldToDB() override {}
    virtual void SetMapAtCreation(PlayerInfo const* info) override;

    void Update(uint32 time) override;
};

#endif //TEST_PLAYER_H
