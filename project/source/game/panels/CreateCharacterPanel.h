// character creation screen
#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "CheckBox.h"
#include "Slider.h"
#include "Class.h"
#include "ListBox.h"
#include "TextBox.h"
#include "HumanData.h"
#include "Attribute.h"
#include "Skill.h"
#include "Perk.h"
#include "TooltipController.h"
#include "FlowContainer.h"
#include "CreatedCharacter.h"

//-----------------------------------------------------------------------------
struct Unit;
class PickItemDialog;

//-----------------------------------------------------------------------------
class CreateCharacterPanel : public GameDialogBox
{
public:
	enum class Group
	{
		Section,
		Attribute,
		Skill,
		Perk,
		TakenPerk,
		PickSkill_Button,
		PickPerk_AddButton,
		PickPerk_RemoveButton,
		PickPerk_DisabledButton
	};

	enum class Mode
	{
		PickClass,
		PickSkillPerk,
		PickAppearance
	};

	explicit CreateCharacterPanel(DialogInfo& info);
	~CreateCharacterPanel();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Init();
	void LoadData();
	void Show(bool enter_name);
	void ShowRedo(Class clas, int hair_index, HumanData& hd, CreatedCharacter& cc);

	// data
	CustomButton custom_x, custom_bt[2];

	// results
	CreatedCharacter cc;
	Class clas;
	string player_name;
	Unit* unit;
	int hair_index;

private:
	enum DOLL_ANIM
	{
		DA_STOI,
		DA_IDZIE,
		DA_ROZGLADA,
		DA_WYJMIJ_BRON,
		DA_SCHOWAJ_BRON,
		DA_WYJMIJ_LUK,
		DA_SCHOWAJ_LUK,
		DA_ATAK,
		DA_STRZAL,
		DA_BLOK,
		DA_BOJOWY
	};

	void SetControls();
	void SetCharacter();
	void OnEnterName(int id);
	void RenderUnit();
	void UpdateUnit(float dt);
	void OnChangeClass(int index);
	cstring GetText(int group, int id);
	void GetTooltip(TooltipController* tooltip, int group, int id);
	void ClassChanged();
	void OnPickSkill(int group, int id);
	void OnPickPerk(int group, int id);
	void RebuildSkillsFlow();
	void RebuildPerksFlow();
	void ResetSkillsPerks();
	void OnShowWarning(int id);
	void PickAttribute(cstring text, Perk picked_perk);
	void PickSkill(cstring text, Perk picked_perk, bool positive = true, int multiple = 0);
	void OnPickAttributeForPerk(int id);
	void OnPickSkillForPerk(int id);
	void UpdateSkill(Skill s, int value, bool mod);
	void UpdateSkillButtons();
	void AddPerk(Perk perk, int value = 0, bool apply = true);
	void CheckSkillsUpdate();
	void UpdateInventory();
	void ResetDoll(bool instant);
	void RandomAppearance();
	Group FlowGroupToTooltipGroup(Group group);
	int GetItemsMod();

	Mode mode;
	bool enter_name;
	// unit
	DOLL_ANIM anim, anim2;
	float t, dist;
	// controls
	Button btCancel, btNext, btBack, btCreate, btRandomSet;
	CheckBox checkbox;
	Slider slider[5];
	ListBox lbClasses;
	TextBox tbClassDesc, tbInfo;
	FlowContainer flowSkills, flowPerks;
	// attribute/skill flow panel
	Int2 flow_pos, flow_size;
	Scrollbar flow_scroll;
	vector<OldFlowItem> flow_items;
	TooltipController tooltip;
	// data
	bool reset_skills_perks, rotating;
	cstring txHardcoreMode, txHair, txMustache, txBeard, txHairColor, txSize, txCharacterCreation, txName, txAttributes, txRelatedAttributes, txCreateCharWarn,
		txSkillPoints, txPerkPoints, txPickAttribIncrease, txPickAttribDecrease, txPickSkillIncrease, txAdvantages, txDisadvantages, txPerks, txTakenPerks,
		txCreateCharTooMany, txFlawExtraPerk;
	Perk picked_perk;
	PickItemDialog* pickItemDialog;
	vector<Perk> available_adv, available_disadv, available_perks;
	vector<std::pair<cstring, int>> taken_perks;
	const Item* items[4];
	TEX tKlasaCecha;
};
