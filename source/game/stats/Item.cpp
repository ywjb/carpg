// przedmiot
#include "Pch.h"
#include "Base.h"
#include "Item.h"

ItemsMap g_items;
std::map<string, string> item_map;
vector<ItemList*> g_item_lists;
vector<LeveledItemList*> g_leveled_item_lists;
vector<Weapon*> g_weapons;
vector<Bow*> g_bows;
vector<Shield*> g_shields;
vector<Armor*> g_armors;
vector<Consumeable*> g_consumeables;
vector<OtherItem*> g_others;
vector<OtherItem*> g_artifacts;
vector<Stock*> stocks;

//-----------------------------------------------------------------------------
// adding new types here will require changes in CreatedCharacter::GetStartingItems
WeaponTypeInfo weapon_type_info[] = {
	NULL, 0.5f, 0.5f, 0.4f, 1.1f, 0.002f, Skill::SHORT_BLADE, // WT_SHORT
	NULL, 0.75f, 0.25f, 0.33f, 1.f, 0.0015f, Skill::LONG_BLADE, // WT_LONG
	NULL, 0.85f, 0.15f, 0.29f, 0.9f, 0.00075f, Skill::BLUNT, // WT_MACE
	NULL, 0.8f, 0.2f, 0.31f, 0.95f, 0.001f, Skill::AXE // WT_AXE
};

vector<const Item*> LeveledItemList::toadd;

//=================================================================================================
const Item* LeveledItemList::Get(int level) const
{
	int best_lvl = -1;

	for(const ItemEntryLevel& ie : items)
	{
		if(ie.level <= level && ie.level >= best_lvl)
		{
			if(ie.level > best_lvl)
			{
				toadd.clear();
				best_lvl = ie.level;
			}
			toadd.push_back(ie.item);
		}
	}

	if(!toadd.empty())
	{
		const Item* best = toadd[rand2() % toadd.size()];
		toadd.clear();
		return best;
	}

	return NULL;
}

//=================================================================================================
const Item* FindItem(cstring id, bool report, ItemListResult* lis)
{
	assert(id);

	// check for item list
	if(id[0] == '!')
	{
		ItemListResult result = FindItemList(id+1);
		if(result.lis == NULL)
			return NULL;

		if(result.is_leveled)
		{
			assert(lis);
			*lis = result;
			return NULL;
		}
		else
		{
			if(lis)
				*lis = result;
			return result.lis->Get();
		}
	}

	if(lis)
		lis->lis = NULL;

	// search item
	auto it = g_items.find(id);
	if(it != g_items.end())
		return it->second;

	// search item by old id
	auto it2 = item_map.find(id);
	if(it2 != item_map.end())
		return FindItem(it2->second.c_str(), report, lis);

	if(report)
		WARN(Format("Missing item '%s'.", id));

	return NULL;
}

//=================================================================================================
ItemListResult FindItemList(cstring name, bool report)
{
	assert(name);

	ItemListResult result;

	if(name[0] == '-')
	{
		result.mod = -(name[1] - '0');
		name = name + 2;
		assert(in_range(result.mod, -9, -1));
	}
	else
		result.mod = 0;

	for(ItemList* lis : g_item_lists)
	{
		if(lis->name == name)
		{
			result.lis = lis;
			result.is_leveled = false;
			return result;
		}
	}

	for(LeveledItemList* llis : g_leveled_item_lists)
	{
		if(llis->name == name)
		{
			result.llis = llis;
			result.is_leveled = true;
			return result;
		}
	}

	if(report)
		WARN(Format("Missing item list '%s'.", name));

	result.lis = NULL;
	return result;
}

//=================================================================================================
Item* CreateItemCopy(const Item* item)
{
	switch(item->type)
	{
	case IT_OTHER:
		{
			OtherItem* o = new OtherItem;
			const OtherItem& o2 = item->ToOther();
			o->ani = o2.ani;
			o->desc = o2.desc;
			o->flags = o2.flags;
			o->id = o2.id;
			o->mesh = o2.mesh;
			o->name = o2.name;
			o->other_type = o2.other_type;
			o->refid = o2.refid;
			o->tex = o2.tex;
			o->type = o2.type;
			o->value = o2.value;
			o->weight = o2.weight;
			return o;
		}
		break;
	case IT_WEAPON:
	case IT_BOW:
	case IT_SHIELD:
	case IT_ARMOR:
	case IT_CONSUMEABLE:
	case IT_GOLD:
	case IT_LETTER:
	default:
		// not implemented yet, YAGNI!
		assert(0);
		return NULL;
	}
}

//=================================================================================================
void Item::Validate(int& err)
{
	for(auto it : g_items)
	{
		const Item& item = *it.second;

		if(item.name.empty())
		{
			++err;
			ERROR(Format("Missing item '%s' name.", item.name.c_str()));
		}

		if(item.mesh.empty())
		{
			++err;
			ERROR(Format("Missing item '%s' mesh/texture.", item.name.c_str()));
		}
	}
}

