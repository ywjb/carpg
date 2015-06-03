#include "Pch.h"
#include "Base.h"
#include "Unit.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
Unit::~Unit()
{
	delete ani;
	delete human_data;
	delete hero;
	delete player;
}

//=================================================================================================
float Unit::CalculateMaxHp() const
{
	float v = (0.8f*attrib[(int)Attribute::CON] + 0.2f*attrib[(int)Attribute::STR]);
	if(v >= 50.f)
		return data->hp_bonus * (1.f + (v-50)/50);
	else
		return data->hp_bonus * (1.f - (50-v)/100);
	//return (10.f + float(attrib[A_CON]-50)/5) * level + attrib[A_CON] + data->hp_bonus;
	//return 1.2f * attrib[A_CON] + 0.2f * attrib[A_STR] + data->hp_bonus;
}

//=================================================================================================
float Unit::CalculateAttack() const
{
	if(HaveWeapon())
		return CalculateAttack(&GetWeapon());
	else
		return (1.f + float(skill[(int)Skill::ONE_HANDED_WEAPON])/100) * (attrib[(int)Attribute::STR] + attrib[(int)Attribute::DEX]/2);
}

//=================================================================================================
float Unit::CalculateAttack(const Item* _weapon) const
{
	assert(_weapon && OR2_EQ(_weapon->type, IT_WEAPON, IT_BOW));

	int str = attrib[(int)Attribute::STR];
	float dex = CalculateDexterity();

	if(_weapon->type == IT_WEAPON)
	{
		const Weapon& w = _weapon->ToWeapon();
		const WeaponTypeInfo& wi = weapon_type_info[w.weapon_type];
		float p;
		if(str >= w.sila)
			p = 1.f;
		else
			p = float(str) / w.sila;
		return wi.str2dmg * str + wi.dex2dmg * dex + (w.dmg * p * (1.f + float(skill[(int)Skill::ONE_HANDED_WEAPON]) / 100));
	}
	else
	{
		const Bow& b = _weapon->ToBow();
		float p;
		if(str >= b.sila)
			p = 1.f;
		else
			p = float(str) / b.sila;
		return (dex + b.dmg * (1.f + float(skill[(int)Skill::BOW]) / 100)) * p;
	}
}

//=================================================================================================
float Unit::CalculateBlock(const Item* _shield) const
{
	assert(_shield && _shield->type == IT_SHIELD);

	const Shield& s = _shield->ToShield();
	float p;
	int str = attrib[(int)Attribute::STR];
	if(str >= s.sila)
		p = 1.f;
	else
		p = float(str) / s.sila;

	return float(s.def) * (1.f + float(skill[(int)Skill::SHIELD]) / 100) * p;
}

float Unit::CalculateWeaponBlock() const
{
	const Weapon& w = GetWeapon();

	float p;
	int str = attrib[(int)Attribute::STR];
	if(str >= w.sila)
		p = 1.f;
	else
		p = float(str) / w.sila;

	return float(w.dmg) * 0.66f * (1.f + float(skill[(int)Skill::SHIELD])*0.008f + float(skill[(int)Skill::ONE_HANDED_WEAPON])*0.002f) * p;
}

//=================================================================================================
// WZ�R NA OBRON�
// kondycja/5 + pancerz * (skill * (1+max(1.f, si�a/wymagana)) + zr�czno��/5*max(%obci��enia, ci�ki_pancerz ? 0.5 : 0)
float Unit::CalculateDefense() const
{
	float def = float(attrib[(int)Attribute::CON])/10 + data->def_bonus;
	float load = GetLoad();

	// pancerz
	if(HaveArmor())
	{
		const Armor& a = GetArmor();

		// pancerz daje tyle ile bazowo * skill
		Skill sk;
		if(!OR2_EQ(a.armor_type, A_HEAVY, A_MONSTER_HEAVY))
		{
			sk = Skill::LIGHT_ARMOR;
			load *= 0.5f;
		}
		else
			sk = Skill::HEAVY_ARMOR;

		float skill_val;
		int str = attrib[(int)Attribute::STR];
		if(str >= a.sila)
			skill_val = float(skill[(int)sk]);
		else
			skill_val = float(skill[(int)sk]) * str / a.sila;
		def += (skill_val/100+1)*a.def;
	}

	// zr�czno��
	if(load < 1.f)
	{
		float dex = CalculateDexterity();
		if(dex > 50.f)
			def += ((dex - 50.f) / 3) * (1.f-load);
	}

	return def;
}

//=================================================================================================
float Unit::CalculateDefense(const Item* _armor) const
{
	assert(_armor && _armor->type == IT_ARMOR);

	float def = float(attrib[(int)Attribute::CON])/10 + data->def_bonus;
	float load = GetLoad();

	const Armor& a = _armor->ToArmor();

	// pancerz daje tyle ile bazowo * skill
	Skill sk;
	if(!a.IsHeavy())
	{
		sk = Skill::LIGHT_ARMOR;
		load *= 0.5f;
	}
	else
		sk = Skill::HEAVY_ARMOR;

	float skill_val;
	int str = attrib[(int)Attribute::STR];
	if(str >= a.sila)
		skill_val = float(skill[(int)sk]);
	else
		skill_val = float(skill[(int)sk]) * str / a.sila;
	def += (skill_val/100+1)*a.def;

	// zr�czno��
	if(load < 1.f)
	{
		float dex = CalculateDexterity(a);
		if(dex > 50.f)
			def += ((dex - 50.f) / 3) * (1.f-load);
	}

	return def;
}

//=================================================================================================
float Unit::CalculateDexterity() const
{
	if(HaveArmor())
		return CalculateDexterity(GetArmor());
	else
		return (float)attrib[(int)Attribute::DEX];
}

//=================================================================================================
float Unit::CalculateDexterity(const Armor& armor) const
{
	float dex = (float)attrib[(int)Attribute::DEX];

	int str = attrib[(int)Attribute::STR];
	if(armor.sila > str)
		dex *= float(str)/armor.sila; 
	
	int max_dex;
	if(armor.IsHeavy())
		max_dex = int((1.f + float(skill[(int)Skill::HEAVY_ARMOR]) / 200)*armor.zrecznosc);
	else
		max_dex = int((1.f + float(skill[(int)Skill::LIGHT_ARMOR]) / 100)*armor.zrecznosc);

	if(dex > (float)max_dex)
	{
		float z = (float)max_dex;
		return z + (dex - z) * (z / dex);
	}

	return dex;
}

//=================================================================================================
bool Unit::DropItem(int index)
{
	Game& game = Game::Get();
	bool no_more = false;

	ItemSlot& s = items[index];
	--s.count;
	weight -= s.item->weight;

	action = A_ANIMATION;
	ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	ani->frame_end_info = false;

	if(game.IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = s.item;
		item->count = 1;
		if(s.team_count > 0)
		{
			--s.team_count;
			item->team_count = 1;
		}
		else
			item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin()+index);
		}
		if(!game.CheckMoonStone(item, this))
			game.AddGroundItem(game.GetContext(*this), item);
		
		if(game.IsServer())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;
		}
	}
	else
	{
		if(s.team_count > 0)
			--s.team_count;
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin()+index);
		}

		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::DROP_ITEM;
		c.id = index;
		c.ile = 1;
	}

	return no_more;
}

void Unit::DropItem(ITEM_SLOT slot)
{
	assert(slots[slot]);
	Game& game = Game::Get();
	const Item*& item2 = slots[slot];

	weight -= item2->weight;

	action = A_ANIMATION;
	ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	ani->frame_end_info = false;

	if(game.IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = item2;
		item->count = 1;
		item->team_count = 0;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		item2 = NULL;
		game.AddGroundItem(game.GetContext(*this), item);

		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;

			NetChange& c2 = Add1(game.net_changes);
			c2.unit = this;
			c2.id = slot;
			c2.type = NetChange::CHANGE_EQUIPMENT;
		}
	}
	else
	{
		item2 = NULL;

		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::DROP_ITEM;
		c.id = SlotToIIndex(slot);
		c.ile = 1;
	}
}

