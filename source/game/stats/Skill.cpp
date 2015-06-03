// character skills & gain tables
#include "Pch.h"
#include "Base.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
struct StatGain
{
	float value; // skill gain per 5 levels
	int weight; // weight for level calculation
};

//-----------------------------------------------------------------------------
// Skill gain table. Normalny no class have skill better then 20 but it may happen.
static StatGain gain[] = {
	0.f,	0,	// 0
	1.f,	0,	// 1
	2.f,	0,	// 2
	3.f,	0,	// 3
	4.f,	0,	// 4
	5.f,	1,	// 5
	6.f,	1,	// 6
	7.f,	1,	// 7
	8.f,	1,	// 8
	9.f,	1,	// 9
	10.f,	2,	// 10
	11.f,	2,	// 11
	12.f,	2,	// 12
	13.f,	2,	// 13
	14.f,	2,	// 14
	15.f,	3,	// 15
	15.5f,	3,	// 16
	16.f,	3,	// 17
	16.5f,	3,	// 18
	17.f,	3,	// 19
	17.5f,	4,	// 20
	18.f,	4,	// 21
	18.5f,	4,	// 22
	19.f,	4,	// 23
	19.5f,	4,	// 24
	20.f,	5,	// 25
};

//-----------------------------------------------------------------------------
// List of all skills
SkillInfo g_skills[(int)Skill::MAX] = {
	SkillInfo(Skill::ONE_HANDED_WEAPON, "one_handed_weapon", SkillGroup::WEAPON, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::SHORT_BLADE, "short_blade", SkillGroup::WEAPON, Attribute::DEX),
	SkillInfo(Skill::LONG_BLADE, "long_blade", SkillGroup::WEAPON, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::BLUNT, "blunt", SkillGroup::WEAPON, Attribute::STR),
	SkillInfo(Skill::AXE, "axe", SkillGroup::WEAPON, Attribute::STR),
	SkillInfo(Skill::BOW, "bow", SkillGroup::WEAPON, Attribute::DEX),
	SkillInfo(Skill::UNARMED, "unarmed", SkillGroup::WEAPON, Attribute::DEX, Attribute::STR),
	SkillInfo(Skill::SHIELD, "shield", SkillGroup::ARMOR, Attribute::STR, Attribute::DEX),
	SkillInfo(Skill::LIGHT_ARMOR, "light_armor", SkillGroup::ARMOR, Attribute::DEX),
	SkillInfo(Skill::MEDIUM_ARMOR, "medium_armor", SkillGroup::ARMOR, Attribute::CON, Attribute::DEX),
	SkillInfo(Skill::HEAVY_ARMOR, "heavy_armor", SkillGroup::ARMOR, Attribute::STR, Attribute::CON),
	SkillInfo(Skill::NATURE_MAGIC, "nature_magic", SkillGroup::MAGIC, Attribute::WIS),
	SkillInfo(Skill::GODS_MAGIC, "gods_magic", SkillGroup::MAGIC, Attribute::WIS),
	SkillInfo(Skill::MYSTIC_MAGIC, "mystic_magic", SkillGroup::MAGIC, Attribute::INT),
	SkillInfo(Skill::SPELLCRAFT, "spellcraft", SkillGroup::MAGIC, Attribute::INT, Attribute::WIS),
	SkillInfo(Skill::CONCENTRATION, "concentration", SkillGroup::MAGIC, Attribute::CON, Attribute::WIS),
	SkillInfo(Skill::IDENTIFICATION, "identification", SkillGroup::MAGIC, Attribute::INT),
	SkillInfo(Skill::LOCKPICK, "lockpick", SkillGroup::OTHER, Attribute::DEX, Attribute::INT),
	SkillInfo(Skill::SNEAK, "sneak", SkillGroup::OTHER, Attribute::DEX),
	SkillInfo(Skill::TRAPS, "traps", SkillGroup::OTHER, Attribute::DEX, Attribute::INT),
	SkillInfo(Skill::STEAL, "steal", SkillGroup::OTHER, Attribute::DEX, Attribute::CHA),
	SkillInfo(Skill::ANIMAL_EMPATHY, "animal_empathy", SkillGroup::OTHER, Attribute::CHA, Attribute::WIS),
	SkillInfo(Skill::SURVIVAL, "survival", SkillGroup::OTHER, Attribute::CON, Attribute::WIS),
	SkillInfo(Skill::PERSUASION, "persuasion", SkillGroup::OTHER, Attribute::CHA),
	SkillInfo(Skill::ALCHEMY, "alchemy", SkillGroup::OTHER, Attribute::INT),
	SkillInfo(Skill::CRAFTING, "crafting", SkillGroup::OTHER, Attribute::INT, Attribute::DEX),
	SkillInfo(Skill::HEALING, "healing", SkillGroup::OTHER, Attribute::WIS),
	SkillInfo(Skill::ATHLETICS, "athletics", SkillGroup::OTHER, Attribute::CON, Attribute::STR),
	SkillInfo(Skill::RAGE, "rage", SkillGroup::OTHER, Attribute::CON)
};

//-----------------------------------------------------------------------------
// List of all skill groups
SkillGroupInfo g_skill_groups[(int)SkillGroup::MAX] = {
	SkillGroupInfo(SkillGroup::WEAPON, "weapon"),
	SkillGroupInfo(SkillGroup::ARMOR, "armor"),
	SkillGroupInfo(SkillGroup::MAGIC, "magic"),
	SkillGroupInfo(SkillGroup::OTHER, "other")
};

//=================================================================================================
SkillInfo* SkillInfo::Find(const string& id)
{
	for(SkillInfo& si : g_skills)
	{
		if(id == si.id)
			return &si;
	}

	return NULL;
}

//=================================================================================================
SkillGroupInfo* SkillGroupInfo::Find(const string& id)
{
	for(SkillGroupInfo& sgi : g_skill_groups)
	{
		if(id == sgi.id)
			return &sgi;
	}

	return NULL;
}

//=================================================================================================
void SkillInfo::Validate(int& err)
{
	for(int i = 0; i<(int)Skill::MAX; ++i)
	{
		SkillInfo& si = g_skills[i];
		if(si.skill_id != (Skill)i)
		{
			WARN(Format("Skill %s: id mismatch.", si.id));
			++err;
		}
		if(si.name.empty())
		{
			WARN(Format("Skill %s: empty name.", si.id));
			++err;
		}
		if(si.desc.empty())
		{
			WARN(Format("Skill %s: empty desc.", si.id));
			++err;
		}
	}

	for(int i = 0; i<(int)SkillGroup::MAX; ++i)
	{
		SkillGroupInfo& sgi = g_skill_groups[i];
		if(sgi.group_id != (SkillGroup)i)
		{
			WARN(Format("Skill group %s: id mismatch.", sgi.id));
			++err;
		}
		if(sgi.name.empty())
		{
			WARN(Format("Skill group %s: empty name.", sgi.id));
			++err;
		}
	}
}

//=================================================================================================
float SkillInfo::GetModifier(int base, int& weight)
{
	if(base <= 0)
	{
		weight = 0;
		return 0.f;
	}
	else if(base >= 25)
	{
		weight = 5;
		return 8.75f;
	}
	else
	{
		StatGain& sg = gain[base];
		weight = sg.weight;
		return sg.value;
	}
}