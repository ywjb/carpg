#pragma once

//-----------------------------------------------------------------------------
#include <ComboBox.h>
#include <TooltipController.h>
#include <Button.h>
#include "Location.h"

//-----------------------------------------------------------------------------
// Show world map with locations, allow travel
class WorldMapGui : public Control
{
public:
	WorldMapGui();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	bool NeedCursor() const override { return true; }
	void Event(GuiEvent e) override;
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Clear();
	Vec2 WorldPosToScreen(const Vec2& pt) const;
	void ShowEncounterMessage(cstring text);
	void StartTravel(bool fast = false);
	bool HaveFocus() const { return !combo_search.focus; }

	DialogBox* dialog_enc;

private:
	void AppendLocationText(Location& loc, string& s);
	void GetCityText(City& city, string& s);
	void CenterView(float dt, const Vec2* target = nullptr);
	Vec2 GetCameraCenter() const;
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);

	TexturePtr tMapBg, tWorldMap, tMapIcon[LI_MAX], tEnc, tSelected[2], tMover, tSide, tMagnifyingGlass, tTrackingArrow;
	cstring txWorldDate, txCurrentLoc, txCitizens, txAvailable, txTarget, txDistance, txTravelTime, txDay, txDays, txOnlyLeaderCanTravel, txBuildings;
	ComboBox combo_search;
	Button buttons[2];
	TooltipController tooltip;
	Vec2 offset, c_pos;
	float zoom;
	int picked_location, tracking;
	bool clicked, follow, fast_follow, c_pos_valid;
};