//=================================================================================================
bool Unit::DropItems(int index, uint count)
{
	Game& game = Game::Get();
	bool no_more = false;

	ItemSlot& s = items[index];
	assert(count <= s.count);
	if(count == 0)
		count = s.count;
	s.count -= count;
	
	weight -= s.item->weight*count;

	action = A_ANIMATION;
	ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	ani->frame_end_info = false;

	if(game.IsLocal())
	{
		GroundItem* item = new GroundItem;
		item->item = s.item;
		item->count = count;
		item->team_count = min(count, s.team_count);
		s.team_count -= item->team_count;
		item->pos = pos;
		item->pos.x -= sin(rot)*0.25f;
		item->pos.z -= cos(rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin()+index);
		}
		game.AddGroundItem(game.GetContext(*this), item);

		if(game.IsServer())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = this;
		}
	}
	else
	{
		s.team_count -= min(count, s.team_count);
		if(s.count == 0)
		{
			no_more = true;
			items.erase(items.begin()+index);
		}

		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::DROP_ITEM;
		c.id = index;
		c.ile = count;
	}

	return no_more;
}

//=================================================================================================
void Unit::RecalculateWeight()
{
	weight = 0;

	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(slots[i])
			weight += slots[i]->weight;
	}

	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item)
			weight += it->item->weight * it->count;
	}
}

//=================================================================================================
int Unit::ConsumeItem(int index)
{
	assert(index >= 0 && index < int(items.size()));

	// je�li co� robi to nie mo�e u�y�
	if(action != A_NONE)
	{
		if(action == A_TAKE_WEAPON && stan_broni == BRON_CHOWA)
		{
			// je�li chowa bro� to u�yj miksturki jak schowa
			if(IsPlayer())
			{
				if(player == Game::Get().pc)
				{
					assert(Inventory::lock_id == LOCK_NO);
					player->po_akcja = PO_WYPIJ;
					Inventory::lock_index = index;
					Inventory::lock_id = LOCK_MY;
					return 2;
				}
				else
				{
					action = A_NONE;
					stan_broni = BRON_SCHOWANA;
					wyjeta = B_BRAK;
					chowana = B_BRAK;
				}
			}
			else
			{
				ai->potion = index;
				return 2;
			}
		}
		else if(!CanDoWhileUsing())
			return 3;
	}

	// je�li bro� jest wyj�ta to schowaj
	if(stan_broni != BRON_SCHOWANA)
	{
		HideWeapon();
		if(IsPlayer())
		{
			assert(Inventory::lock_id == LOCK_NO && Game::Get().pc == player);
			player->po_akcja = PO_WYPIJ;
			Inventory::lock_index = index;
			Inventory::lock_id = LOCK_MY;
		}
		else
			ai->potion = index;
		return 2;
	}

	ItemSlot& slot = items[index];

	assert(slot.item && slot.item->type == IT_CONSUMEABLE);

	const Consumeable& cons = slot.item->ToConsumeable();

	// usu� przedmiot
	--slot.count;
	weight -= cons.weight;
	bool removed = false;
	if(slot.team_count)
	{
		--slot.team_count;
		used_item_is_team = true;
	}
	else
		used_item_is_team = false;
	if(slot.count == 0)
	{
		items.erase(items.begin()+index);
		removed = true;
	}

	// zacznij spo�ywa�
	cstring anim_name;
	if(cons.cons_type == Food)
	{
		action = A_EAT;
		anim_name = "je";
	}
	else
	{
		action = A_DRINK;
		anim_name = "pije";
	}
	etap_animacji = 0;
	ani->Play(anim_name, PLAY_ONCE|PLAY_PRIO1, 1);
	used_item = &cons;

	// wy�lij komunikat
	Game& game = Game::Get();
	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::CONSUME_ITEM;
		if(game.IsServer())
		{
			c.unit = this;
			c.id = (int)used_item;
			c.ile = 0;
		}
		else
			c.id = index;
	}

	return removed ? 0 : 1;
}

//=================================================================================================
void Unit::ConsumeItem(const Consumeable& item, bool force, bool send)
{
	if(action != A_NONE && action != A_ANIMATION2)
	{
		if(Game::Get().IsLocal())
		{
			assert(0);
			return;
		}
		else
			stan_broni = BRON_SCHOWANA;
	}

	cstring anim_name;
	if(item.cons_type == Food)
	{
		action = A_EAT;
		anim_name = "je";
	}
	else
	{
		action = A_DRINK;
		anim_name = "pije";
	}
	
	etap_animacji = 0;
	ani->Play(anim_name, PLAY_ONCE|PLAY_PRIO1, 1);
	used_item = &item;
	used_item_is_team = true;

	if(send)
	{
		Game& game = Game::Get();
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::CONSUME_ITEM;
			c.unit = this;
			c.id = (int)&item;
			c.ile = (force ? 1 : 0);
		}
	}
}

//=================================================================================================
void Unit::HideWeapon()
{
	switch(stan_broni)
	{
	case BRON_SCHOWANA:
		return;
	case BRON_CHOWA:
		// anuluje wyci�ganie nast�pnej broni po schowaniu tej
		wyjeta = B_BRAK;
		return;
	case BRON_WYJMUJE:
		if(etap_animacji == 0)
		{
			// jeszcze nie wyj�� broni z pasa, po prostu wy��cz t� grupe
			action = A_NONE;
			wyjeta = B_BRAK;
			stan_broni = BRON_SCHOWANA;
			ani->Deactivate(1);
		}
		else
		{
			// wyj�� bro� z pasa, zacznij chowa�
			chowana = wyjeta;
			wyjeta = B_BRAK;
			stan_broni = BRON_CHOWA;
			etap_animacji = 0;
			SET_BIT(ani->groups[1].state, AnimeshInstance::FLAG_BACK);
		}
		break;
	case BRON_WYJETA:
		ani->Play(GetTakeWeaponAnimation(wyjeta == B_JEDNORECZNA), PLAY_PRIO1|PLAY_ONCE|PLAY_BACK, 1);
		chowana = wyjeta;
		wyjeta = B_BRAK;
		etap_animacji = 0;
		action = A_TAKE_WEAPON;
		stan_broni = BRON_CHOWA;
		ani->frame_end_info2 = false;
		break;
	}

	Game& game = Game::Get();
	if(game.IsOnline())
	{
		NetChange& c = Add1(game.net_changes);
		c.type = NetChange::TAKE_WEAPON;
		c.unit = this;
		c.id = 1;
	}
}

//=================================================================================================
void Unit::TakeWeapon(BRON _type)
{
	assert(_type == B_JEDNORECZNA || _type == B_LUK);

	if(action != A_NONE)
		return;

	if(wyjeta == _type)
		return;

	if(wyjeta == B_BRAK)
	{
		ani->Play(GetTakeWeaponAnimation(_type == B_JEDNORECZNA), PLAY_PRIO1|PLAY_ONCE, 1);
		chowana = B_BRAK;
		wyjeta = _type;
		etap_animacji = 0;
		action = A_TAKE_WEAPON;
		stan_broni = BRON_WYJMUJE;
		ani->frame_end_info2 = false;

		Game& game = Game::Get();
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::TAKE_WEAPON;
			c.unit = this;
			c.id = 0;
		}
	}
	else
	{
		HideWeapon();
		wyjeta = _type;
	}
}

//=================================================================================================
// Dodaje przedmiot(y) do ekwipunku, zwraca true je�li przedmiot si� zestackowa�
//=================================================================================================
bool Unit::AddItem(const Item* item, uint count, uint team_count)
{
	assert(item && count != 0 && team_count <= count);

	Game& game = Game::Get();
	if(item->type == IT_GOLD && game.IsTeamMember(*this))
	{
		if(game.IsLocal())
		{
			if(team_count && IsTeamMember())
			{
				game.AddGold(team_count);
				uint normal_gold = count - team_count;
				if(normal_gold)
				{
					gold += normal_gold;
					if(IsPlayer() && player != game.pc)
						game.GetPlayerInfo(player).UpdateGold();
				}
			}
			else
			{
				gold += count;
				if(IsPlayer() && player != game.pc)
					game.GetPlayerInfo(player).UpdateGold();
			}
		}
		return true;
	}

	weight += item->weight * count;

	return InsertItem(items, item, count, team_count);
}