//=================================================================================================
void SetItemsMap()
{
	auto& m = item_map;

	// old typo
	m["vs_emerals"] = "vs_emerald";

	// new armor names
	m["armor_leather"] = "al_leather";
	m["armor_studded"] = "al_studded";
	m["armor_chain_shirt"] = "al_chain_shirt";
	m["armor_mithril_shirt"] = "al_chain_shirt_mith";
	m["armor_dragonskin"] = "al_dragonskin";
	m["armor_unique2"] = "al_angelskin";
	m["armor_chainmail"] = "am_chainmail";
	m["armor_breastplate"] = "am_breastplate";
	m["armor_plate"] = "ah_plate";
	m["armor_crystal"] = "ah_crystal";
	m["armor_adamantine"] = "ah_plate_adam";
	m["armor_unique"] = "ah_black_armor";
	m["armor_blacksmith"] = "al_blacksmith";
	m["armor_innkeeper"] = "al_innkeeper";
	m["armor_goblin"] = "al_goblin";
	m["armor_orcish_leather"] = "al_orc";
	m["armor_orcish_chainmail"] = "am_orc";
	m["armor_orcish_shaman"] = "al_orc_shaman";
	m["armor_mage_1"] = "al_mage";
	m["armor_mage_2"] = "al_mage2";
	m["armor_mage_3"] = "al_mage3";
	m["armor_mage_4"] = "al_mage4";
	m["armor_necromancer"] = "al_necromancer";
	m["clothes"] = "al_clothes";
	m["clothes2"] = "al_clothes2";
	m["clothes3"] = "al_clothes2b";
	m["clothes4"] = "al_clothes3";
	m["clothes5"] = "al_clothes3b";

	// new consumeable names
	m["potion_smallnreg"] = "p_nreg";
	m["potion_bignreg"] = "p_nreg2";
	m["potion_smallheal"] = "p_hp";
	m["potion_mediumheal"] = "p_hp2";
	m["potion_bigheal"] = "p_hp3";
	m["potion_reg"] = "p_reg";
	m["potion_antidote"] = "p_antidote";
	m["potion_antimagic"] = "p_antimagic";
	m["potion_str"] = "p_str";
	m["potion_end"] = "p_end";
	m["potion_dex"] = "p_dex";
	m["potion_water"] = "p_water";
	m["potion_beer"] = "p_beer";
	m["potion_vodka"] = "p_vodka";
	m["potion_spirit"] = "p_spirit";
	m["bread"] = "f_bread";
	m["meat"] = "f_meat";
	m["raw_meat"] = "f_raw_meat";
	m["apple"] = "f_apple";
	m["cheese"] = "f_cheese";
	m["honeycomb"] = "f_honeycomb";
	m["rice"] = "f_rice";
	m["mushroom"] = "f_mushroom";
}

extern string g_system_dir;

enum KeywordGroup
{
	G_ITEM_TYPE,
	G_PROPERTY,
	G_WEAPON_TYPE,
	G_MATERIAL,
	G_DMG_TYPE,
	G_FLAGS,
	G_ARMOR_SKILL,
	G_ARMOR_UNIT_TYPE,
	G_CONSUMEABLE_TYPE,
	G_EFFECT,
	G_OTHER_TYPE,
	G_STOCK_KEYWORD
};

enum Property
{
	P_WEIGHT,
	P_VALUE,
	P_MESH,
	P_TEX,
	P_ATTACK,
	P_REQ_STR,
	P_TYPE,
	P_MATERIAL,
	P_DMG_TYPE,
	P_FLAGS,
	P_DEFENSE,
	P_MOBILITY,
	P_SKILL,
	P_TEX_OVERRIDE,
	P_EFFECT,
	P_POWER,
	P_TIME
};

enum StockKeyword
{
	SK_SET,
	SK_CITY,
	SK_ELSE,
	SK_CHANCE,
	SK_RANDOM
};

//=================================================================================================
int ReadFlags(Tokenizer& t, int group)
{
	int flags = 0;

	if(t.IsSymbol('{'))
	{
		t.Next();

		while(!t.IsSymbol('}'))
		{
			flags |= t.MustGetKeywordId(group);
			t.Next();
		}
	}
	else
		flags = t.MustGetKeywordId(group);

	return flags;
}

