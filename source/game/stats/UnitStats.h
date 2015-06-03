// r�ne statystyki postaci
#pragma once

//-----------------------------------------------------------------------------
#include "Attribute.h"
#include "Skill.h"

//-----------------------------------------------------------------------------
// Typ broni
enum BRON
{
	B_BRAK,
	B_JEDNORECZNA,
	B_LUK
	/*
	B_DWURECZNA
	B_MIOTANA
	*/
};

//-----------------------------------------------------------------------------
// Przyczyna trenowania
enum TrainWhat
{
	Train_Hit,
	Train_Hurt,
	Train_Block,
	Train_Bash,
	Train_Shot
};

enum class StatState
{
	NORMAL,
	POSITIVE,
	POSITIVE_MIXED,
	MIXED,
	NEGATIVE_MIXED,
	NEGATIVE,
	MAX
};

extern DWORD stat_state_colors[(int)StatState::MAX];