#pragma once

#include <core/prerequisites.hpp>

namespace s2 {
	//shitty hack for scoping weakly-typed enums
	namespace ClientCmd {
		enum ClientCmd : uint8_t {
			DownloadWorld = 0x62,
			Connect = 0xC0,
			Vars = 0xC1,
			RequestStateStrings = 0xC2,
			Disconnect = 0xC3,
			Ready = 0xC4,
			Join = 0xC5,
			Message = 0xC6,
			Snapshot = 0xC7,
			Gamedata = 0xC8,
			AckEndgame = 0xCB,
			LoadingHeartbeat = 0xCD,
			Reauthenticate = 0xCE
		};
	}
	namespace ServerCmd {
		enum ServerCmd : uint8_t {
			KickClient = 0x05,
			ShuttingDown = 0x06,
			RequestVars = 0x50,
			DenyConnect = 0x51,
			StateReset = 0x52,
			StateUpdate = 0x53,
			CompressedStateUpdate = 0x54,
			StateFragment = 0x55,
			StateTerminate = 0x56,
			CompressedStateTerminate = 0x57,
			StateStringsEnd = 0x58,
			LoadWorld = 0x5A,
			Snapshot = 0x5B,
			CompressedSnapshot = 0x5C,
			SnapshotFragment = 0x5D,
			SnapshotTerminate = 0x5E,
			CompressedSnapshotTerminate = 0x5F,
			Gamedata = 0x60,
			ClientAuthenticated = 0x61,
			DownloadWorld = 0x62,
			DownloadFinished = 0x63,
			DownloadStart = 0x64,
			NewVoiceClient = 0x65,
			UpdateVoiceClient = 0x66,
			RemoveVoiceClient = 0x67
		};
	}
	namespace Gamedata {
		enum Gamedata : uint8_t {
			RequestUnit = 0x1,
			RequestTeam = 0x2,
			ChatAll = 0x3,
			ChatTeam = 0x4,
			ChatSquad = 0x5,
			ServerMessage = 0x6,
			SpendPoint = 0x7,
			Ping = 0x8, // maybe?
			Spawn = 0xb,
			RemoteExecute = 0xc, //remote command
			PurchaseItem = 0xd, //short:id
			SellItem = 0xe, //int:slot, int:amount
			BuildingUpkeep = 0x10, //togglebuildingupkeep command
			PromotePlayer = 0x11,
			DemotePlayer = 0x12,
			RequestCommand = 0x14,
			DeclineOfficer = 0x15,
			RequestSpawn = 0x16,
			JoinSquad = 0x17,
			BuyPersistant = 0x18,
			Message = 0x19,
			Reward = 0x1a,
			HitFeedback = 0x1b,
			MinimapDraw = 0x1c,
			Sacrifice = 0x1d,
			SpendTeamPoint = 0x1e, //spendteampoint
			MinimapPing = 0x1f,
			Resign = 0x20,
			BuildingAttackAlert = 0x21,
			PetBanish = 0x22,
			OfficerClearOrders = 0x25,
			OfficerRally = 0x26,
			EndgameTermination = 0x28,
			EndgameFragment = 0x29,
			SwapInventory = 0x2a,
			SubmitReplayComment = 0x2b,
			MalphasSpawned = 0x2d,
			ConstructionComplete = 0x2e,
			GoldmineLow = 0x2f,
			GoldmineDepleted = 0x30,
			PermanentItems = 0x31,
			CmderGivePlayerGold = 0x33, //short:goldamount, int:clientid
			VoiceCommand = 0x36,
			ExecScript = 0x38, //clientexecscript command
			SendScriptInput = 0x39, //sendscriptinput
			PlayerDonateTeamGold = 0x3d, // contribute cmd
			StartConstructBuilding = 0x3e,
			BuildingDestroyed = 0x3f,
			PlaceSpawnFlag = 0x40,
			CancelSacrifice = 0x44, // cancelsacrifice cmd
			RequestServerStatus = 0x45, // requestserverstatus cmd
			Death = 0x47,
			SpawnWorker = 0x48, //spawnworker cmd
			SendMessage = 0x4a, //sendmessage cmd
			SubmitCommanderRating = 0x4d,
			SubmitKarmaRating = 0x4e,
			GadgetAccessed = 0x50,
			SwitchDormancy = 0x51, // cancel command
			Repairable = 0x53, // ? repairable/togglerepairable command
			Killstreak = 0x57,
			RequestRestartGame = 0x59, //restart cmd
			RequestUpdateItemList = 0x5d, //updateitems cmd
			RequestCallVote = 0x5e, //int:votetype, 0=impeach, 1=concede, 2=shuffle, 3=nextmap
			ResetLifetime = 0x60,
			HqHealthstatus = 0x61,
		};
	}
}