//=================================================================================================
void Unit::ApplyConsumeableEffect(const Consumeable& item)
{
	Game& game = Game::Get();

	switch(item.effect)
	{
	case E_HEAL:
		hp += item.power;
		if(hp > hpmax)
			hp = hpmax;
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
		break;
	case E_POISON:
	case E_ALCOHOL:
		if(!IS_SET(data->flagi, F_ODPORNOSC_NA_TRUCIZNY))
		{
			Effect& e = Add1(effects);
			e.effect = item.effect;
			e.time = item.time;
			e.power = item.power/item.time;
		}
		break;
	case E_REGENERATE:
	case E_NATURAL:
	case E_ANTIMAGIC:
		{
			Effect& e = Add1(effects);
			e.effect = item.effect;
			e.time = item.time;
			e.power = item.power;
		}
		break;
	case E_ANTIDOTE:
		{
			uint index = 0;
			for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
			{
				if(it->effect == E_POISON)
					_to_remove.push_back(index);
			}

			while(!_to_remove.empty())
			{
				index = _to_remove.back();
				_to_remove.pop_back();
				if(index == effects.size()-1)
					effects.pop_back();
				else
				{
					std::iter_swap(effects.begin()+index, effects.end()-1);
					effects.pop_back();
				}
			}

			if(alcohol != 0.f)
			{
				alcohol = 0.f;
				if(IsPlayer() && game.pc != player)
					game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
			}
		}
		break;
	case E_NONE:
		break;
	case E_STR:
		game.Train(*this, false, (int)Attribute::STR);
		break;
	case E_END:
		game.Train(*this, false, (int)Attribute::CON);
		break;
	case E_DEX:
		game.Train(*this, false, (int)Attribute::DEX);
		break;
	case E_FOOD:
		{
			Effect& e = Add1(effects);
			e.effect = E_FOOD;
			e.time = item.power;
			e.power = 1.f;
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void Unit::UpdateEffects(float dt)
{
	float best_reg = 0.f, food_heal = 0.f, poison_dmg = 0.f, alco_sum = 0.f;

	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == E_NATURAL)
			continue;
		switch(it->effect)
		{
		case E_REGENERATE:
			if(it->power > best_reg)
				best_reg = it->power;
			break;
		case E_POISON:
			poison_dmg += it->power;
			break;
		case E_ALCOHOL:
			alco_sum += it->power;
			break;
		case E_FOOD:
			food_heal += dt;
			break;
		}
		if((it->time -= dt) <= 0.f)
			_to_remove.push_back(index);
	}

	Game& game = Game::Get();

	if((best_reg > 0.f || food_heal > 0.f) && hp != hpmax)
	{
		float natural = 1.f;
		FindEffect(E_NATURAL, &natural);
		hp += (best_reg * dt + food_heal) * natural;
		if(hp > hpmax)
			hp = hpmax;
		if(game.IsOnline())
		{
			NetChange& c = Add1(game.net_changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = this;
		}
	}

	if(alco_sum > 0.f)
	{
		alcohol += alco_sum*dt;
		if(alcohol >= hpmax && live_state == ALIVE)
			game.UnitFall(*this);
		if(IsPlayer() && player != game.pc)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
	}
	else if(alcohol != 0.f)
	{
		alcohol -= dt/10*attrib[(int)Attribute::CON];
		if(alcohol < 0.f)
			alcohol = 0.f;
		if(IsPlayer() && player != game.pc)
			game.GetPlayerInfo(player).update_flags |= PlayerInfo::UF_ALCOHOL;
	}

	if(poison_dmg != 0.f)
		game.GiveDmg(game.GetContext(*this), NULL, poison_dmg * dt, *this, NULL, DMG_NO_BLOOD);
	if(IsPlayer())
	{
		if(game.IsOnline() && player != game.pc && player->last_dmg_poison != poison_dmg)
			game.game_players[player->id].update_flags |= PlayerInfo::UF_POISON_DAMAGE;
		player->last_dmg_poison = poison_dmg;
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
void Unit::EndEffects(int days, int* best_nat)
{
	if(best_nat)
		*best_nat = 0;

	alcohol = 0.f;
	if(effects.empty())
		return;

	uint index = 0;
	float best_reg = 0.f, best_natural = 1.f, food = 0.f;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		switch(it->effect)
		{
		case E_REGENERATE:
			{
				float reg = it->power * it->time;
				if(reg > best_reg)
					best_reg = reg;
				_to_remove.push_back(index);
			}
			break;
		case E_POISON:
			hp -= it->power * it->time;
			_to_remove.push_back(index);
			break;
		case E_FOOD:
			food += it->time;
			_to_remove.push_back(index);
			break;
		case E_ALCOHOL:
			_to_remove.push_back(index);
			break;
		case E_NATURAL:
			best_natural = 2.f;
			if(best_nat)
			{
				int t = roundi(it->time);
				if(t > *best_nat)
					*best_nat = t;
			}
			it->time -= days;
			if(it->time <= 0.f)
				_to_remove.push_back(index);
			break;
		}
	}

	hp += (best_reg + food) * best_natural;
	if(hp < 1.f)
		hp = 1.f;
	else if(hp > hpmax)
		hp = hpmax;

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
// Dodaje przedmioty do ekwipunku i zak�ada je je�li nie ma nic za�o�onego. Dodane przedmioty s�
// traktowane jako dru�ynowe
//=================================================================================================
void Unit::AddItemAndEquipIfNone(const Item* item, uint count)
{
	assert(item && count != 0);

	if(item->IsStackable())
		AddItem(item, count, count);
	else
	{
		// za�� je�li nie ma
		switch(item->type)
		{
		case IT_WEAPON:
			if(!HaveWeapon())
			{
				slots[SLOT_WEAPON] = item;
				--count;
			}
			break;
		case IT_BOW:
			if(!HaveBow())
			{
				slots[SLOT_BOW] = item;
				--count;
			}
			break;
		case IT_SHIELD:
			if(!HaveShield())
			{
				slots[SLOT_SHIELD] = item;
				--count;
			}
			break;
		case IT_ARMOR:
			if(!HaveArmor())
			{
				slots[SLOT_ARMOR] = item;
				--count;
			}
			break;
		}

		if(count)
			AddItem(item, count, count);
	}
}

//=================================================================================================
void Unit::GetBox(BOX& _box) const
{
	float radius = GetUnitRadius() /*/ SQRT_2*/;

	// box jest wpisany w ko�o!
	// ^ ju� nie
	
	_box.v1.x = pos.x - radius;
	_box.v1.y = pos.y;
	_box.v1.z = pos.z - radius;
	_box.v2.x = pos.x + radius;
	_box.v2.y = pos.y + max(MIN_H, GetUnitHeight());
	_box.v2.z = pos.z + radius;
}

//=================================================================================================
int Unit::GetDmgType() const
{
	if(HaveWeapon())
		return GetWeapon().dmg_type;
	else
		return data->dmg_type;
}

//=================================================================================================
VEC3 Unit::GetLootCenter() const
{
	Animesh::Point* point2 = ani->ani->GetPoint("centrum");

	if(!point2)
		return pos;

	const Animesh::Point& point = *point2;
	MATRIX matBone = point.mat * ani->mat_bones[point.bone];

	MATRIX matPos, matRot;
	D3DXMatrixTranslation(&matPos, pos);
	D3DXMatrixRotationY(&matRot, rot);

	matBone = matBone * (matRot * matPos);

	VEC3 pos(0,0,0), out;
	D3DXVec3TransformCoord(&out, &pos, &matBone);

	return out;
}

//=================================================================================================
float Unit::CalculateWeaponPros(const Weapon& _weapon) const
{
	// oblicz dps i tyle na t� wersj�
	return CalculateAttack(&_weapon) * GetAttackSpeed(&_weapon);
}

//=================================================================================================
bool Unit::IsBetterWeapon(const Weapon& _weapon) const
{
	if(!HaveWeapon())
		return true;

	return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(_weapon);
}

//=================================================================================================
bool Unit::IsBetterWeapon(const Weapon& _weapon, int* value) const
{
	if(!HaveWeapon())
	{
		if(value)
			*value = 100;
		return true;
	}

	if(value)
	{
		float v = CalculateWeaponPros(_weapon);
		*value = (int)v;
		return CalculateWeaponPros(GetWeapon()) < v;
	}
	else
		return CalculateWeaponPros(GetWeapon()) < CalculateWeaponPros(_weapon);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& _armor) const
{
	if(!HaveArmor())
		return true;

	return CalculateDefense(&GetArmor()) < CalculateDefense(&_armor);
}

//=================================================================================================
bool Unit::IsBetterArmor(const Armor& _armor, int* value) const
{
	if(!HaveArmor())
	{
		if(value)
			*value = 100;
		return true;
	}

	if(value)
	{
		float v = CalculateDefense(&_armor);
		*value = (int)v;
		return CalculateDefense(&GetArmor()) < v;
	}
	else
		return CalculateDefense(&GetArmor()) < CalculateDefense(&_armor);
}

//=================================================================================================
void Unit::RecalculateHp()
{
	float hpp = hp/hpmax;
	hpmax = CalculateMaxHp();
	hp = hpmax * hpp;
	if(hp < 1)
		hp = 1;
	else if(hp > hpmax)
		hp = hpmax;
}

//=================================================================================================
float Unit::CalculateShieldAttack() const
{
	assert(HaveShield());

	const Shield& s = GetShield();
	float p;

	int str = attrib[(int)Attribute::STR];
	if(str >= s.sila)
		p = 1.f;
	else
		p = float(str) / s.sila;

	return float(str) / 2 + CalculateDexterity() / 4 + (s.def * p * (1.f + float(skill[(int)Skill::SHIELD]) / 200));
}

//=================================================================================================
float Unit::GetAttackFrame(int frame) const
{
	assert(in_range(frame, 0, 2));

	assert(attack_id < data->frames->attacks);

	if(!data->frames->extra)
	{
		switch(frame)
		{
		case 0:
			return data->frames->t[F_ATTACK1_START+attack_id*2+0];
		case 1:
			return data->frames->lerp(F_ATTACK1_START+attack_id*2);
		case 2:
			return data->frames->t[F_ATTACK1_START+attack_id*2+1];
		default:
			assert(0);
			return data->frames->t[F_ATTACK1_START+attack_id*2+1];
		}
	}
	else
	{
		switch(frame)
		{
		case 0:
			return data->frames->extra->e[attack_id].start;
		case 1:
			return data->frames->extra->e[attack_id].lerp();
		case 2:
			return data->frames->extra->e[attack_id].end;
		default:
			assert(0);
			return data->frames->extra->e[attack_id].end;
		}
	}
}

//=================================================================================================
int Unit::GetRandomAttack() const
{
	if(data->frames->extra)
	{
		int a;

		switch(GetWeapon().weapon_type)
		{
		case WT_LONG:
			a = A_LONG_BLADE;
			break;
		case WT_SHORT:
			a = A_SHORT_BLADE;
			break;
		case WT_MACE:
			a = A_BLUNT;
			break;
		case WT_AXE:
			a = A_AXE;
			break;
		default:
			assert(0);
			a = A_LONG_BLADE;
			break;
		}

		do 
		{
			int n = rand2()%data->frames->attacks;
			if(IS_SET(data->frames->extra->e[n].flags, a))
				return n;
		}
		while(1);
	}
	else
		return rand2()%data->frames->attacks;
}

//=================================================================================================
void Unit::Save(HANDLE file, bool local)
{
	// id postaci
	byte len = (byte)strlen(data->id);
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, data->id, len, &tmp, NULL);

	// przedmioty
	for(uint i=0; i<SLOT_MAX; ++i)
	{
		if(slots[i])
			WriteString1(file, slots[i]->id);
		else
		{
			byte zero = 0;
			WriteFile(file, &zero, sizeof(zero), &tmp, NULL);
		}
	}
	uint ile = items.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item)
		{
			byte len = (byte)strlen(it->item->id);
			WriteFile(file, &len, sizeof(len), &tmp, NULL);
			WriteFile(file, it->item->id, len, &tmp, NULL);
			WriteFile(file, &it->count, sizeof(it->count), &tmp, NULL);
			WriteFile(file, &it->team_count, sizeof(it->team_count), &tmp, NULL);
			if(it->item->id[0] == '$')
				WriteFile(file, &it->item->refid, sizeof(int), &tmp, NULL);
		}
		else
		{
			byte b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, NULL);
		}
	}

	WriteFile(file, &live_state, sizeof(live_state), &tmp, NULL);
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &hp, sizeof(hp), &tmp, NULL);
	WriteFile(file, &hpmax, sizeof(hpmax), &tmp, NULL);
	WriteFile(file, &type, sizeof(type), &tmp, NULL);
	WriteFile(file, &level, sizeof(level), &tmp, NULL);
	File f(file);
	f << attrib;
	f << skill;
	WriteFile(file, &gold, sizeof(gold), &tmp, NULL);
	WriteFile(file, &invisible, sizeof(invisible), &tmp, NULL);
	WriteFile(file, &in_building, sizeof(in_building), &tmp, NULL);
	WriteFile(file, &to_remove, sizeof(to_remove), &tmp, NULL);
	WriteFile(file, &temporary, sizeof(temporary), &tmp, NULL);
	WriteFile(file, &quest_refid, sizeof(quest_refid), &tmp, NULL);
	WriteFile(file, &assist, sizeof(assist), &tmp, NULL);
	WriteFile(file, &auto_talk, sizeof(auto_talk), &tmp, NULL);
	if(auto_talk == 2)
		WriteFile(file, &auto_talk_timer, sizeof(auto_talk_timer), &tmp, NULL);
	WriteFile(file, &dont_attack, sizeof(dont_attack), &tmp, NULL);
	WriteFile(file, &attack_team, sizeof(attack_team), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);
	int unit_event_handler_quest_refid = (event_handler ? event_handler->GetUnitEventHandlerQuestRefid() : -1);
	WriteFile(file, &unit_event_handler_quest_refid, sizeof(unit_event_handler_quest_refid), &tmp, NULL);
	WriteFile(file, &weight, sizeof(weight), &tmp, NULL);
	int guard_refid = (guard_target ? guard_target->refid : -1);
	WriteFile(file, &guard_refid, sizeof(guard_refid), &tmp, NULL);

	if(human_data)
	{
		byte b = 1;
		WriteFile(file, &b, sizeof(b), &tmp, NULL);
		human_data->Save(file);
	}
	else
	{
		byte b = 0;
		WriteFile(file, &b, sizeof(b), &tmp, NULL);
	}

	if(local)
	{
		ani->Save(file);
		WriteFile(file, &animacja, sizeof(animacja), &tmp, NULL);
		WriteFile(file, &animacja2, sizeof(animacja2), &tmp, NULL);

		WriteFile(file, &prev_pos, sizeof(prev_pos), &tmp, NULL);
		WriteFile(file, &speed, sizeof(speed), &tmp, NULL);
		WriteFile(file, &prev_speed, sizeof(prev_speed), &tmp, NULL);
		WriteFile(file, &etap_animacji, sizeof(etap_animacji), &tmp, NULL);
		WriteFile(file, &attack_id, sizeof(attack_id), &tmp, NULL);
		WriteFile(file, &action, sizeof(action), &tmp, NULL);
		WriteFile(file, &wyjeta, sizeof(wyjeta), &tmp, NULL);
		WriteFile(file, &chowana, sizeof(chowana), &tmp, NULL);
		WriteFile(file, &stan_broni, sizeof(stan_broni), &tmp, NULL);
		WriteFile(file, &trafil, sizeof(trafil), &tmp, NULL);
		WriteFile(file, &hurt_timer, sizeof(hurt_timer), &tmp, NULL);
		WriteFile(file, &target_pos, sizeof(target_pos), &tmp, NULL);
		WriteFile(file, &target_pos2, sizeof(target_pos2), &tmp, NULL);
		WriteFile(file, &talking, sizeof(talking), &tmp, NULL);
		WriteFile(file, &talk_timer, sizeof(talk_timer), &tmp, NULL);
		WriteFile(file, &attack_power, sizeof(attack_power), &tmp, NULL);
		WriteFile(file, &atak_w_biegu, sizeof(atak_w_biegu), &tmp, NULL);
		WriteFile(file, &timer, sizeof(timer), &tmp, NULL);
		WriteFile(file, &alcohol, sizeof(alcohol), &tmp, NULL);
		WriteFile(file, &raise_timer, sizeof(raise_timer), &tmp, NULL);

		if(used_item)
		{
			byte len = (byte)strlen(used_item->id);
			WriteFile(file, &len, sizeof(len), &tmp, NULL);
			WriteFile(file, used_item->id, len, &tmp, NULL);
			WriteFile(file, &used_item_is_team, sizeof(used_item_is_team), &tmp, NULL);
		}
		else
		{
			byte b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, NULL);
		}

		if(useable)
		{
			if(useable->user != this)
			{
				WARN(Format("Invalid useable %s (%d) user %s.", useable->GetBase()->id, useable->refid, data->id));
				useable = NULL;
				int refi = -1;
				WriteFile(file, &refi, sizeof(refi), &tmp, NULL);
			}
			else
				WriteFile(file, &useable->refid, sizeof(useable->refid), &tmp, NULL);
		}
		else
		{
			int refi = -1;
			WriteFile(file, &refi, sizeof(refi), &tmp, NULL);
		}

		WriteFile(file, &last_bash, sizeof(last_bash), &tmp, NULL);
	}

	// efekty
	ile = effects.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	if(ile)
		WriteFile(file, &effects[0], sizeof(Effect)*ile, &tmp, NULL);

	if(player)
	{
		byte b = 1;
		WriteFile(file, &b, sizeof(b), &tmp, NULL);
		player->Save(file);
	}
	else
	{
		byte b = 0;
		WriteFile(file, &b, sizeof(b), &tmp, NULL);
	}

	if(hero)
		hero->Save(file);
}

