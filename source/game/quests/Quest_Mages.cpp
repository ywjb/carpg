#include "Pch.h"
#include "Base.h"
#include "Quest_Mages.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"
#include "Journal.h"
#include "SaveState.h"

//-----------------------------------------------------------------------------
DialogEntry mages_scholar[] = {
	IF_QUEST_PROGRESS(Quest_Magowie::Progress::None),
		TALK(357),
		TALK(358),
		TALK(359),
		TALK(360),
		TALK(361),
		CHOICE(362),
		SET_QUEST_PROGRESS(Quest_Magowie::Progress::Started),
			TALK2(363),
			TALK(364),
			END,
		END_CHOICE,
		CHOICE(365),
			TALK(366),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Magowie::Progress::Started),
		TALK(367),
		IF_HAVE_ITEM("q_magowie_kula"),
			CHOICE(368),
				SET_QUEST_PROGRESS(Quest_Magowie::Progress::Finished),
				TALK(369),
				TALK(370),
				END,
			END_CHOICE,
			CHOICE(371),
				TALK(372),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		ELSE,
			TALK(373),
			END,
		END_IF,
	END_IF,
	TALK(374),
	END,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry mages_golem[] = {
	IF_QUEST_PROGRESS(Quest_Magowie::Progress::Finished),
	SET_QUEST_PROGRESS(Quest_Magowie::Progress::EncounteredGolem),
	END_IF,
	IF_SPECIAL("q_magowie_zaplacono"),
		TALK(375),
		END,
	END_IF,
	TALK(376),
	CHOICE(377),
		SPECIAL("q_magowie_zaplac"),
		TALK(378),
		END,
	END_CHOICE,
	CHOICE(379),
		TALK(380),
		SPECIAL("attack"),
		END,
	END_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_Magowie::Start()
{
	quest_id = Q_MAGES;
	type = Type::Unique;
	// start_loc ustawiane w InitQuests
}

//=================================================================================================
DialogEntry* Quest_Magowie::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_magowie_uczony") == 0)
		return mages_scholar;
	else
		return mages_golem;
}