//=================================================================================================
bool LoadItem(Tokenizer& t)
{
	ITEM_TYPE type = (ITEM_TYPE)t.GetKeywordId(G_ITEM_TYPE);
	int req = BIT(P_WEIGHT) | BIT(P_VALUE) | BIT(P_MESH) | BIT(P_TEX) | BIT(P_FLAGS);
	Item* item;

	switch(type)
	{
	case IT_WEAPON:
		item = new Weapon;
		req |= BIT(P_ATTACK) | BIT(P_REQ_STR) | BIT(P_TYPE) | BIT(P_MATERIAL) | BIT(P_DMG_TYPE);
		break;
	case IT_BOW:
		item = new Bow;
		req |= BIT(P_ATTACK) | BIT(P_REQ_STR);
		break;
	case IT_SHIELD:
		item = new Shield;
		req |= BIT(P_DEFENSE) | BIT(P_REQ_STR) | BIT(P_MATERIAL);
		break;
	case IT_ARMOR:
		item = new Armor;
		req |= BIT(P_DEFENSE) | BIT(P_MOBILITY) | BIT(P_REQ_STR) | BIT(P_MATERIAL) | BIT(P_SKILL) | BIT(P_TYPE) | BIT(P_TEX_OVERRIDE);
		break;
	case IT_CONSUMEABLE:
		item = new Consumeable;
		req |= BIT(P_EFFECT) | BIT(P_TIME) | BIT(P_POWER) | BIT(P_TYPE);
		break;
	case IT_OTHER:
	default:
		item = new OtherItem;
		req |= BIT(P_TYPE);
		break;
	}

	try
	{
		// id
		t.Next();
		item->id = t.MustGetItem();
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		// properties
		while(!t.IsSymbol('}'))
		{
			Property prop = (Property)t.MustGetKeywordId(G_PROPERTY);
			if(!IS_SET(req, BIT(prop)))
			{
				ERROR(Format("Item '%s' can't have property '%s'.", item->id.c_str(), t.GetTokenString().c_str()));
				delete item;
				return false;
			}

			t.Next();

			switch(prop)
			{
			case P_WEIGHT:
				item->weight = int(t.MustGetNumberFloat() * 10);
				if(item->weight < 0)
				{
					ERROR(Format("Item '%s' have negative weight %g.", item->id.c_str(), item->GetWeight()));
					delete item;
					return false;
				}
				break;
			case P_VALUE:
				item->value = t.MustGetInt();
				if(item->value < 0)
				{
					ERROR(Format("Item '%s' have negative value %d.", item->id.c_str(), item->value));
					return false;
				}
				break;
			case P_MESH:
				if(IS_SET(item->flags, ITEM_TEX_ONLY))
				{
					ERROR(Format("Item '%s' can't have mesh, it is texture only item.", item->id.c_str()));
					delete item;
					return false;
				}
				item->mesh = t.MustGetString();
				if(item->mesh.empty())
				{
					ERROR(Format("Item '%s' have empty mesh.", item->id.c_str()));
					delete item;
					return false;
				}
				break;
			case P_TEX:
				if(!item->mesh.empty() && !IS_SET(item->flags, ITEM_TEX_ONLY))
				{
					ERROR(Format("Item '%s' can't be texture only item, it have mesh.", item->id.c_str()));
					delete item;
					return false;
				}
				item->mesh = t.MustGetString();
				if(item->mesh.empty())
				{
					ERROR(Format("Item '%s' have empty texture name.", item->id.c_str()));
					delete item;
					return false;
				}
				item->flags |= ITEM_TEX_ONLY;
				break;
			case P_ATTACK:
				{
					int attack = t.MustGetInt();
					if(attack < 0)
					{
						ERROR(Format("Item '%s' have negative attack %d.", item->id.c_str(), attack));
						delete item;
						return false;
					}
					if(item->type == IT_WEAPON)
						item->ToWeapon().dmg = attack;
					else
						item->ToBow().dmg = attack;
				}
				break;
			case P_REQ_STR:
				{
					int req_str = t.MustGetInt();
					if(req_str < 0)
					{
						ERROR(Format("Item '%s' have negative required strength %d.", item->id.c_str(), req_str));
						delete item;
						return false;
					}
					switch(item->type)
					{
					case IT_WEAPON:
						item->ToWeapon().req_str = req_str;
						break;
					case IT_BOW:
						item->ToBow().req_str = req_str;
						break;
					case IT_SHIELD:
						item->ToShield().req_str = req_str;
						break;
					case IT_ARMOR:
						item->ToArmor().req_str = req_str;
						break;
					}
				}
				break;
			case P_TYPE:
				switch(item->type)
				{
				case IT_WEAPON:
					item->ToWeapon().weapon_type = (WEAPON_TYPE)t.MustGetKeywordId(G_WEAPON_TYPE);
					break;
				case IT_ARMOR:
					item->ToArmor().armor_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_UNIT_TYPE);
					break;
				case IT_CONSUMEABLE:
					item->ToConsumeable().cons_type = (ConsumeableType)t.MustGetKeywordId(G_CONSUMEABLE_TYPE);
					break;
				case IT_OTHER:
					item->ToOther().other_type = (OtherType)t.MustGetKeywordId(G_OTHER_TYPE);
					break;
				}
				break;
			case P_MATERIAL:
				{
					MATERIAL_TYPE mat = (MATERIAL_TYPE)t.MustGetKeywordId(G_MATERIAL);
					switch(item->type)
					{
					case IT_WEAPON:
						item->ToWeapon().material = mat;
						break;
					case IT_SHIELD:
						item->ToShield().material = mat;
						break;
					case IT_ARMOR:
						item->ToArmor().material = mat;
						break;
					}
				}
				break;
			case P_DMG_TYPE:
				item->ToWeapon().dmg_type = ReadFlags(t, G_DMG_TYPE);
				if(item->ToWeapon().dmg_type == 0)
				{
					ERROR(Format("Weapon '%s' have empty damage type.", item->id.c_str()));
					delete item;
					return false;
				}
				break;
			case P_FLAGS:
				{
					int old_flags = (item->flags & ITEM_TEX_ONLY);
					item->flags |= ReadFlags(t, G_FLAGS) | old_flags;
				}
				break;
			case P_DEFENSE:
				{
					int def = t.MustGetInt();
					if(def < 0)
					{
						ERROR(Format("Item '%s' have negative defense %d.", item->id.c_str(), def));
						delete item;
						return false;
					}
					if(item->type == IT_SHIELD)
						item->ToShield().def = def;
					else
						item->ToArmor().def = def;
				}
				break;
			case P_MOBILITY:
				{
					int mob = t.MustGetInt();
					if(mob < 0)
					{
						ERROR(Format("Item '%s' have negative mobility %d.", item->id.c_str(), mob));
						delete item;
						return false;
					}
					item->ToArmor().mobility = mob;
				}
				break;
			case P_SKILL:
				item->ToArmor().skill = (Skill)t.MustGetKeywordId(G_ARMOR_SKILL);
				break;
			case P_TEX_OVERRIDE:
				{
					vector<TexId>& tex_o = item->ToArmor().tex_override;
					if(t.IsSymbol('{'))
					{
						t.Next();
						while(!t.IsSymbol('}'))
						{
							tex_o.push_back(TexId(t.MustGetString().c_str()));
							t.Next();
						}
						if(tex_o.empty())
						{
							ERROR(Format("Item '%s' have empty texture overrides.", item->id.c_str()));
							delete item;
							return false;
						}
					}
					else
						tex_o.push_back(TexId(t.MustGetString().c_str()));
				}
				break;
			case P_EFFECT:
				item->ToConsumeable().effect = (ConsumeEffect)t.MustGetKeywordId(G_EFFECT);
				break;
			case P_POWER:
				{
					float power = t.MustGetNumberFloat();
					if(power < 0.f)
					{
						ERROR(Format("Item '%s' have negative power %g.", item->id.c_str(), power));
						delete item;
						return false;
					}
					item->ToConsumeable().power = power;
				}
				break;
			case P_TIME:
				{
					float time = t.MustGetNumberFloat();
					if(time < 0.f)
					{
						ERROR(Format("Item '%s' have negative time %g.", item->id.c_str(), time));
						delete item;
						return false;
					}
					item->ToConsumeable().time = time;
				}
				break;
			default:
				assert(0);
				break;
			}

			t.Next();
		}

		if(item->mesh.empty())
		{
			ERROR(Format("Item '%s' don't have mesh/texture.", item->id.c_str()));
			delete item;
			return false;
		}

		cstring key = item->id.c_str();

		ItemsMap::iterator it = g_items.lower_bound(key);
		if(it != g_items.end() && !(g_items.key_comp()(key, it->first)))
		{
			ERROR(Format("Item '%s' already exists.", key));
			delete item;
			return false;
		}
		else
			g_items.insert(it, ItemsMap::value_type(key, item));

		switch(item->type)
		{
		case IT_WEAPON:
			g_weapons.push_back((Weapon*)item);
			break;
		case IT_BOW:
			g_bows.push_back((Bow*)item);
			break;
		case IT_SHIELD:
			g_shields.push_back((Shield*)item);
			break;
		case IT_ARMOR:
			g_armors.push_back((Armor*)item);
			break;
		case IT_CONSUMEABLE:
			g_consumeables.push_back((Consumeable*)item);
			break;
		case IT_OTHER:
			g_others.push_back((OtherItem*)item);
			if(item->ToOther().other_type == Artifact)
				g_artifacts.push_back((OtherItem*)item);
			break;
		}

		return true;
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to parse item \"%s\": %s", item->id.c_str(), err));
		delete item;
		return false;
	}
}