inline bool IsEmptySlot(const ItemSlot& slot)
{
	return !slot.item;
}

//=================================================================================================
void Unit::Load(HANDLE file, bool local)
{
	human_data = NULL;

	// id postaci
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, NULL);
	data = FindUnitData(BUF);

	// przedmioty
	bool can_sort = true;
	if(LOAD_VERSION >= V_0_2_10)
	{
		for(int i=0; i<SLOT_MAX; ++i)
		{
			ReadString1(file);
			slots[i] = (BUF[0] ? ::FindItem(BUF) : NULL);
		}
	}
	else
	{
		for(int i=0; i<SLOT_MAX; ++i)
			slots[i] = NULL;
	}
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	items.resize(ile);
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, NULL);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, NULL);
			ReadFile(file, &it->count, sizeof(it->count), &tmp, NULL);
			ReadFile(file, &it->team_count, sizeof(it->team_count), &tmp, NULL);
			if(LOAD_VERSION < V_0_2_10)
			{
				int equipped_as;
				ReadFile(file, &equipped_as, sizeof(equipped_as), &tmp, NULL);
			}
			if(BUF[0] != '$')
			{
				if(strcmp(BUF, "gold") == 0)
					it->item = &Game::Get().gold_item;
				else
					it->item = ::FindItem(BUF);
			}
			else
			{
				int quest_refid;
				ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, NULL);
				Game::Get().AddQuestItemRequest(&it->item, BUF, quest_refid, &items, this);
				it->item = QUEST_ITEM_PLACEHOLDER;
				can_sort = false;
			}
		}
		else
		{
			assert(LOAD_VERSION < V_0_2_10);
			it->item = NULL;
			it->count = 0;
		}
	}

	ReadFile(file, &live_state, sizeof(live_state), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_20 && live_state != ALIVE)
		live_state = LiveState(live_state + 2); // kolejno�� si� zmieni�a
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &hp, sizeof(hp), &tmp, NULL);
	ReadFile(file, &hpmax, sizeof(hpmax), &tmp, NULL);
	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_10)
	{
		int weapon, bow, shield, armor;
		ReadFile(file, &weapon, sizeof(weapon), &tmp, NULL);
		ReadFile(file, &shield, sizeof(shield), &tmp, NULL);
		ReadFile(file, &bow, sizeof(bow), &tmp, NULL);
		ReadFile(file, &armor, sizeof(armor), &tmp, NULL);
		if(weapon != -1)
		{
			slots[SLOT_WEAPON] = items[weapon].item;
			items[weapon].item = NULL;
		}
		if(bow != -1)
		{
			slots[SLOT_BOW] = items[bow].item;
			items[bow].item = NULL;
		}
		if(shield != -1)
		{
			slots[SLOT_SHIELD] = items[shield].item;
			items[shield].item = NULL;
		}
		if(armor != -1)
		{
			slots[SLOT_ARMOR] = items[armor].item;
			items[armor].item = NULL;
		}
		RemoveElements(items, IsEmptySlot);
	}
	ReadFile(file, &level, sizeof(level), &tmp, NULL);
	File f(file);
	if(LOAD_VERSION >= V_DEVEL)
	{
		f >> attrib;
		f >> skill;
	}
	else
	{
		for(int i = 0; i < 3; ++i)
			f >> attrib[i];
		for(int i = 3; i < 6; ++i)
			attrib[i] = -1;
		for(int i = 0; i< (int)Skill::MAX; ++i)
			skill[i] = -1;
		int old_skill[(int)OldSkill::MAX];
		f >> old_skill;
		skill[(int)Skill::ONE_HANDED_WEAPON] = old_skill[(int)OldSkill::WEAPON];
		skill[(int)Skill::BOW] = old_skill[(int)OldSkill::BOW];
		skill[(int)Skill::SHIELD] = old_skill[(int)OldSkill::SHIELD];
		skill[(int)Skill::LIGHT_ARMOR] = old_skill[(int)OldSkill::LIGHT_ARMOR];
		skill[(int)Skill::HEAVY_ARMOR] = old_skill[(int)OldSkill::HEAVY_ARMOR];
	}
	ReadFile(file, &gold, sizeof(gold), &tmp, NULL);
	ReadFile(file, &invisible, sizeof(invisible), &tmp, NULL);
	ReadFile(file, &in_building, sizeof(in_building), &tmp, NULL);
	ReadFile(file, &to_remove, sizeof(to_remove), &tmp, NULL);
	ReadFile(file, &temporary, sizeof(temporary), &tmp, NULL);
	ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_20)
	{
		// w nowszych wersjach nie ma tej zmiennej, alkohol dzia�a inaczej
		bool niesmierc;
		ReadFile(file, &niesmierc, sizeof(niesmierc), &tmp, NULL);
	}
	ReadFile(file, &assist, sizeof(assist), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_10)
	{
		bool old_auto_talk;
		ReadFile(file, &old_auto_talk, sizeof(old_auto_talk), &tmp, NULL);
		auto_talk = (old_auto_talk ? 1 : 0);
	}
	else
	{
		ReadFile(file, &auto_talk, sizeof(auto_talk), &tmp, NULL);
		if(auto_talk == 2)
			ReadFile(file, &auto_talk_timer, sizeof(auto_talk_timer), &tmp, NULL);
	}
	ReadFile(file, &dont_attack, sizeof(dont_attack), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		attack_team = false;
	else
		ReadFile(file, &attack_team, sizeof(attack_team), &tmp, NULL);
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);
	int unit_event_handler_quest_refid;
	ReadFile(file, &unit_event_handler_quest_refid, sizeof(unit_event_handler_quest_refid), &tmp, NULL);
	if(unit_event_handler_quest_refid == -2)
		event_handler = Game::_game;
	else if(unit_event_handler_quest_refid == -1)
		event_handler = NULL;
	else
	{
		event_handler = (UnitEventHandler*)unit_event_handler_quest_refid;
		Game::Get().load_unit_handler.push_back(this);
	}
	CalculateLoad();
	if(LOAD_VERSION < V_0_2_10)
	{
		weight = 0;
		if(can_sort)
		{
			RemoveNullItems(items);
			RecalculateWeight();
			SortItems(items);
		}
	}
	else
	{
		if(can_sort && LOAD_VERSION < V_0_2_20)
			SortItems(items);
		ReadFile(file, &weight, sizeof(weight), &tmp, NULL);
		RecalculateWeight();
	}
	if(LOAD_VERSION < V_0_2_10)
		guard_target = NULL;
	else
	{
		int guard_refid;
		ReadFile(file, &guard_refid, sizeof(guard_refid), &tmp, NULL);
		if(guard_refid == -1)
			guard_target = NULL;
		else
		{
			guard_target = (Unit*)guard_refid;
			Game::Get().load_unit_refid.push_back(&guard_target);
		}
	}

	bubble = NULL; // ustawianie przy wczytaniu SpeechBubble
	changed = false;
	busy = Busy_No;
	visual_pos = pos;

	byte b;
	ReadFile(file, &b, sizeof(b), &tmp, NULL);
	if(b == 1)
	{
		human_data = new Human;
		human_data->Load(file);
	}
	else
		human_data = NULL;

	if(local)
	{
		if(IS_SET(data->flagi, F_CZLOWIEK))
			ani = new AnimeshInstance(Game::Get().aHumanBase);
		else
			ani = new AnimeshInstance(data->ani);
		ani->Load(file);
		ani->ptr = this;
		ReadFile(file, &animacja, sizeof(animacja), &tmp, NULL);
		ReadFile(file, &animacja2, sizeof(animacja2), &tmp, NULL);

		ReadFile(file, &prev_pos, sizeof(prev_pos), &tmp, NULL);
		ReadFile(file, &speed, sizeof(speed), &tmp, NULL);
		ReadFile(file, &prev_speed, sizeof(prev_speed), &tmp, NULL);
		ReadFile(file, &etap_animacji, sizeof(etap_animacji), &tmp, NULL);
		ReadFile(file, &attack_id, sizeof(attack_id), &tmp, NULL);
		ReadFile(file, &action, sizeof(action), &tmp, NULL);
		if(LOAD_VERSION < V_0_2_20 && action >= A_EAT)
			action = ACTION(action+1);
		ReadFile(file, &wyjeta, sizeof(wyjeta), &tmp, NULL);
		ReadFile(file, &chowana, sizeof(chowana), &tmp, NULL);
		ReadFile(file, &stan_broni, sizeof(stan_broni), &tmp, NULL);
		ReadFile(file, &trafil, sizeof(trafil), &tmp, NULL);
		ReadFile(file, &hurt_timer, sizeof(hurt_timer), &tmp, NULL);
		ReadFile(file, &target_pos, sizeof(target_pos), &tmp, NULL);
		ReadFile(file, &target_pos2, sizeof(target_pos2), &tmp, NULL);
		ReadFile(file, &talking, sizeof(talking), &tmp, NULL);
		ReadFile(file, &talk_timer, sizeof(talk_timer), &tmp, NULL);
		ReadFile(file, &attack_power, sizeof(attack_power), &tmp, NULL);
		if(LOAD_VERSION < V_0_2_10)
			attack_power += 1.f;
		ReadFile(file, &atak_w_biegu, sizeof(atak_w_biegu), &tmp, NULL);
		ReadFile(file, &timer, sizeof(timer), &tmp, NULL);
		if(LOAD_VERSION >= V_0_2_20)
		{
			ReadFile(file, &alcohol, sizeof(alcohol), &tmp, NULL);
			ReadFile(file, &raise_timer, sizeof(raise_timer), &tmp, NULL);
		}
		else
		{
			alcohol = 0.f;
			if(action == A_ANIMATION2 && etap_animacji > 0)
				++etap_animacji;
			raise_timer = timer;
		}

		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, NULL);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, NULL);
			used_item = ::FindItem(BUF);
			if(LOAD_VERSION < V_0_2_10)
				used_item_is_team = true;
			else
				ReadFile(file, &used_item_is_team, sizeof(used_item_is_team), &tmp, NULL);
		}
		else
			used_item = NULL;

		int refi;
		ReadFile(file, &refi, sizeof(refi), &tmp, NULL);
		if(refi == -1)
			useable = NULL;
		else
			Useable::AddRequest(&useable, refi, this);

		if(action == A_SHOOT)
		{
			vector<AnimeshInstance*>& bow_instances = Game::Get().bow_instances;
			if(bow_instances.empty())
				bow_instance = new AnimeshInstance(GetBow().ani);
			else
			{
				bow_instance = bow_instances.back();
				bow_instances.pop_back();
				bow_instance->ani = GetBow().ani;
			}
			bow_instance->Play(&bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND, 0);
			bow_instance->groups[0].speed = ani->groups[1].speed;
			bow_instance->groups[0].time = ani->groups[1].time;
		}

		if(LOAD_VERSION < V_0_2_10)
			last_bash = 0.f;
		else
			ReadFile(file, &last_bash, sizeof(last_bash), &tmp, NULL);
	}
	else
	{
		ani = NULL;
		ai = NULL;
		useable = NULL;
		used_item = NULL;
		stan_broni = BRON_SCHOWANA;
		wyjeta = B_BRAK;
		chowana = B_BRAK;
		frozen = 0;
		talking = false;
		animacja = animacja2 = ANI_STOI;
		action = A_NONE;
		atak_w_biegu = false;
		hurt_timer = 0.f;
		speed = prev_speed = 0.f;
		alcohol = 0.f;
	}

	// efekty
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	effects.resize(ile);
	if(ile)
		ReadFile(file, &effects[0], sizeof(Effect)*ile, &tmp, NULL);

	ReadFile(file, &b, sizeof(b), &tmp, NULL);
	if(b == 1)
	{
		player = new PlayerController;
		player->unit = this;
		player->Load(file);
	}
	else
		player = NULL;

	if(local && human_data)
		human_data->ApplyScale(ani->ani);

	if(IS_SET(data->flagi, F_BOHATER))
	{
		hero = new HeroData;
		hero->unit = this;
		hero->Load(file);
	}
	else
		hero = NULL;
	
	in_arena = -1;
	ai = NULL;
	look_target = NULL;
	interp = NULL;
	frozen = 0;

	// fizyka
	if(local)
	{
		btCapsuleShape* caps = new btCapsuleShape(GetUnitRadius(), max(MIN_H, GetUnitHeight()));
		cobj = new btCollisionObject;
		cobj->setCollisionShape(caps);
		cobj->setUserPointer(this);
		cobj->setCollisionFlags(CG_UNIT);
		Game::Get().phy_world->addCollisionObject(cobj);
		Game::Get().UpdateUnitPhysics(this, IsAlive() ? pos : VEC3(1000,1000,1000));
	}
	else
		cobj = NULL;

	// konwersja ekwipunku z V0
	if(LOAD_VERSION == V_0_2 && IS_SET(data->flagi2, F2_AKTUALIZUJ_PRZEDMIOTY_V0))
	{
		ClearInventory();
		Game::Get().ParseItemScript(*this, data->items);
	}

	// zabezpieczenie
	if(((stan_broni == BRON_WYJETA || stan_broni == BRON_WYJMUJE) && wyjeta == B_BRAK) ||
		(stan_broni == BRON_CHOWA && chowana == B_BRAK))
	{
		stan_broni = BRON_SCHOWANA;
		wyjeta = B_BRAK;
		chowana = B_BRAK;
		WARN(Format("Unit '%s' had broken weapon state.", data->id));
	}

	// calculate new attributes
	if(LOAD_VERSION < V_DEVEL)
	{
		UnitData* ud = data;
		if(IsPlayer())
			level = CalculateLevel();

		StatProfile& profile = ud->GetStatProfile();
		profile.SetForNew(level, attrib, skill);
	}
}