//=================================================================================================
void Quest_Magowie::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			name = game->txQuest[165];
			start_time = game->worldtime;
			state = Quest::Started;

			Location& sl = GetStartLocation();
			target_loc = game->GetClosestLocation(L_CRYPT, sl.pos);
			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.reset = true;
			tl.spawn = SG_NIEUMARLI;
			tl.st = 8;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}
			
			at_level = tl.GetLastLevel();
			item_to_give[0] = FindItem("q_magowie_kula");
			spawn_item = Quest_Event::Item_InTreasure;

			quest_index = game->quests.size();
			game->quests.push_back(this);
			RemoveElement<Quest*>(game->unaccepted_quests, this);

			msgs.push_back(Format(game->txQuest[166], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[167], tl.name.c_str(), GetTargetLocationDir()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;

			const Item* item = FindItem("q_magowie_kula");
			game->current_dialog->talker->AddItem(item, 1, true);
			game->RemoveItem(*game->current_dialog->pc->unit, item, 1);
			game->magowie_uczony = game->current_dialog->talker;
			game->magowie_stan = Game::MS_UCZONY_CZEKA;

			GetTargetLocation().active_quest = NULL;

			game->AddReward(1500);
			msgs.push_back(game->txQuest[168]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(!game->plotka_questowa[P_MAGOWIE])
			{
				game->plotka_questowa[P_MAGOWIE] = true;
				--game->ile_plotek_questowych;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::EncounteredGolem:
		{
			Quest_Mages* q = new Quest_Mages;
			game->magowie_refid2 = q->refid = game->quest_counter;
			++game->quest_counter;
			q->Start();
			q->name = game->txQuest[169];
			q->start_time = game->worldtime;
			q->state = Quest::Started;
			game->magowie_stan = Game::MS_SPOTKANO_GOLEMA;
			q->quest_index = game->quests.size();
			game->quests.push_back(q);
			game->plotka_questowa[P_MAGOWIE2] = false;
			++game->ile_plotek_questowych;
			q->msgs.push_back(Format(game->txQuest[170], game->day+1, game->month+1, game->year));
			q->msgs.push_back(game->txQuest[171]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, q->quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[172]);

			if(game->IsOnline())
				game->Net_AddQuest(q->refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Magowie::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else
	{
		assert(0);
		return NULL;
	}
}

//=================================================================================================
bool Quest_Magowie::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "magowie") == 0;
}

//=================================================================================================
void Quest_Magowie::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	if(!done)
	{
		item_to_give[0] = FindItem("q_magowie_kula");
		spawn_item = Quest_Event::Item_InTreasure;
	}
}

//-----------------------------------------------------------------------------
DialogEntry mages2_captain[] = {
	IF_QUEST_PROGRESS(Quest_Mages::Progress::None),
		TALK(381),
		TALK(382),
		TALK(383),
		SET_QUEST_PROGRESS(Quest_Mages::Progress::Started),
		TALK2(384),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::Started),
		TALK2(385),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(Quest_Mages::Progress::MageWantsBeer, Quest_Mages::Progress::GivenVodka),
		TALK(386),
		TALK(387),
		TALK(388),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::GotoTower),
		TALK(389),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::MageTalkedAboutTower),
		TALK(390),
		TALK(391),
		TALK(392),
		SET_QUEST_PROGRESS(Quest_Mages::Progress::TalkedWithCaptain),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(Quest_Mages::Progress::TalkedWithCaptain, Quest_Mages::Progress::MageDrinkPotion),
		TALK(393),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(Quest_Mages::Progress::NotRecruitMage, Quest_Mages::Progress::RecruitMage),
		TALK(394),
		TALK(395),
		END,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(Quest_Mages::Progress::KilledBoss, Quest_Mages::Progress::TalkedWithMage),
		TALK2(396),
		TALK(397),
		TALK(398),
		SET_QUEST_PROGRESS(Quest_Mages::Progress::Finished),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::Finished),
		TALK(399),
		END,
	END_IF,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry mages2_mage[] = {
	IF_QUEST_PROGRESS_RANGE(Quest_Mages::Progress::TalkedWithMage, Quest_Mages::Progress::Finished),
		TALK(400),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::KilledBoss),
		TALK(401),
		TALK(402),
		TALK(403),
		SET_QUEST_PROGRESS(Quest_Mages::Progress::TalkedWithMage),
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::RecruitMage),
		IF_SPECIAL("q_magowie_u_bossa"),
			TALK2(1290),
		ELSE,
			TALK2(404),
		END_IF,
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::NotRecruitMage),
		TALK(405),
		CHOICE(406),
			SET_QUEST_PROGRESS(Quest_Mages::Progress::RecruitMage),
			TALK(407),
			END,
		END_CHOICE,
		CHOICE(408),
			TALK(409),
			TALK(410),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::MageDrinkPotion),
		TALK(411),
		TALK2(412),
		TALK(413),
		TALK(414),
		TALK2(415),
		TALK(416),
		CHOICE(417),
			SET_QUEST_PROGRESS(Quest_Mages::Progress::RecruitMage),
			TALK(418),
			END,
		END_CHOICE,
		CHOICE(419),
			SET_QUEST_PROGRESS(Quest_Mages::Progress::NotRecruitMage),
			TALK(420),
			TALK(421),
			END,
		END_CHOICE,
		ESCAPE_CHOICE,
		SHOW_CHOICES,
	END_IF,
	IF_QUEST_PROGRESS_RANGE(Quest_Mages::Progress::MageTalkedAboutTower, Quest_Mages::Progress::BoughtPotion),
		TALK(422),
		IF_HAVE_ITEM("q_magowie_potion"),
			CHOICE(423),
				TALK(424),
				SET_QUEST_PROGRESS(Quest_Mages::Progress::MageDrinkPotion),
				TALK(425),
				TALK(426),
				RESTART,
			END_CHOICE,
			CHOICE(427),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		END_IF,
		END,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::GotoTower),
		IF_SPECIAL("q_magowie_u_siebie"),
			IF_SPECIAL("q_magowie_czas"),
				TALK(428),
				TALK(429),
				TALK(430),
				SET_QUEST_PROGRESS(Quest_Mages::Progress::MageTalkedAboutTower),
				END,
			ELSE,
				TALK(431),
				END,
			END_IF,
		ELSE,
			TALK2(432),
			END,
		END_IF,
	END_IF,
	IF_ONCE,
		IF_SPECIAL("is_not_known"),
			SPECIAL("tell_name"),
			TALK2(433),
		ELSE,
			TALK(434),
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::MageWantsBeer),
		IF_HAVE_ITEM("p_beer"),
			CHOICE(435),
				TALK(436),
				SET_QUEST_PROGRESS(Quest_Mages::Progress::MageWantsVodka),
				TALK(437),
				TALK(438),
				END,
			END_CHOICE,
		END_IF,
	END_IF,
	IF_QUEST_PROGRESS(Quest_Mages::Progress::MageWantsVodka),
		IF_HAVE_ITEM("p_vodka"),
			CHOICE(439),
				TALK(440),
				SET_QUEST_PROGRESS(Quest_Mages::Progress::GivenVodka),
				TALK(441),
				RESTART,
			END_CHOICE,
		END_IF,
	END_IF,
	CHOICE(442),
		IF_QUEST_PROGRESS(Quest_Mages::Progress::Started),
			TALK(443),
			SET_QUEST_PROGRESS(Quest_Mages::Progress::MageWantsBeer),
			END,
		END_IF,
		IF_QUEST_PROGRESS(Quest_Mages::Progress::MageWantsBeer),
			TALK(444),
			END,
		END_IF,
		IF_QUEST_PROGRESS(Quest_Mages::Progress::MageWantsVodka),
			TALK(445),
			END,
		END_IF,
		IF_QUEST_PROGRESS(Quest_Mages::Progress::GivenVodka),
			TALK(446),
			TALK(447),
			TALK(448),
			TALK(449),
			TALK(450),
			TALK(451),
			CHOICE(452),
				SET_QUEST_PROGRESS(Quest_Mages::Progress::GotoTower),
				TALK2(453),
				TALK(454),
				END,
			END_CHOICE,
			CHOICE(455),
				END,
			END_CHOICE,
			ESCAPE_CHOICE,
			SHOW_CHOICES,
		END_IF,
	END_CHOICE,
	CHOICE(456),
		TALK(457),
		END,
	END_CHOICE,
	CHOICE(458),
		END,
	END_CHOICE,
	ESCAPE_CHOICE,
	SHOW_CHOICES,
	END_OF_DIALOG
};

