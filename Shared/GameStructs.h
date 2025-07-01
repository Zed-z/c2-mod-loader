#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <sstream>

struct Vec3i {
	int x, y, z;
};

struct Vec3fx {
	float x, y, z;
};

struct RotPos3i {
	Vec3i rotation;
	Vec3i position;
};

struct RotPos3fx {
	Vec3fx rotation;
	Vec3fx position;
};

struct Mat3x4i {
	int m[3][4];
};

typedef int32_t StratEntityFlags;

struct LocalVarsStruct {
	int32_t vars[20];
	int32_t triggers[];
};

struct StratEntity {
	StratEntity* next;
	StratEntity* prev;
	StratEntity* parent;
	int32_t* InstrStream;
	void* model;
	void* animation;
	const char* name;
	int32_t distanceToPlayer;

	union {
		struct {
			Vec3i newRotation;
			Vec3i newPosition;
		};
		RotPos3i newRotPos;
		RotPos3fx RotPosFx;
	};
	RotPos3i OldRotPos;
	Vec3i scale;
	Mat3x4i matrix0;
	RotPos3i StartRotPos;

	void* collPoints;
	int16_t collExtent;
	uint16_t collisionBoneCount;
	void* collisionBones;
	Vec3i collisionOffset;
	int32_t collRadius;

	int32_t gapField6;
	int16_t wField7;
	int16_t wField8;
	int16_t wField9;
	int16_t wField10;
	Vec3i vec0;

	StratEntityFlags flags0;
	StratEntityFlags flags1;

	void* map;

	LocalVarsStruct* localVars;
	void* triggers;

	int16_t wField0;
	int16_t wField1;
	int32_t field1;
	int32_t triggerCount;
	int32_t* StackPtr;

	int32_t Fade;
	int32_t animIndex0;
	int32_t animSpeed;
	int32_t animFrame;

	int32_t verticalVelocity;

	void* wpFirst;
	void* wpLast;
	void* wpCurrent;
	void* wpField;

	int16_t shadowSpriteIndex;
	int16_t shadowSize;

	BYTE bField2;

	BYTE blinkCount;
	BYTE blinkCountdown;
	BYTE blinkNum;

	BYTE field6[2];
	int16_t gap13[2];
	int32_t gap14[1];
};
