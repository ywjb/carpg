// konsola w grze, otwierana tyld�
#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "InputTextBox.h"

//-----------------------------------------------------------------------------
class Console : public GameDialogBox, public OnCharHandler
{
public:
	explicit Console(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void AddText(cstring str);
	void OnInput(const string& str);
	void LoadData();
	void Reset() { itb.Reset(); }

private:
	TEX tBackground;
	InputTextBox itb;
	bool added;
};