//=================================================================================================
bool Unit::FindEffect(ConsumeEffect effect, float* value)
{
	Effect* top = NULL;
	float topv = 0.f;

	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->effect == effect && it->power > topv)
		{
			top = &*it;
			topv = it->power;
		}
	}

	if(top)
	{
		if(value)
			*value = topv;
		return true;
	}
	else
		return false;
}

//=================================================================================================
// szuka miksturek leczniczych w ekwipunku, zwraca -1 je�li nie odnaleziono
int Unit::FindHealingPotion() const
{
	float missing = hpmax - hp, heal=0, heal2=inf();
	int id = -1, id2 = -1, index = 0;

	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item && it->item->type == IT_CONSUMEABLE)
		{
			const Consumeable& pot = it->item->ToConsumeable();
			if(!pot.IsHealingPotion())
				continue;

			if(pot.power <= missing)
			{
				if(pot.power > heal)
				{
					heal = pot.power;
					id = index;
				}
			}
			else
			{
				if(pot.power < heal2)
				{
					heal2 = pot.power;
					id2 = index;
				}
			}
		}
	}

	if(id != -1)
	{
		if(id2 != -1)
		{
			if(missing - heal < heal2 - missing)
				return id;
			else
				return id2;
		}
		else
			return id;
	}
	else
		return id2;
}