//=================================================================================================
bool LoadItemList(Tokenizer& t)
{
	ItemList* lis = new ItemList;
	ItemListResult used_list;
	const Item* item;

	try
	{
		// name
		t.Next();
		lis->name = t.MustGetItemKeyword();
		t.Next();
		
		// {
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			item = FindItem(t.MustGetItemKeyword().c_str(), false, &used_list);
			if(used_list.lis != NULL)
			{
				ERROR(Format("Item list \"%s\" have item list \"%s\" inside.", lis->name.c_str(), used_list.GetName()));
				delete lis;
				return false;
			}
			if(!item)
			{
				ERROR(Format("Item list \"%s\" have missing item \"%s\".", lis->name.c_str(), t.GetTokenString().c_str()));
				delete lis;
				return false;
			}

			lis->items.push_back(item);
			t.Next();
		}

		g_item_lists.push_back(lis);

		return true;
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to parse item list \"%s\": %s", lis->name.c_str(), err));
		delete lis;
		return false;
	}
}

//=================================================================================================
bool LoadLeveledItemList(Tokenizer& t)
{
	LeveledItemList* lis = new LeveledItemList;
	ItemListResult used_list;
	const Item* item;
	int level;

	try
	{
		// name
		t.Next();
		lis->name = t.MustGetItemKeyword();
		t.Next();

		// {
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			item = FindItem(t.MustGetItemKeyword().c_str(), false, &used_list);
			if(used_list.lis != NULL)
			{
				ERROR(Format("Leveled item list \"%s\" have item list \"%s\" inside.", lis->name.c_str(), used_list.GetName()));
				delete lis;
				return false;
			}
			if(!item)
			{
				ERROR(Format("Leveled item list \"%s\" have missing item \"%s\".", lis->name.c_str(), t.GetTokenString().c_str()));
				delete lis;
				return false;
			}

			t.Next();
			level = t.MustGetInt();
			if(level < 0)
			{
				ERROR(Format("Leveled item list \"%s\" have negative required level %d for item \"%s\".", lis->name.c_str(),
					item->id.c_str(), level));
				delete lis;
				return false;
			}

			lis->items.push_back({ item, level });
			t.Next();
		}

		g_leveled_item_lists.push_back(lis);

		return true;
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to parse leveled item list \"%s\": %s", lis->name.c_str(), err));
		delete lis;
		return false;
	}
}

