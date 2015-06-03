// drzwi
#pragma once

//-----------------------------------------------------------------------------
// id zamka
#define LOCK_NONE 0
#define LOCK_UNLOCKABLE 1
#define LOCK_KOPALNIA 2
#define LOCK_ORKOWIE 3
#define LOCK_SAMOUCZEK 100

//-----------------------------------------------------------------------------
struct Door
{
	enum State
	{
		Closed,
		Opening,
		Opening2,
		Open,
		Closing,
		Closing2
	};

	VEC3 pos;
	float rot;
	INT2 pt;
	int locked;
	int netid;
	bool door2;

	// lokalne zmienne
	State state;
	AnimeshInstance* ani;
	btCollisionObject* phy;

	Door() : door2(false)
	{
	}
	~Door()
	{
		delete ani;
	}
	inline bool IsBlocking() const
	{
		return state == Closed || state == Opening || state == Closing2;
	}
	inline bool IsBlockingView() const
	{
		return state == Closed;
	}
	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
};