//=================================================================================================
void Unit::ReequipItems()
{
	bool zmiany = false;
	int index = 0;
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(!it->item)
			continue;

		if(it->item->type == IT_GOLD)
		{
			gold += it->count;
			it->item = NULL;
			zmiany = true;
		}
		else if(it->item->IsWearableByHuman())
		{
			ITEM_SLOT slot = ItemTypeToSlot(it->item->type);
			assert(slot != SLOT_INVALID);

			if(slots[slot])
			{
				if(slots[slot]->value < it->item->value)
				{
					const Item* item = slots[slot];
					slots[slot] = it->item;
					it->item = item;
				}
			}
			else
			{
				slots[slot] = it->item;
				it->item = NULL;
				zmiany = true;
			}
		}
	}
	if(zmiany)
	{
		RemoveNullItems(items);
		SortItems(items);
	}

	// je�li nie ma broni, daj mu jak�� kiepsk�
	if(type == HUMANOID && !HaveWeapon() && !HaveBow())
	{
		cstring base_weapons[4] = {
			"dagger_short",
			"axe_small",
			"blunt_club",
			"sword_long"
		};

		AddItemAndEquipIfNone(::FindItem(base_weapons[rand2()%4]));
	}
}

//=================================================================================================
void Unit::RemoveQuestItem(int _refid)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item && IS_SET(it->item->flags, ITEM_QUEST) && it->item->refid == _refid)
		{
			weight -= it->item->weight;
			items.erase(it);
			return;
		}
	}

	assert(0 && "Nie znalaziono questowego przedmiotu do usuni�cia!");
}

