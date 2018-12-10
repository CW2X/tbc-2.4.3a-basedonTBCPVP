
/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `account_ip`
--

DROP TABLE IF EXISTS `account_ip`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `account_ip` (
  `id` int(10) unsigned NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `ip` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) unsigned NOT NULL,
  PRIMARY KEY (`id`,`time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `arena_match`
--

DROP TABLE IF EXISTS `arena_match`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `arena_match` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `team1` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `team1_name` varchar(255) NOT NULL DEFAULT '',
  `team1_member1` int(10) unsigned DEFAULT NULL,
  `team1_member1_ip` varchar(30) DEFAULT NULL,
  `team1_member1_heal` mediumint(8) unsigned DEFAULT NULL,
  `team1_member1_damage` mediumint(8) unsigned DEFAULT NULL,
  `team1_member1_kills` tinyint(3) unsigned DEFAULT NULL,
  `team1_member2` int(10) unsigned DEFAULT NULL,
  `team1_member2_ip` varchar(30) DEFAULT NULL,
  `team1_member2_heal` mediumint(8) unsigned DEFAULT NULL,
  `team1_member2_damage` mediumint(8) unsigned DEFAULT NULL,
  `team1_member2_kills` tinyint(3) unsigned DEFAULT NULL,
  `team1_member3` int(10) unsigned DEFAULT NULL,
  `team1_member3_ip` varchar(30) DEFAULT NULL,
  `team1_member3_heal` mediumint(8) unsigned DEFAULT NULL,
  `team1_member3_damage` mediumint(8) unsigned DEFAULT NULL,
  `team1_member3_kills` tinyint(3) unsigned DEFAULT NULL,
  `team1_member4` int(10) unsigned DEFAULT NULL,
  `team1_member4_ip` varchar(30) DEFAULT NULL,
  `team1_member4_heal` mediumint(8) unsigned DEFAULT NULL,
  `team1_member4_damage` mediumint(8) unsigned DEFAULT NULL,
  `team1_member4_kills` tinyint(3) unsigned DEFAULT NULL,
  `team1_member5` int(10) unsigned DEFAULT NULL,
  `team1_member5_ip` varchar(30) DEFAULT NULL,
  `team1_member5_heal` mediumint(8) unsigned DEFAULT NULL,
  `team1_member5_damage` mediumint(8) unsigned DEFAULT NULL,
  `team1_member5_kills` tinyint(3) unsigned DEFAULT NULL,
  `team2` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `team2_name` varchar(255) NOT NULL DEFAULT '',
  `team2_member1` int(10) unsigned DEFAULT NULL,
  `team2_member1_ip` varchar(30) DEFAULT NULL,
  `team2_member1_heal` mediumint(8) unsigned DEFAULT NULL,
  `team2_member1_damage` mediumint(8) unsigned DEFAULT NULL,
  `team2_member1_kills` tinyint(3) unsigned DEFAULT NULL,
  `team2_member2` int(10) unsigned DEFAULT NULL,
  `team2_member2_ip` varchar(30) DEFAULT NULL,
  `team2_member2_heal` mediumint(8) unsigned DEFAULT NULL,
  `team2_member2_damage` mediumint(8) unsigned DEFAULT NULL,
  `team2_member2_kills` tinyint(3) unsigned DEFAULT NULL,
  `team2_member3` int(10) unsigned DEFAULT NULL,
  `team2_member3_ip` varchar(30) DEFAULT NULL,
  `team2_member3_heal` mediumint(8) unsigned DEFAULT NULL,
  `team2_member3_damage` mediumint(8) unsigned DEFAULT NULL,
  `team2_member3_kills` tinyint(3) unsigned DEFAULT NULL,
  `team2_member4` int(10) unsigned DEFAULT NULL,
  `team2_member4_ip` varchar(30) DEFAULT NULL,
  `team2_member4_heal` mediumint(8) unsigned DEFAULT NULL,
  `team2_member4_damage` mediumint(8) unsigned DEFAULT NULL,
  `team2_member4_kills` tinyint(3) unsigned DEFAULT NULL,
  `team2_member5` int(10) unsigned DEFAULT NULL,
  `team2_member5_ip` varchar(30) DEFAULT NULL,
  `team2_member5_heal` mediumint(8) unsigned DEFAULT NULL,
  `team2_member5_damage` mediumint(8) unsigned DEFAULT NULL,
  `team2_member5_kills` tinyint(3) unsigned DEFAULT NULL,
  `start_time` bigint(20) NOT NULL DEFAULT '0',
  `end_time` bigint(20) NOT NULL DEFAULT '0',
  `winner` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `rating_change` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `winner_rating` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `loser_rating` mediumint(8) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `arena_season_stats`
--

DROP TABLE IF EXISTS `arena_season_stats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `arena_season_stats` (
  `teamid` int(10) unsigned NOT NULL DEFAULT '0',
  `time1` int(10) unsigned DEFAULT '0' COMMENT 'Time spent first in minutes',
  `time2` int(10) unsigned DEFAULT '0' COMMENT 'Time spent second in minutes',
  `time3` int(10) unsigned DEFAULT '0' COMMENT 'Time spent third in minutes',
  PRIMARY KEY (`teamid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `arena_team_event`
--

DROP TABLE IF EXISTS `arena_team_event`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `arena_team_event` (
  `entry` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `id` int(10) unsigned NOT NULL,
  `event` tinyint(3) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `player` int(10) unsigned NOT NULL,
  `ip` varchar(16) NOT NULL DEFAULT '0.0.0.0',
  `time` int(11) unsigned NOT NULL,
  PRIMARY KEY (`entry`),
  KEY `idx_arena_team_event_id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `bg_stats`
--

DROP TABLE IF EXISTS `bg_stats`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bg_stats` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `mapid` mediumint(8) unsigned NOT NULL,
  `start_time` int(11) unsigned NOT NULL COMMENT 'Start timestamp',
  `end_time` int(11) unsigned NOT NULL COMMENT 'End timestamp',
  `winner` enum('alliance','horde','none') NOT NULL,
  `score_alliance` mediumint(9) NOT NULL,
  `score_horde` mediumint(9) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `boss_down`
--

DROP TABLE IF EXISTS `boss_down`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `boss_down` (
  `id` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `boss_entry` mediumint(8) unsigned NOT NULL,
  `boss_name` varchar(100) NOT NULL,
  `boss_name_fr` varchar(100) NOT NULL,
  `guild_id` mediumint(8) unsigned NOT NULL,
  `guild_name` varchar(100) NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `guild_percentage` float NOT NULL,
  `leaderGuid` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_boss` (`boss_entry`)
) ENGINE=MyISAM AUTO_INCREMENT=134708 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_auction_create`
--

DROP TABLE IF EXISTS `char_auction_create`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_auction_create` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `seller_account` int(11) unsigned NOT NULL,
  `seller_guid` int(11) unsigned NOT NULL,
  `item_guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `item_count` smallint(5) unsigned NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_auction_won`
--

DROP TABLE IF EXISTS `char_auction_won`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_auction_won` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `bidder_account` int(11) NOT NULL,
  `bidder_guid` int(11) unsigned NOT NULL,
  `seller_account` int(11) unsigned NOT NULL,
  `seller_guid` int(11) unsigned NOT NULL,
  `item_guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `item_count` smallint(5) unsigned NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_chat`
--

DROP TABLE IF EXISTS `char_chat`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_chat` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `time` int(11) unsigned NOT NULL,
  `type` tinyint(3) NOT NULL,
  `guid` int(11) unsigned NOT NULL,
  `account` int(11) unsigned NOT NULL,
  `target_guid` int(11) unsigned DEFAULT NULL,
  `channelId` int(11) unsigned DEFAULT NULL COMMENT 'for type Guild/Officer/Group/Raid/BG',
  `channelName` varchar(30) NOT NULL COMMENT 'for type Channel',
  `message` varchar(255) NOT NULL,
  `IP` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=13397 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_delete`
--

DROP TABLE IF EXISTS `char_delete`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_delete` (
  `account` int(10) unsigned NOT NULL DEFAULT '0',
  `guid` int(10) unsigned NOT NULL DEFAULT '0',
  `name` varchar(12) NOT NULL DEFAULT '',
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(16) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`guid`),
  KEY `idx_acct` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_enchant`
--

DROP TABLE IF EXISTS `char_enchant`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_enchant` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `player_guid` int(11) unsigned NOT NULL,
  `target_player_guid` int(11) unsigned NOT NULL,
  `item_guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `enchant_id` mediumint(8) unsigned NOT NULL,
  `permanent` tinyint(1) NOT NULL,
  `player_IP` varchar(15) NOT NULL,
  `target_player_IP` varchar(15) NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=167 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_guild_money_deposit`
--

DROP TABLE IF EXISTS `char_guild_money_deposit`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_guild_money_deposit` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(11) unsigned NOT NULL,
  `guid` int(11) unsigned NOT NULL COMMENT 'player guid',
  `guildId` int(6) unsigned NOT NULL,
  `amount` smallint(8) unsigned NOT NULL COMMENT 'negative is withdraw',
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_item_delete`
--

DROP TABLE IF EXISTS `char_item_delete`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_item_delete` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(11) unsigned NOT NULL,
  `playerguid` int(11) unsigned NOT NULL COMMENT 'player guid',
  `entry` mediumint(8) unsigned NOT NULL COMMENT 'item entry',
  `count` smallint(5) unsigned NOT NULL COMMENT 'item count',
  `time` int(11) unsigned NOT NULL COMMENT 'delete time',
  `IP` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=448 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_item_guild_bank`
--

DROP TABLE IF EXISTS `char_item_guild_bank`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_item_guild_bank` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(11) unsigned NOT NULL,
  `guid` int(11) unsigned NOT NULL,
  `guildId` int(6) unsigned NOT NULL,
  `direction` enum('chartoguild','guildtochar') NOT NULL,
  `item_guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `item_count` smallint(5) unsigned NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_item_vendor`
--

DROP TABLE IF EXISTS `char_item_vendor`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_item_vendor` (
  `id` bigint(20) unsigned NOT NULL,
  `transaction_type` enum('buy','sell','buyback') NOT NULL,
  `account` int(11) unsigned NOT NULL,
  `guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `item_count` smallint(5) unsigned NOT NULL,
  `vendor_entry` mediumint(8) unsigned NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(15) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_rename`
--

DROP TABLE IF EXISTS `char_rename`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_rename` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(10) unsigned NOT NULL,
  `guid` int(10) unsigned NOT NULL,
  `old_name` varchar(12) NOT NULL,
  `new_name` varchar(12) NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(16) NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_char_rename_guid` (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_trade`
--

DROP TABLE IF EXISTS `char_trade`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_trade` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `player1_account` int(11) unsigned NOT NULL,
  `player2_account` int(11) unsigned NOT NULL,
  `player1_guid` int(11) unsigned NOT NULL,
  `player2_guid` int(11) unsigned NOT NULL,
  `money1` int(11) unsigned NOT NULL DEFAULT '0',
  `money2` int(11) unsigned NOT NULL DEFAULT '0',
  `player1_IP` varchar(15) NOT NULL,
  `player2_IP` varchar(15) NOT NULL,
  `time` int(11) unsigned NOT NULL,
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `char_trade_items`
--

DROP TABLE IF EXISTS `char_trade_items`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `char_trade_items` (
  `trade_id` bigint(20) unsigned NOT NULL,
  `id` smallint(5) unsigned NOT NULL AUTO_INCREMENT,
  `p1top2` tinyint(1) NOT NULL,
  `item_guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `item_count` smallint(5) unsigned NOT NULL,
  PRIMARY KEY (`trade_id`,`id`),
  KEY `id` (`id`),
  CONSTRAINT `char_trade_items_ibfk_1` FOREIGN KEY (`trade_id`) REFERENCES `char_trade` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `gm_command`
--

DROP TABLE IF EXISTS `gm_command`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `gm_command` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `account` int(11) unsigned NOT NULL DEFAULT '0',
  `guid` int(11) unsigned NOT NULL,
  `gmlevel` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `time` int(11) unsigned NOT NULL,
  `map` mediumint(8) unsigned NOT NULL,
  `x` float NOT NULL,
  `y` float NOT NULL,
  `z` float NOT NULL,
  `area_name` varchar(20) NOT NULL,
  `zone_name` varchar(20) NOT NULL,
  `selection_type` varchar(15) NOT NULL,
  `selection_guid` int(11) NOT NULL DEFAULT '0',
  `selection_name` varchar(25) NOT NULL,
  `selection_map` mediumint(8) NOT NULL,
  `selection_x` float NOT NULL,
  `selection_y` float NOT NULL,
  `selection_z` float NOT NULL,
  `command` text NOT NULL COMMENT 'base command',
  `IP` varchar(15) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=53505 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `gm_sanction`
--

DROP TABLE IF EXISTS `gm_sanction`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `gm_sanction` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `author_account` int(11) unsigned NOT NULL,
  `author_guid` int(11) unsigned NOT NULL,
  `target_account` int(11) unsigned NOT NULL,
  `target_guid` int(11) unsigned NOT NULL,
  `target_IP` varchar(15) NOT NULL COMMENT 'only for ban IP',
  `type` tinyint(3) unsigned NOT NULL COMMENT 'see enum SanctionType',
  `duration` mediumint(8) unsigned NOT NULL COMMENT 'in secs',
  `time` int(11) NOT NULL COMMENT 'Current time',
  `reason` varchar(255) NOT NULL,
  `IP` varchar(15) NOT NULL COMMENT 'Author IP',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `gm_sanction_remove`
--

DROP TABLE IF EXISTS `gm_sanction_remove`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `gm_sanction_remove` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `author_account` int(11) unsigned NOT NULL,
  `author_guid` int(11) unsigned NOT NULL,
  `target_account` int(11) unsigned NOT NULL,
  `target_guid` int(11) unsigned NOT NULL,
  `target_IP` varchar(15) NOT NULL,
  `type` tinyint(3) unsigned NOT NULL COMMENT 'see enum SanctionType',
  `time` int(11) NOT NULL COMMENT 'Current time',
  `IP` varchar(15) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mail`
--

DROP TABLE IF EXISTS `mail`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mail` (
  `id` int(11) unsigned NOT NULL,
  `type` tinyint(3) unsigned NOT NULL,
  `sender_account` int(11) unsigned NOT NULL,
  `sender_guid_or_entry` int(11) unsigned NOT NULL,
  `receiver_guid` int(11) unsigned NOT NULL,
  `subject` longtext NOT NULL,
  `message` longtext NOT NULL,
  `money` int(11) NOT NULL DEFAULT '0' COMMENT 'can be negative',
  `time` int(11) unsigned NOT NULL,
  `IP` varchar(15) NOT NULL DEFAULT '0.0.0.0',
  `gm_involved` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mail_items`
--

DROP TABLE IF EXISTS `mail_items`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mail_items` (
  `mail_id` int(11) unsigned NOT NULL,
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `item_guid` int(11) unsigned NOT NULL,
  `item_entry` mediumint(8) unsigned NOT NULL,
  `item_count` smallint(5) unsigned NOT NULL DEFAULT '1',
  PRIMARY KEY (`mail_id`,`id`),
  KEY `id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=644 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mon_classes`
--

DROP TABLE IF EXISTS `mon_classes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mon_classes` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `time` int(10) unsigned NOT NULL,
  `class` tinyint(3) unsigned NOT NULL,
  `players` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=3645712 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mon_maps`
--

DROP TABLE IF EXISTS `mon_maps`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mon_maps` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `time` int(10) unsigned NOT NULL,
  `map` mediumint(8) unsigned NOT NULL,
  `players` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=6076186 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mon_players`
--

DROP TABLE IF EXISTS `mon_players`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mon_players` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `time` int(10) unsigned NOT NULL,
  `active` int(10) unsigned NOT NULL,
  `queued` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=405080 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mon_races`
--

DROP TABLE IF EXISTS `mon_races`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mon_races` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `time` int(10) unsigned NOT NULL,
  `race` tinyint(3) unsigned NOT NULL,
  `players` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=4050791 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mon_timediff`
--

DROP TABLE IF EXISTS `mon_timediff`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mon_timediff` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `time` int(10) unsigned NOT NULL,
  `diff` mediumint(8) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=404596 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `phishing`
--

DROP TABLE IF EXISTS `phishing`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `phishing` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `srcguid` int(11) NOT NULL,
  `dstguid` int(11) NOT NULL,
  `time` bigint(20) NOT NULL,
  `data` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `warden_fails`
--

DROP TABLE IF EXISTS `warden_fails`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `warden_fails` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `guid` int(11) unsigned NOT NULL,
  `account` int(11) unsigned NOT NULL,
  `check_id` int(4) unsigned NOT NULL,
  `comment` text NOT NULL,
  `time` bigint(11) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;


DROP TABLE IF EXISTS `updates_include`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `updates_include` (
  `path` varchar(200) NOT NULL COMMENT 'directory to include. $ means relative to the source directory.',
  `state` enum('RELEASED','ARCHIVED') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if the directory contains released or archived updates.',
  PRIMARY KEY (`path`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='List of directories where we want to include sql updates.';
/*!40101 SET character_set_client = @saved_cs_client */;

INSERT INTO `updates_include` VALUES ('$/sql/updates/logs','RELEASED'),('$/sql/custom/logs','RELEASED'),('$/sql/old/2.4.3/logs','ARCHIVED');

DROP TABLE IF EXISTS `updates`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `updates` (
  `name` varchar(200) NOT NULL COMMENT 'filename with extension of the update.',
  `hash` char(40) DEFAULT '' COMMENT 'sha1 hash of the sql file.',
  `state` enum('RELEASED','ARCHIVED') NOT NULL DEFAULT 'RELEASED' COMMENT 'defines if an update is released or archived.',
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'timestamp when the query was applied.',
  `speed` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'time the query takes to apply in ms.',
  PRIMARY KEY (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='List of all applied updates in this database.';
/*!40101 SET character_set_client = @saved_cs_client */;

/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