//=================================================================================================
bool LoadStock(Tokenizer& t)
{
	Stock* stock = new Stock;
	bool in_set = false, in_city = false, in_city_else;
	ItemListResult used_list;

	try
	{
		// id
		t.Next();
		stock->id = t.MustGetItemKeyword();
		t.Next();
		for(Stock* s : stocks)
		{
			if(s->id == stock->id)
			{
				ERROR(Format("Stock list \"%s\" already exists.", stock->id.c_str()));
				delete stock;
				return false;
			}
		}

		// {
		t.AssertSymbol('{');
		t.Next();

		while(true)
		{
			if(t.IsSymbol('}'))
			{
				if(in_city)
				{
					t.Next();
					if(!in_city_else && t.IsKeyword(SK_ELSE, G_STOCK_KEYWORD))
					{
						in_city_else = true;
						stock->code.push_back(SE_NOT_CITY);
						t.Next();
						t.AssertSymbol('{');
						t.Next();
					}
					else
					{
						in_city = false;
						stock->code.push_back(SE_ANY_CITY);
					}
				}
				else if(in_set)
				{
					in_set = false;
					stock->code.push_back(SE_END_SET);
					t.Next();
				}
				else
					break;
			}
			else if(t.IsKeywordGroup(G_STOCK_KEYWORD))
			{
				StockKeyword k = (StockKeyword)t.GetKeywordId(G_STOCK_KEYWORD);

				switch(k)
				{
				case SK_SET:
					if(in_set)
					{
						ERROR(Format("Stock list \"%s\" can't have nested sets.", stock->id.c_str()));
						delete stock;
						return false;
					}
					if(in_city)
					{
						ERROR(Format("Stock list \"%s\" can't have set block inside city block.", stock->id.c_str()));
						delete stock;
						return false;
					}
					in_set = true;
					stock->code.push_back(SE_START_SET);
					t.Next();
					t.AssertSymbol('{');
					t.Next();
					break;
				case SK_CITY:
					if(in_city)
					{
						ERROR(Format("Stock list \"%s\" already in city block.", stock->id.c_str()));
						delete stock;
						return false;
					}
					t.Next();
					if(t.IsSymbol('{'))
					{
						t.Next();
						in_city = true;
						in_city_else = false;
					}
					else if(t.IsItem())
					{
						const Item* item = FindItem(t.GetItem().c_str(), false, &used_list);
						stock->code.push_back(SE_CITY);
						if(used_list.lis != NULL)
						{
							stock->code.push_back(used_list.is_leveled ? SE_LEVELED_LIST : SE_LIST);
							stock->code.push_back((int)used_list.lis);
						}
						else if(!item)
						{
							ERROR(Format("Stock list \"%s\" have missing item \"%s\".", stock->id.c_str(), t.GetItem().c_str()));
							delete stock;
							return false;
						}
						else
						{
							stock->code.push_back(SE_ITEM);
							stock->code.push_back((int)item);
						}
						t.Next();
						stock->code.push_back(SE_ANY_CITY);
					}
					else
					{
						char c = '{';
						t.StartUnexpected().Add(Tokenizer::T_SYMBOL, (int*)&c).Add(Tokenizer::T_ITEM).Throw();
					}
					break;
				case SK_CHANCE:
					t.Next();
					if(t.IsSymbol('{'))
					{
						t.Next();
						stock->code.push_back(SE_CHANCE);
						uint chance_pos = stock->code.size();
						stock->code.push_back(0);
						stock->code.push_back(0);
						int count = 0, chance = 0;
						while(!t.IsSymbol('}'))
						{
							const Item* item = FindItem(t.MustGetItem().c_str(), false, &used_list);
							if(used_list.lis != NULL)
							{
								stock->code.push_back(used_list.is_leveled ? SE_LEVELED_LIST : SE_LIST);
								stock->code.push_back((int)used_list.lis);
							}
							else if(!item)
							{
								ERROR(Format("Stock list \"%s\" have missing item \"%s\".", stock->id.c_str(), t.GetItem().c_str()));
								delete stock;
								return false;
							}
							else
							{
								stock->code.push_back(SE_ITEM);
								stock->code.push_back((int)item);
							}
							t.Next();
							int ch = t.MustGetInt();
							if(ch <= 0)
							{
								ERROR(Format("Stock list \"%s\" have negative chance %d.", stock->id.c_str(), ch));
								delete stock;
								return false;
							}
							++count;
							chance += ch;
							stock->code.push_back(ch);
							t.Next();
						}
						if(count <= 1)
						{
							ERROR(Format("Stock list \"%s\" have chance with 1 or less items.", stock->id.c_str()));
							delete stock;
							return false;
						}
						stock->code[chance_pos] = count;
						stock->code[chance_pos + 1] = chance;
						t.Next();
					}
					else
					{
						stock->code.push_back(SE_CHANCE);
						stock->code.push_back(2);
						stock->code.push_back(2);
						const Item* item = FindItem(t.MustGetItem().c_str(), false, &used_list);
						if(used_list.lis != NULL)
						{
							stock->code.push_back(used_list.is_leveled ? SE_LEVELED_LIST : SE_LIST);
							stock->code.push_back((int)used_list.lis);
						}
						else if(!item)
						{
							ERROR(Format("Stock list \"%s\" have missing item \"%s\".", stock->id.c_str(), t.GetItem().c_str()));
							delete stock;
							return false;
						}
						else
						{
							stock->code.push_back(SE_ITEM);
							stock->code.push_back((int)item);
						}
						stock->code.push_back(1);
						t.Next();
						item = FindItem(t.MustGetItem().c_str(), false, &used_list);
						if(used_list.lis != NULL)
						{
							stock->code.push_back(used_list.is_leveled ? SE_LEVELED_LIST : SE_LIST);
							stock->code.push_back((int)used_list.lis);
						}
						else if(!item)
						{
							ERROR(Format("Stock list \"%s\" have missing item \"%s\".", stock->id.c_str(), t.GetItem().c_str()));
							delete stock;
							return false;
						}
						else
						{
							stock->code.push_back(SE_ITEM);
							stock->code.push_back((int)item);
						}
						stock->code.push_back(1);
						t.Next();
					}
					break;
				case SK_RANDOM:
					{
						t.Next();
						int a = t.MustGetInt();
						t.Next();
						int b = t.MustGetInt();
						if(a >= b || a < 1 || b < 1)
						{
							ERROR(Format("Stock list \"%s\" have invalid random values (%d, %d).", stock->id.c_str(), a, b));
							delete stock;
							return false;
						}
						stock->code.push_back(SE_RANDOM);
						stock->code.push_back(a);
						stock->code.push_back(b);
						t.Next();
						const Item* item = FindItem(t.MustGetItem().c_str(), false, &used_list);
						if(used_list.lis != NULL)
						{
							stock->code.push_back(used_list.is_leveled ? SE_LEVELED_LIST : SE_LIST);
							stock->code.push_back((int)used_list.lis);
						}
						else if(!item)
						{
							ERROR(Format("Stock list \"%s\" have missing item \"%s\".", stock->id.c_str(), t.GetItem().c_str()));
							delete stock;
							return false;
						}
						else
						{
							stock->code.push_back(SE_ITEM);
							stock->code.push_back((int)item);
						}
						t.Next();
					}
					break;
				default:
					t.Unexpected();
					break;
				}
			}
			else if(t.IsItem())
			{
				stock->code.push_back(SE_ADD);
				const Item* item = FindItem(t.MustGetItem().c_str(), false, &used_list);
				if(used_list.lis != NULL)
				{
					stock->code.push_back(used_list.is_leveled ? SE_LEVELED_LIST : SE_LIST);
					stock->code.push_back((int)used_list.lis);
				}
				else if(!item)
				{
					ERROR(Format("Stock list \"%s\" have missing item \"%s\".", stock->id.c_str(), t.GetItem().c_str()));
					delete stock;
					return false;
				}
				else
				{
					stock->code.push_back(SE_ITEM);
					stock->code.push_back((int)item);
				}
				t.Next();
			}
			else
				t.Unexpected();
		}

		if(stock->code.empty())
		{
			ERROR(Format("Stock list \"%s\" have no code.", stock->id.c_str()));
			delete stock;
			return false;
		}

		stocks.push_back(stock);
		return true;
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to parse stock list '%s': %s", stock->id.c_str(), err));
		delete stock;
		return false;
	}
}