//-----------------------------------------------------------------------------
DialogEntry mages2_boss[] = {
	TALK(459),
	IF_QUEST_PROGRESS(Quest_Mages::Progress::RecruitMage),
		TALK2(460),
	END_IF,
	TALK(461),
	SPECIAL("attack"),
	END,
	END_OF_DIALOG
};

//=================================================================================================
void Quest_Mages::Start()
{
	type = Type::Unique;
	quest_id = Q_MAGES2;
	talked = Quest_Mages::Talked::No;
}

//=================================================================================================
DialogEntry* Quest_Mages::GetDialog(int type2)
{
	assert(type2 == QUEST_DIALOG_NEXT);

	if(strcmp(game->current_dialog->talker->data->id, "q_magowie_stary") == 0)
		return mages2_mage;
	else if(strcmp(game->current_dialog->talker->data->id, "q_magowie_boss") == 0)
		return mages2_boss;
	else
		return mages2_captain;
}

//=================================================================================================
void Quest_Mages::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::Started:
		// porozmawiano ze stra�nikiem o golemach, wys�a� do maga
		{
			start_loc = game->current_location;
			game->magowie_miasto = start_loc;
			game->magowie_gdzie = mage_loc = game->GetRandomCityLocation(start_loc);

			Location& sl = GetStartLocation();
			Location& ml = *game->locations[mage_loc];

			msgs.push_back(Format(game->txQuest[173], sl.name.c_str(), ml.name.c_str(), GetLocationDirName(sl.pos, ml.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			
			game->magowie_stan = Game::MS_POROZMAWIANO_Z_KAPITANEM;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::MageWantsBeer:
		// mag chce piwa
		{
			msgs.push_back(Format(game->txQuest[174], game->current_dialog->talker->hero->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::MageWantsVodka:
		// daj piwo, chce w�dy
		{
			const Item* piwo = FindItem("p_beer");
			game->RemoveItem(*game->current_dialog->pc->unit, piwo, 1);
			game->current_dialog->talker->ConsumeItem(piwo->ToConsumeable());
			game->current_dialog->dialog_wait = 2.5f;
			msgs.push_back(game->txQuest[175]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GivenVodka:
		// da�e� w�de
		{
			const Item* woda = FindItem("p_vodka");
			game->RemoveItem(*game->current_dialog->pc->unit, woda, 1);
			game->current_dialog->talker->ConsumeItem(woda->ToConsumeable());
			game->current_dialog->dialog_wait = 2.5f;
			msgs.push_back(game->txQuest[176]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::GotoTower:
		// idzie za tob� do pustej wie�y
		{
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -64.f, MAGE_TOWER, SG_BRAK, true, 2);
			Location& loc = *game->locations[target_loc];
			loc.st = 1;
			loc.state = LS_KNOWN;
			game->current_dialog->talker->hero->team_member = true;
			game->current_dialog->talker->hero->mode = HeroData::Follow;
			game->current_dialog->talker->hero->free = true;
			game->AddTeamMember(game->current_dialog->talker, false);
			msgs.push_back(Format(game->txQuest[177], game->current_dialog->talker->hero->name.c_str(), GetTargetLocationName(), GetLocationDirName(game->location->pos, GetTargetLocation().pos),
				game->location->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->magowie_gdzie = target_loc;
			game->magowie_stan = Game::MS_STARY_MAG_DOLACZYL;
			game->magowie_czas = 0;
			game->magowie_uczony = game->current_dialog->talker;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
				game->Net_RecruitNpc(game->current_dialog->talker);
			}
		}
		break;
	case Progress::MageTalkedAboutTower:
		// mag sobie przypomnia� �e to jego wie�a
		{
			game->current_dialog->talker->auto_talk = 0;
			game->magowie_stan = Game::MS_PRZYPOMNIAL_SOBIE;
			msgs.push_back(Format(game->txQuest[178], game->current_dialog->talker->hero->name.c_str(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithCaptain:
		// cpt kaza� pogada� z alchemikiem
		{
			game->magowie_stan = Game::MS_KUP_MIKSTURE;
			msgs.push_back(game->txQuest[179]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::BoughtPotion:
		// kupno miksturki
		// wywo�ywane z DT_IF_SPECAL q_magowie_kup
		{
			if(prog != Progress::BoughtPotion)
			{
				msgs.push_back(game->txQuest[180]);
				game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
				game->AddGameMsg3(GMS_JOURNAL_UPDATED);

				if(game->IsOnline())
					game->Net_UpdateQuest(refid);
			}
			const Item* item = FindItem("q_magowie_potion");
			game->current_dialog->pc->unit->AddItem(item, 1, false);
			game->current_dialog->pc->unit->gold -= 150;

			if(game->IsOnline() && !game->current_dialog->is_local)
			{
				game->Net_AddItem(game->current_dialog->pc, item, false);
				game->Net_AddedItemMsg(game->current_dialog->pc);
				game->GetPlayerInfo(game->current_dialog->pc).update_flags |= PlayerInfo::UF_GOLD;
			}
			else
				game->AddGameMsg3(GMS_ADDED_ITEM);
		}
		break;
	case Progress::MageDrinkPotion:
		// wypi� miksturk�
		{
			const Item* mikstura = FindItem("q_magowie_potion");
			game->RemoveItem(*game->current_dialog->pc->unit, mikstura, 1);
			game->current_dialog->talker->ConsumeItem(mikstura->ToConsumeable());
			game->current_dialog->dialog_wait = 3.f;
			game->magowie_stan = Game::MS_MAG_WYLECZONY;
			msgs.push_back(game->txQuest[181]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			GetTargetLocation().active_quest = NULL;
			target_loc = game->CreateLocation(L_DUNGEON, VEC2(0,0), -64.f, MAGE_TOWER, SG_MAGOWIE_I_GOLEMY);
			Location& loc = GetTargetLocation();
			loc.state = LS_HIDDEN;
			loc.st = 15;
			loc.active_quest = this;
			game->magowie_gdzie = mage_loc;
			do
			{
				game->GenerateHeroName(Class::MAGE, false, game->magowie_imie);
			}
			while(game->current_dialog->talker->hero->name == game->magowie_imie);
			done = false;
			unit_event_handler = this;
			unit_auto_talk = true;
			at_level = loc.GetLastLevel();
			unit_to_spawn = FindUnitData("q_magowie_boss");
			unit_dont_attack = true;
			unit_to_spawn2 = FindUnitData("golem_iron");
			spawn_2_guard_1 = true;

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::NotRecruitMage:
		// nie zrekrutowa�em maga
		{
			Unit* u = game->current_dialog->talker;
			u->hero->free = false;
			u->hero->team_member = false;
			u->MakeItemsTeam(true);
			RemoveElement(game->team, u);
			game->magowie_stan = Game::MS_MAG_IDZIE;

			if(game->current_location == start_loc)
			{
				// id� do karczmy
				u->ai->goto_inn = true;
				u->ai->timer = 0.f;
			}
			else
			{
				// id� do startowej lokacji do karczmy
				u->hero->mode = HeroData::Leave;
				u->event_handler = this;
			}

			Location& target = GetTargetLocation();
			target.state = LS_KNOWN;

			msgs.push_back(Format(game->txQuest[182], u->hero->name.c_str(), game->magowie_imie.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_ChangeLocationState(target_loc, false);
				game->Net_KickNpc(u);
			}
		}
		break;
	case Progress::RecruitMage:
		// zrekrutowa�em maga
		{
			Unit* u = game->current_dialog->talker;
			Location& target = GetTargetLocation();

			if(prog == Progress::MageDrinkPotion)
			{
				target.state = LS_KNOWN;
				msgs.push_back(Format(game->txQuest[183], u->hero->name.c_str(), game->magowie_imie.c_str(), target.name.c_str(), GetTargetLocationDir(), GetStartLocationName()));
			}
			else
			{
				msgs.push_back(Format(game->txQuest[184], u->hero->name.c_str()));
				u->hero->free = true;
				u->hero->mode = HeroData::Follow;
				u->hero->team_member = true;
				u->ai->goto_inn = false;
				game->AddTeamMember(u, false);
				game->magowie_imie_dobry.clear();
			}

			if(game->IsOnline())
			{
				if(prog == Progress::MageDrinkPotion)
					game->Net_ChangeLocationState(target_loc, false);
				else
					game->Net_RecruitNpc(u);
				game->Net_UpdateQuest(refid);
			}

			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->magowie_stan = Game::MS_MAG_ZREKRUTOWANY;
		}
		break;
	case Progress::KilledBoss:
		// zabito maga
		{
			if(game->magowie_stan == Game::MS_MAG_ZREKRUTOWANY)
				game->magowie_uczony->auto_talk = 1;
			game->magowie_stan = Game::MS_UKONCZONO;
			msgs.push_back(game->txQuest[185]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->AddNews(game->txQuest[186]);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::TalkedWithMage:
		// porozmawiano z magiem po
		{
			msgs.push_back(Format(game->txQuest[187], game->current_dialog->talker->hero->name.c_str(), game->magowie_imie.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			// id� sobie
			Unit* u = game->current_dialog->talker;
			u->hero->free = false;
			u->hero->team_member = false;
			u->hero->mode = HeroData::Leave;
			u->MakeItemsTeam(true);
			RemoveElement(game->team, u);
			game->magowie_uczony = NULL;

			if(game->IsOnline())
			{
				game->Net_UpdateQuest(refid);
				game->Net_KickNpc(u);
			}
		}
		break;
	case Progress::Finished:
		// odebrano nagrod�
		{
			GetTargetLocation().active_quest = NULL;
			state = Quest::Completed;
			if(game->magowie_uczony)
			{
				game->magowie_uczony->temporary = true;
				game->magowie_uczony = NULL;
			}
			game->AddReward(5000);
			msgs.push_back(game->txQuest[188]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			game->EndUniqueQuest();
			if(!game->plotka_questowa[P_MAGOWIE2])
			{
				game->plotka_questowa[P_MAGOWIE2] = true;
				--game->ile_plotek_questowych;
			}

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}

	prog = prog2;
}

//=================================================================================================
cstring Quest_Mages::FormatString(const string& str)
{
	if(str == "mage_loc")
		return game->locations[mage_loc]->name.c_str();
	else if(str == "mage_dir")
		return GetLocationDirName(GetStartLocation().pos, game->locations[mage_loc]->pos);
	else if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "target_dir2")
		return GetLocationDirName(game->location->pos, GetTargetLocation().pos);
	else if(str == "name")
		return game->current_dialog->talker->hero->name.c_str();
	else if(str == "enemy")
		return game->magowie_imie.c_str();
	else if(str == "dobry")
		return game->magowie_uczony->hero->name.c_str();
	else
	{
		assert(0);
		return NULL;
	}
}

//=================================================================================================
bool Quest_Mages::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "magowie2") == 0;
}

//=================================================================================================
void Quest_Mages::HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit)
{
	if(unit == game->magowie_uczony)
	{
		if(type == UnitEventHandler::LEAVE)
		{
			game->magowie_imie_dobry = unit->hero->name;
			game->magowie_hd.Set(*unit->human_data);
			game->magowie_stan = Game::MS_MAG_POSZEDL;
			game->magowie_uczony = NULL;
		}
	}
	else if(strcmp(unit->data->id, "q_magowie_boss") == 0 && type == UnitEventHandler::DIE && prog != Progress::KilledBoss)
	{
		SetProgress(Progress::KilledBoss);
		unit->event_handler = NULL;
	}
}

//=================================================================================================
void Quest_Mages::Save(HANDLE file)
{
	Quest_Dungeon::Save(file);

	WriteFile(file, &mage_loc, sizeof(mage_loc), &tmp, NULL);
	WriteFile(file, &talked, sizeof(talked), &tmp, NULL);
}

//=================================================================================================
void Quest_Mages::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	ReadFile(file, &mage_loc, sizeof(mage_loc), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		talked = Talked::No;
	else
		ReadFile(file, &talked, sizeof(talked), &tmp, NULL);

	if(!done && prog >= Progress::MageDrinkPotion)
	{
		unit_event_handler = this;
		unit_auto_talk = true;
		at_level = GetTargetLocation().GetLastLevel();
		unit_to_spawn = FindUnitData("q_magowie_boss");
		unit_dont_attack = true;
		unit_to_spawn2 = FindUnitData("golem_iron");
		spawn_2_guard_1 = true;
	}
}