//=================================================================================================
bool Unit::HaveItem(const Item* item)
{
	for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->item == item)
			return true;
	}
	return false;
}

//=================================================================================================
float Unit::GetLevel(TrainWhat src)
{
	if(IsAI())
		return float(level);

	const int b_str = BIT((int)Attribute::STR);
	const int b_con = BIT((int)Attribute::CON);
	const int b_dex = BIT((int)Attribute::DEX);
	
	switch(src)
	{
	case Train_Hit:
		// gracz zaatakowa� innego gracza
		return player->CalculateLevel(b_str | b_dex, BIT((int)Skill::ONE_HANDED_WEAPON), USE_WEAPON);
	case Train_Shot:
		return player->CalculateLevel(b_str | b_dex, BIT((int)Skill::BOW), USE_BOW);
	case Train_Block:
	case Train_Bash:
		return player->CalculateLevel(b_str | b_dex, BIT((int)Skill::SHIELD), USE_SHIELD);
	case Train_Hurt:
		{
			int s = 0, u = 0;
			if(HaveArmor())
			{
				if(GetArmor().IsHeavy())
					s = BIT((int)Skill::HEAVY_ARMOR);
				else
					s = BIT((int)Skill::LIGHT_ARMOR);
				u = USE_ARMOR;
			}
			return player->CalculateLevel(b_str | b_con | b_dex, s, u);
		}
	default:
		assert(0);
		return 0.f;
	}
}

//=================================================================================================
float Unit::GetAttackSpeed(const Weapon* used_weapon) const
{
	const Weapon* wep;

	if(used_weapon)
		wep = used_weapon;
	else if(HaveWeapon())
		wep = &GetWeapon();
	else
		wep = NULL;

	/* co wp�ywa na szybko�� ataku?
	+ rodzaj broni
	+ zr�czno��
	+ umiej�tno�ci
	+ brakuj�ca si�a
	+ ud�wig */
	if(wep)
	{
		const WeaponTypeInfo& info = wep->GetInfo();

		float mod = 1.f + float(skill[(int)Skill::ONE_HANDED_WEAPON]) / 200 + CalculateDexterity()*info.dex_speed - GetAttackSpeedModFromStrength(*wep);

		if(IsPlayer())
			mod -= GetAttackSpeedModFromLoad();

		if(mod < 0.1f)
			mod = 0.1f;

		return GetWeapon().GetInfo().base_speed * mod;
	}
	else
		return 1.f + float(skill[(int)Skill::ONE_HANDED_WEAPON]) / 200 + CalculateDexterity()*0.001f;
}

//=================================================================================================
float Unit::GetBowAttackSpeed() const
{
	float mod = 0.8f + float(skill[(int)Skill::BOW]) / 200 + CalculateDexterity()*0.004f - GetAttackSpeedModFromStrength(GetBow());
	if(IsPlayer())
		mod -= GetAttackSpeedModFromLoad();
	if(mod < 0.25f)
		mod = 0.25f;
	return mod;
}

//=================================================================================================
void Unit::MakeItemsTeam(bool team)
{
	if(team)
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			it->team_count = it->count;
	}
	else
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
			it->team_count = 0;
	}
}

//=================================================================================================
void Unit::HealPoison()
{
	if(hp < 1.f)
		hp = 1.f;

	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == E_POISON)
			_to_remove.push_back(index);
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
void Unit::RemovePoison()
{
	uint index = 0;
	for(vector<Effect>::iterator it = effects.begin(), end = effects.end(); it != end; ++it, ++index)
	{
		if(it->effect == E_POISON)
			_to_remove.push_back(index);
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == effects.size()-1)
			effects.pop_back();
		else
		{
			std::iter_swap(effects.begin()+index, effects.end()-1);
			effects.pop_back();
		}
	}
}

//=================================================================================================
int Unit::FindItem(const Item* item, int refid) const
{
	assert(item);

	for(int i=0; i<SLOT_MAX; ++i)
	{
		if(slots[i] == item)
		{
			if(refid == -1)
				return SlotToIIndex(ITEM_SLOT(i));
			else if(IS_SET(slots[i]->flags, ITEM_QUEST))
			{
				if(slots[i]->refid == refid)
					return SlotToIIndex(ITEM_SLOT(i));
			}
		}
	}

	int index = 0;
	for(vector<ItemSlot>::const_iterator it = items.begin(), end = items.end(); it != end; ++it, ++index)
	{
		if(it->item == item)
		{
			if(refid == -1)
				return index;
			else if(IS_SET(it->item->flags, ITEM_QUEST))
			{
				if(it->item->refid == refid)
					return index;
			}
		}
	}

	return INVALID_IINDEX;
}

//=================================================================================================
void Unit::ClearInventory()
{
	items.clear();
	for(int i=0; i<SLOT_MAX; ++i)
		slots[i] = NULL;
	weight = 0;
	wyjeta = B_BRAK;
	chowana = B_BRAK;
	stan_broni = BRON_SCHOWANA;
	if(player)
		player->ostatnia = B_BRAK;
}

//=================================================================================================
cstring NAMES::point_weapon = "bron";
cstring NAMES::point_hidden_weapon = "schowana";
cstring NAMES::point_shield = "tarcza";
cstring NAMES::point_shield_hidden = "tarcza_plecy";
cstring NAMES::point_bow = "luk";
cstring NAMES::point_cast = "castpoint";
cstring NAMES::points[] = {
	"bron",
	"schowana",
	"tarcza",
	"tarcza_plecy",
	"luk"
};
uint NAMES::n_points = countof(NAMES::points);

//=================================================================================================
cstring NAMES::ani_take_weapon = "wyjmuje";
cstring NAMES::ani_take_weapon_no_shield = "wyjmuje_bez_tarczy";
cstring NAMES::ani_take_bow = "wyjmij_luk";
cstring NAMES::ani_move = "idzie";
cstring NAMES::ani_run = "biegnie";
cstring NAMES::ani_left = "w_lewo";
cstring NAMES::ani_right = "w_prawo";
cstring NAMES::ani_stand = "stoi";
cstring NAMES::ani_battle = "bojowy";
cstring NAMES::ani_battle_bow = "bojowy_luk";
cstring NAMES::ani_die = "umiera";
cstring NAMES::ani_hurt = "trafiony";
cstring NAMES::ani_shoot = "atak_luk";
cstring NAMES::ani_block = "blok";
cstring NAMES::ani_bash = "bash";
cstring NAMES::ani_base[] = {
	"idzie",
	"w_lewo",
	"w_prawo",
	"stoi",
	"bojowy",
	"umiera"
};
cstring NAMES::ani_humanoid[] = {
	"wyjmuje",
	"wyjmij_luk",
	"bojowy_luk",
	"atak_luk",
	"blok",
	"bash"
};
cstring NAMES::ani_attacks[] = {
	"atak1",
	"atak2",
	"atak3",
	"atak4",
	"atak5",
	"atak6"
};
uint NAMES::n_ani_base = countof(NAMES::ani_base);
uint NAMES::n_ani_humanoid = countof(NAMES::ani_humanoid);
int NAMES::max_attacks = countof(ani_attacks);

//=================================================================================================
VEC3 Unit::GetEyePos() const
{
	const Animesh::Point& point = *ani->ani->GetPoint("oczy");
	MATRIX matBone = point.mat * ani->mat_bones[point.bone];

	MATRIX matPos, matRot;
	D3DXMatrixTranslation(&matPos, pos);
	D3DXMatrixRotationY(&matRot, rot);

	matBone = matBone * (matRot * matPos);

	VEC3 pos(0,0,0), out;
	D3DXVec3TransformCoord(&out, &pos, &matBone);

	return out;
}