//=================================================================================================
void LoadItems()
{
	Tokenizer t(Tokenizer::F_UNESCAPE | Tokenizer::F_MULTI_KEYWORDS);
	if(!t.FromFile(Format("%s/items.txt", g_system_dir.c_str())))
	{
		return;
	}

	t.AddKeywords(G_ITEM_TYPE, {
		{ "weapon", IT_WEAPON },
		{ "bow", IT_BOW },
		{ "shield", IT_SHIELD },
		{ "armor", IT_ARMOR },
		{ "other", IT_OTHER },
		{ "consumeable", IT_CONSUMEABLE },
		{ "list", IT_LIST },
		{ "leveled_list", IT_LEVELED_LIST },
		{ "stock", IT_STOCK }
	});

	t.AddKeywords(G_PROPERTY, {
		{ "weight", P_WEIGHT },
		{ "value", P_VALUE },
		{ "mesh", P_MESH },
		{ "tex", P_TEX },
		{ "attack", P_ATTACK },
		{ "req_str", P_REQ_STR },
		{ "type", P_TYPE },
		{ "material", P_MATERIAL },
		{ "dmg_type", P_DMG_TYPE },
		{ "flags", P_FLAGS },
		{ "defense", P_DEFENSE },
		{ "mobility", P_MOBILITY },
		{ "skill", P_SKILL },
		{ "tex_override", P_TEX_OVERRIDE },
		{ "effect", P_EFFECT },
		{ "power", P_POWER },
		{ "time", P_TIME }
	});

	t.AddKeywords(G_WEAPON_TYPE, {
		{ "short_blade", WT_SHORT },
		{ "long_blade", WT_LONG },
		{ "axe", WT_AXE },
		{ "blunt", WT_MACE }
	});

	t.AddKeywords(G_MATERIAL, {
		{ "wood", MAT_WOOD },
		{ "skin", MAT_SKIN },
		{ "metal", MAT_IRON },
		{ "crystal", MAT_CRYSTAL },
		{ "cloth", MAT_CLOTH },
		{ "rock", MAT_ROCK },
		{ "body", MAT_BODY },
		{ "bone", MAT_BONE }
	});

	t.AddKeywords(G_DMG_TYPE, {
		{ "slash", DMG_SLASH },
		{ "pierce", DMG_PIERCE },
		{ "blunt", DMG_BLUNT }
	});

	// register item flags (ITEM_TEX_ONLY is not flag in item but property)
	t.AddKeywords(G_FLAGS, {
		{ "not_chest", ITEM_NOT_CHEST },
		{ "not_shop", ITEM_NOT_SHOP },
		{ "not_alchemist", ITEM_NOT_ALCHEMIST },
		{ "quest", ITEM_QUEST },
		{ "not_blacksmith", ITEM_NOT_BLACKSMITH },
		{ "mage", ITEM_MAGE },
		{ "dont_drop", ITEM_DONT_DROP },
		{ "secret", ITEM_SECRET },
		{ "backstab", ITEM_BACKSTAB },
		{ "power1", ITEM_POWER_1 },
		{ "power2", ITEM_POWER_2 },
		{ "power3", ITEM_POWER_3 },
		{ "power4", ITEM_POWER_4 },
		{ "magic_resistance10", ITEM_MAGIC_RESISTANCE_10 },
		{ "magic_resistance25", ITEM_MAGIC_RESISTANCE_25 },
		{ "ground_mesh", ITEM_GROUND_MESH },
		{ "crystal_sound", ITEM_CRYSTAL_SOUND },
		{ "important", ITEM_IMPORTANT },
		{ "not_merchant", ITEM_NOT_MERCHANT },
		{ "not_random", ITEM_NOT_RANDOM },
		{ "hq", ITEM_HQ },
		{ "magical", ITEM_MAGICAL },
		{ "unique", ITEM_UNIQUE }
	});

	t.AddKeywords(G_ARMOR_SKILL, {
		{ "light_armor", (int)Skill::LIGHT_ARMOR },
		{ "medium_armor", (int)Skill::MEDIUM_ARMOR },
		{ "heavy_armor", (int)Skill::HEAVY_ARMOR }
	});

	t.AddKeywords(G_ARMOR_UNIT_TYPE, {
		{ "human", (int)ArmorUnitType::HUMAN },
		{ "goblin", (int)ArmorUnitType::GOBLIN },
		{ "orc", (int)ArmorUnitType::ORC }
	});

	t.AddKeywords(G_CONSUMEABLE_TYPE, {
		{ "potion", (int)ConsumeableType::Potion },
		{ "drink", (int)ConsumeableType::Drink },
		{ "food", (int)ConsumeableType::Food }
	});

	t.AddKeywords(G_EFFECT, {
		{ "heal", E_HEAL },
		{ "regenerate", E_REGENERATE },
		{ "natural", E_NATURAL },
		{ "antidote", E_ANTIDOTE },
		{ "poison", E_POISON },
		{ "alcohol", E_ALCOHOL },
		{ "str", E_STR },
		{ "end", E_END },
		{ "dex", E_DEX },
		{ "antimagic", E_ANTIMAGIC },
		{ "food", E_FOOD }
	});

	t.AddKeywords(G_OTHER_TYPE, {
		{ "tool", Tool },
		{ "valueable", Valueable },
		{ "other", OtherItems },
		{ "artifact", Artifact }
	});
	
	t.AddKeywords(G_STOCK_KEYWORD, {
		{ "set", SK_SET },
		{ "city", SK_CITY },
		{ "else", SK_ELSE },
		{ "chance", SK_CHANCE },
		{ "random", SK_RANDOM }
	});

	int errors = 0;
	
	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeywordGroup(G_ITEM_TYPE))
			{
				ITEM_TYPE type = (ITEM_TYPE)t.GetKeywordId(G_ITEM_TYPE);
				bool ok = true;

				if(type == IT_LIST)
				{
					if(!LoadItemList(t))
						ok = false;
				}
				else if(type == IT_LEVELED_LIST)
				{
					if(!LoadLeveledItemList(t))
						ok = false;
				}
				else if(type == IT_STOCK)
				{
					if(!LoadStock(t))
						ok = false;
				}
				else
				{
					if(!LoadItem(t))
						ok = false;
				}

				if(!ok)
				{
					++errors;
					skip = true;
				}
			}
			else
			{
				int group = G_ITEM_TYPE;
				ERROR(t.FormatUnexpected(Tokenizer::T_KEYWORD_GROUP, &group));
				++errors;
				skip = true;
			}

			if(skip)
				t.SkipTo([](Tokenizer& t) -> bool { return t.IsKeywordGroup(G_ITEM_TYPE); });
			else
				t.Next();
		}
	}
	catch(cstring err)
	{
		ERROR(Format("Failed to load items: %s", err));
		++errors;
	}

	if(errors > 0)
		throw Format("Failed to load items (%d errors), check log for details.", errors);
}

//=================================================================================================
void ClearItems()
{
	DeleteElements(g_item_lists);
	DeleteElements(g_leveled_item_lists);
	DeleteElements(stocks);

	for(auto it : g_items)
		delete it.second;
	g_items.clear();
}

//=================================================================================================
Stock* FindStockScript(cstring id)
{
	assert(id);

	for(Stock* s : stocks)
	{
		if(s->id == id)
			return s;
	}

	return NULL;
}

#undef IN
#undef OUT

enum class CityBlock
{
	IN,
	OUT,
	ANY
};

inline bool CheckCity(CityBlock in_city, bool city)
{
	if(in_city == CityBlock::IN)
		return city;
	else if(in_city == CityBlock::OUT)
		return !city;
	else
		return true;
}

//=================================================================================================
void ParseStockScript(Stock* stock, int level, bool city, vector<ItemSlot>& items)
{
	CityBlock in_city = CityBlock::ANY;
	LocalVector2<int> sets;
	bool in_set = false;
	uint i = 0;

redo_set:
	for(; i < stock->code.size(); ++i)
	{
		switch((StockEntry)stock->code[i])
		{
		case SE_ADD:
			if(CheckCity(in_city, city))
			{
				++i;
				switch((StockEntry)stock->code[i])
				{
				case SE_ITEM:
					++i;
					InsertItemBare(items, (const Item*)stock->code[i]);
					break;
				case SE_LIST:
					++i;
					InsertItemBare(items, ((ItemList*)stock->code[i])->Get());
					break;
				case SE_LEVELED_LIST:
					++i;
					InsertItemBare(items, ((LeveledItemList*)stock->code[i])->Get(level));
					break;
				default:
					assert(0);
					break;
				}
			}
			else
				i += 2;
			break;
		case SE_CHANCE:
			if(CheckCity(in_city, city))
			{
				++i;
				int count = stock->code[i];
				++i;
				int chance = stock->code[i];
				int ch = rand2() % chance;
				int total = 0;
				bool done = false;
				for(int j = 0; j < count; ++j)
				{
					++i;
					StockEntry type = (StockEntry)stock->code[i];
					++i;
					int ptr = stock->code[i];
					++i;
					total += stock->code[i];
					if(ch < total && !done)
					{
						done = true;
						switch(type)
						{
						case SE_ITEM:
							InsertItemBare(items, (const Item*)ptr);
							break;
						case SE_LIST:
							InsertItemBare(items, ((ItemList*)ptr)->Get());
							break;
						case SE_LEVELED_LIST:
							InsertItemBare(items, ((LeveledItemList*)ptr)->Get(level));
							break;
						default:
							assert(0);
							break;
						}
					}
				}
			}
			else
			{
				++i;
				int count = stock->code[i];
				i += 1 + 3 * count;
			}
			break;
		case SE_RANDOM:
			if(CheckCity(in_city, city))
			{
				++i;
				int a = stock->code[i];
				++i;
				int b = stock->code[i];
				++i;
				StockEntry type = (StockEntry)stock->code[i];
				++i;
				int ptr = stock->code[i];
				switch(type)
				{
				case SE_ITEM:
					InsertItemBare(items, (const Item*)ptr);
					break;
				case SE_LIST:
					InsertItemBare(items, ((ItemList*)ptr)->Get());
					break;
				case SE_LEVELED_LIST:
					InsertItemBare(items, ((LeveledItemList*)ptr)->Get(level));
					break;
				default:
					assert(0);
					break;
				}
			}
			else
				i += 4;
			break;
		case SE_CITY:
			in_city = CityBlock::IN;
			break;
		case SE_NOT_CITY:
			in_city = CityBlock::OUT;
			break;
		case SE_ANY_CITY:
			in_city = CityBlock::ANY;
			break;
		case SE_START_SET:
			assert(!in_set);
			sets.push_back(i + 1);
			while(stock->code[i] != SE_END_SET)
				++i;
			break;
		case SE_END_SET:
			assert(in_set);
			return;
		default:
			assert(0);
			break;
		}
	}

	if(sets.size() > 0)
	{
		i = sets[rand2() % sets.size()];
		in_set = true;
		goto redo_set;
	}
}