//=================================================================================================
float Unit::GetBlockSpeed() const
{
// 	const Shield& s = GetShield();
// 	float block_speed = 0.33f;
// 	if(attrib[A_STR] < s.sila)
// 		block_speed
	return 0.1f;
}

//=================================================================================================
float Unit::CalculateArmorDefense(const Armor* in_armor)
{
	const Armor* _armor = (in_armor ? in_armor : GetArmorPtr());
	if(!_armor)
		return 0.f;

	// pancerz daje tyle ile bazowo * skill
	Skill sk;
	if(!OR2_EQ(_armor->armor_type, A_HEAVY, A_MONSTER_HEAVY))
		sk = Skill::LIGHT_ARMOR;
	else
		sk = Skill::HEAVY_ARMOR;

	float skill_val;
	int str = attrib[(int)Attribute::STR];
	if(str >= _armor->sila)
		skill_val = float(skill[(int)sk]);
	else
		skill_val = float(skill[(int)sk]) * str / _armor->sila;

	return (skill_val/100+1)*_armor->def;
}

//=================================================================================================
float Unit::CalculateDexterityDefense(const Armor* in_armor)
{
	float load = GetLoad();
	float mod = 1.f;

	const Armor* _armor = (in_armor ? in_armor : GetArmorPtr());

	// pancerz
	if(_armor)
	{
		// lekki pancerz
		// mod = 1.f
		// �redni = 0.5f
		// ci�ki = 0.2f
		if(_armor->IsHeavy())
			mod = 0.2f;
	}
	else
		mod = 2.f;

	// zr�czno��
	if(load < 1.f)
	{
		float dex = CalculateDexterity();
		if(dex > 50.f)
			return ((dex - 50.f) / 3) * (1.f-load) * mod;
	}

	return 0.f;
}

//=================================================================================================
float Unit::CalculateBaseDefense()
{
	return float(data->def_bonus + attrib[(int)Attribute::CON]/10);
}

//=================================================================================================
bool Unit::IsBetterItem(const Item* item) const
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return IsBetterWeapon(item->ToWeapon());
	case IT_ARMOR:
		return IsBetterArmor(item->ToArmor());
	case IT_SHIELD:
		if(HaveShield())
			return item->value > GetShield().value;
		else
			return true;
	case IT_BOW:
		if(HaveBow())
			return item->value > GetBow().value;
		else
			return true;
	default:
		return false;
	}
}

//=================================================================================================
int Unit::CountItem(const Item* item)
{
	assert(item);

	if(item->IsStackable())
	{
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
			{
				// zak�ada �e mo�na mie� tylko jeden stack takich przedmiot�w
				return it->count;
			}
		}
		return 0;
	}
	else
	{
		int ile = 0;
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(it->item == item)
				++ile;
		}
		return ile;
	}
}

//=================================================================================================
const Item* Unit::GetIIndexItem( int i_index ) const
{
	if(i_index >= 0)
	{
		if(i_index < (int)items.size())
			return items[i_index].item;
		else
			return NULL;
	}
	else
	{
		ITEM_SLOT slot_type = IIndexToSlot(i_index);
		if(slot_type < SLOT_MAX)
			return slots[slot_type];
		else
			return NULL;
	}
}

//=================================================================================================
Animesh::Animation* Unit::GetTakeWeaponAnimation(bool melee) const
{
	if(melee)
	{
		if(HaveShield())
			return ani->ani->GetAnimation(NAMES::ani_take_weapon);
		else
		{
			Animesh::Animation* anim = ani->ani->GetAnimation(NAMES::ani_take_weapon_no_shield);
			if(!anim)
			{
				// brak animacja wyci�gania broni bez tarczy, u�yj zwyk�ej
				return ani->ani->GetAnimation(NAMES::ani_take_weapon);
			}
			else
				return anim;
		}
	}
	else
		return ani->ani->GetAnimation(NAMES::ani_take_bow);
}

//=================================================================================================
float Unit::CalculateMagicResistance() const
{
	float mres = 1.f;
	if(IS_SET(data->flagi2, F2_ODPORNOSC_NA_MAGIE_25))
		mres = 0.75f;
	else if(IS_SET(data->flagi2, F2_ODPORNOSC_NA_MAGIE_50))
		mres = 0.5f;
	if(HaveArmor())
	{
		const Armor& a = GetArmor();
		if(IS_SET(a.flags, ITEM_MAGIC_RESISTANCE_10))
			mres *= 0.9f;
		else if(IS_SET(a.flags, ITEM_MAGIC_RESISTANCE_25))
			mres *= 0.75f;
	}
	if(HaveShield())
	{
		const Shield& s = GetShield();
		if(IS_SET(s.flags, ITEM_MAGIC_RESISTANCE_10))
			mres *= 0.9f;
		else if(IS_SET(s.flags, ITEM_MAGIC_RESISTANCE_25))
			mres *= 0.75f;
	}
	if(HaveEffect(E_ANTIMAGIC))
		mres *= 0.5f;
	return mres;
}

//=================================================================================================
int Unit::CalculateMagicPower() const
{
	int mlvl = 0;
	if(HaveArmor())
		mlvl += GetArmor().GetMagicPower();
	if(stan_broni == BRON_WYJETA)
	{
		if(wyjeta == B_JEDNORECZNA)
			mlvl += GetWeapon().GetMagicPower();
		else
			mlvl += GetBow().GetMagicPower();
	}
	return mlvl;
}

//=================================================================================================
bool Unit::HaveEffect(ConsumeEffect e) const
{
	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		if(it->effect == e)
			return true;
	}
	return false;
}

//=================================================================================================
int Unit::GetBuffs() const
{
	int b = 0;

	for(vector<Effect>::const_iterator it = effects.begin(), end = effects.end(); it != end; ++it)
	{
		switch(it->effect)
		{
		case E_POISON:
			b |= BUFF_POISON;
			break;
		case E_ALCOHOL:
			b |= BUFF_ALCOHOL;
			break;
		case E_REGENERATE:
			b |= BUFF_REGENERATION;
			break;
		case E_FOOD:
			b |= BUFF_FOOD;
			break;
		case E_NATURAL:
			b |= BUFF_NATURAL;
			break;
		}
	}
	
	if(alcohol/hpmax >= 0.1f)
		b |= BUFF_ALCOHOL;

	return b;
}

int Unit::CalculateLevel()
{
	if(player)
		return CalculateLevel(player->clas);
	else
		return level;
}

int Unit::CalculateLevel(Class clas)
{
	UnitData* ud = g_classes[(int)clas].unit_data;

	float tlevel = 0.f;
	float weight_sum = 0.f;
	StatProfile& profile = ud->GetStatProfile();

	// calculate player level based on attributes and skills that are important for that class
	for(int i = 0; i < (int)Attribute::MAX; ++i)
	{
		int base = profile.attrib[i] - 50;
		if(base > 0 && attrib[i] > 0)
		{
			int dif = attrib[i] - profile.attrib[i], weight;
			float mod = AttributeInfo::GetModifier(base, weight);
			tlevel += (float(dif) / mod) * weight * 5;
			weight_sum += weight;
		}
	}

	for(int i = 0; i < (int)Skill::MAX; ++i)
	{
		int base = profile.skill[i];
		if(base > 0 && skill[i] > 0)
		{
			int dif = skill[i] - base, weight;
			float mod = SkillInfo::GetModifier(base, weight);
			tlevel += (float(dif) / mod) * weight * 5;
			weight_sum += weight;
		}
	}

	return (int)floor(tlevel / weight_sum);
}

int Unit::GetAttribute(Attribute a, int& base, int& base_start, StatState& state)
{
	StatProfile& profile = data->GetStatProfile();
	base_start = profile.attrib[(int)a];
	base = attrib[(int)a];
	state = StatState::NORMAL;
	return attrib[(int)a];
}

int Unit::GetSkill(Skill s, int& base, int& base_start, StatState& state)
{
	StatProfile& profile = data->GetStatProfile();
	base_start = profile.skill[(int)s];
	base = skill[(int)s];
	state = StatState::NORMAL;
	return skill[(int)s];
}