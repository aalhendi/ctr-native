#include <ctr_math.h>

enum Characters
{
	CRASH_BANDICOOT = 0,
	NEO_CORTEX,
	TINY_TIGER,
	COCO_BANDICOOT,
	N_GIN,
	DINGODILE,
	POLAR,
	PURA,
	PINSTRIPE,
	PAPU_PAPU,
	RIPPER_ROO,
	KOMODO_JOE,
	N_TROPY,
	PENTA_PENGUIN,
	FAKE_CRASH,
	NITROS_OXIDE = 15
};

enum KartState
{
	KS_NORMAL = 0,
	KS_CRASHING = 1,
	KS_DRIFTING = 2,
	KS_SPINNING = 3,
	KS_ENGINE_REVVING = 4,
	KS_MASK_GRABBED = 5,
	KS_BLASTED = 6,
	KS_ANTIVSHIFT = 9,
	KS_WARP_PAD = 10,
	KS_FREEZE = 11
};

enum PhysType
{
	// MetaPhys[0]
	GRAVITY = 0,
	// Driver offset
	GRAVITY_OFFSET = 0x416,

	// MetaPhys[1]
	JUMP = 1,
	// Driver offset
	JUMP_OFFSET = 0x418,

	// MetaPhys[2]
	ACCEL_SLIDE = 0x2,
	// Driver offset
	ACCEL_SLIDE_OFFSET = 0x41A,

	// MetaPhys[3]
	ACCEL_FRICTION = 0x3,
	// Driver offset
	ACCEL_FRICTION_OFFSET = 0x41C,

	// MetaPhys[4]
	SLIDING_UNK41E = 0x4,
	// Driver offset
	SLIDING_UNK41E_OFFSET = 0x41E,

	// MetaPhys[5]
	FRICTION_UNK420 = 0x5,
	// Driver offset
	FRICTION_UNK420_OFFSET = 0x420,

	// MetaPhys[6]
	BRAKING_FRICTION = 0x6,
	// Driver offset
	BRAKING_FRICTION_OFFSET = 0x422,

	// MetaPhys[7]
	DRIFT_UNK424 = 0x7,
	// Driver offset
	DRIFT_UNK424_OFFSET = 0x424,

	// MetaPhys[8]
	DRIFT_FRICTION = 0x8,
	// Driver offset
	DRIFT_FRICTION_OFFSET = 0x426,

	// MetaPhys[9]
	ACCELERATION_NO_RESERVES = 9,
	// Driver offset
	ACCEL_NO_RESERVES_OFFSET = 0x428,

	// MetaPhys[A]
	ACCELERATION_WITH_RESERVES = 0xA,
	// Driver offset
	ACCEL_WITH_RESERVES_OFFSET = 0x42A,

	// MetaPhys[B]
	CLASS_SPEED = 0xB,
	// Driver offset
	CLASS_SPEED_OFFSET = 0x42C,

	// MetaPhys[C]
	SPEEDOMETER_UNK42E = 0xC,
	// Driver offset
	SPEEDOMETER_UNK42E_OFFSET = 0x42E,

	// MetaPhys[D]
	MAX_SPEED_SINGLE_TURBO = 0xD,
	// Driver offset
	MAX_SPEED_SINGLE_TURBO_OFFSET = 0x430,

	// MetaPhys[E]
	MAX_SPEED_WITH_SACRED = 0xE,
	// Driver offset
	MAX_SPEED_WITH_SACRED_OFFSET = 0x432,

	// MetaPhys[F]
	RESERVE_SPEED = 0xF,
	// Driver offset
	RESERVE_SPEED_OFFSET = 0x434,

	// MetaPhys[10]
	MASK_SPEED = 0x10,
	// Driver offset
	MASK_SPEED_OFFSET = 0x436,

	// MetaPhys[11]
	TURN_SPEED_UNK438 = 0x11,
	// Driver offset
	TURN_SPEED_UNK438_OFFSET = 0x438,

	// MetaPhys[12]
	TURNING_STAT = 0x12,
	// Driver offset
	TURNING_STAT_OFFSET = 0x43A,

	// MetaPhys[13]
	REVERSE_TURNING_SPEED = 0x13,
	// Driver offset
	REVERSE_TURNING_SPEED_OFFSET = 0x43B,

	// MetaPhys[14]
	TURN_UNK43C = 0x14,
	// Driver offset
	TURN_UNK43C_OFFSET = 0x43C,

	// MetaPhys[15]
	TURN_UNK43E = 0x15,
	// Driver offset
	TURN_UNK43E_OFFSET = 0x43E,

	// MetaPhys[16]
	// Driver offset 0x440

	// MetaPhys[17]
	// Driver offset 0x442

	// MetaPhys[18]
	// Driver offset 0x444

	// MetaPhys[19]
	// Driver offset 0x446

	// MetaPhys[1A]
	// Driver offset 0x447

	// MetaPhys[1B]
	// Driver offset 0x448

	// MetaPhys[1C]
	// Driver offset 0x44A

	// MetaPhys[1D]
	// Driver offset 0x44C

	// MetaPhys[1E]
	// Driver offset 0x44E

	// MetaPhys[1F]
	// Driver offset 0x450

	// MetaPhys[20]
	// Driver offset 0x452

	// MetaPhys[21]
	// Driver offset 0x454

	// MetaPhys[22]
	// Driver offset 0x457	(457 char == 456 s16)

	// MetaPhys[23]
	// Driver offset 0x458

	// MetaPhys[24]
	// Driver offset 0x459

	// MetaPhys[25]
	// Driver offset 0x45A

	// MetaPhys[26] Kart Turn Animation (speed?)
	// Driver offset 0x45C

	// MetaPhys[27]
	// Driver offset 0x45D

	// MetaPhys[28]
	// Driver offset 0x45E

	// MetaPhys[29]
	// Driver offset 0x45F

	// MetaPhys[2A]
	// Driver offset 0x460

	// MetaPhys[2B]
	// Driver offset 0x461

	// MetaPhys[2C]
	// Driver offset 0x462

	// MetaPhys[2D]
	// Driver offset 0x463

	// MetaPhys[2E]
	// Driver offset 0x464

	// MetaPhys[2F]
	// Driver offset 0x466

	// MetaPhys[30]
	// Driver offset 0x468

	// MetaPhys[31]
	// Driver offset 0x46A

	// MetaPhys[32]
	// Driver offset 0x46B

	// MetaPhys[33]
	// Driver offset 0x46C

	// MetaPhys[34]
	// Driver offset 0x46E

	// MetaPhys[35]
	// Driver offset 0x470

	// MetaPhys[36]
	// Driver offset 0x472

	// MetaPhys[37]
	// Driver offset 0x474

	// MetaPhys[38]
	TURBO_METER_EMPTY = 0x38,
	// Driver offset
	TURBO_METER_EMPTY_OFFSET = 0x476,

	// MetaPhys[39]
	TURBO_METER_FILLED = 0x39,
	// Driver offset
	TURBO_METER_FILLED_OFFSET = 0x477,

	// MetaPhys[3A]
	// Driver offset 0x478

	// MetaPhys[3B]
	// Driver offset 0x479

	// MetaPhys[3C]
	// Driver offset 0x47A

	// MetaPhys[3D]
	// Driver offset 0x47C

	// MetaPhys[3E]
	// Driver offset 0x47E

	// MetaPhys[3F]
	// Driver offset 0x480

	// offset 482 is NOT const,
	// that's driver rank

	// MetaPhys[40]
	PROTOTYPE_KEY = 0x40,
	// Driver offset
	PROTOTYPE_KEY_OFFSET = 0x484,

	NUM_PHYS_TYPES = 65

	// ...
	// 65 (0x41) exist
};

enum TurboType
{
	START_LINE_BOOST = 0,
	FREEZE_RESERVES_ON_TURBO_PAD = 0x1,
	POWER_SLIDE_HANG_TIME = 0x2,
	TURBO_PAD = 0x4,
	TURBO_ITEM = 0x8,
	SUPER_ENGINE = 0x10
};

enum EngineClass
{
	// crash, cortex, joe, fake, oxide
	BALANCED,

	// coco, ngin, pinstripe
	ACCEL,

	// tiny, dingo, papu, ntropy
	SPEED,

	// polar, pura, roo (penta in ntsc)
	TURN,

	NUM_CLASSES
};

enum Actions
{
	ACTION_TOUCH_GROUND = 0x1,
	ACTION_WARP = 0x4000,
	ACTION_BOT = 0x100000,
	ACTION_RACE_FINISHED = 0x2000000,
};

enum BotFlags
{
	BOT_FLAG_ESTIMATE_NAV = 0x1,
	BOT_FLAG_DAMAGE_ACTIVE = 0x2,
	BOT_FLAG_MOON_GRAVITY = 0x20,
	BOT_FLAG_BOSS_PATH_REQUESTED = 0x40,
	BOT_FLAG_BOSS_PATH_ACTIVE = 0x80,
};

struct MetaPhys
{
	u32 unusedDebugStr;
	int offset;
	int size;
	int value[NUM_CLASSES];
};


struct Turbo
{
	struct Instance *inst;
	struct Driver *driver;

	// 0x8
	// Index for the animation frame of the exhaust fire
	// Value 0 means it uses turbo0, 1 means it uses turbo1, so on until 7 is reached and it loops back to 0
	s16 fireAnimIndex;

	// 0xA
	// Stores the visual size of the exhaust fire
	// Integer value ranging from 4 to 8
	// One power-slide and green hang time is 5
	// Two power-slides and yellow hang time is 6
	// Three power-slides, red hang time, and start boost is 7
	// Turbo pad and USF is 8
	// 4 appears to be unused
	// Fire doesn't get any smaller at values lower than 4 nor bigger at values higher than 8
	s16 fireSize;

	// 0xC
	// Value that decreases every time VehTurbo_ThTick gets called
	// If it reaches 0 it makes the fire start disappearing
	s8 fireDisappearCountdown;

	// 0xD
	// Used for the distortion of the sound that indicates active fire/reserves
	u8 fireAudioDistort;

	// 0xE
	// Cooldown for when fire is visible
	// Set to 96 (which makes fire invisible for 0.1 seconds, 96 / 1000 = 96ms = 0.1s) when obtaining turbo from certain sources, namely those from
	// power-sliding (used to make fire pop with each power-slide)
	s16 fireVisibilityCooldown;
};

struct BotData
{
	// these offset are from the perspective as they exist from within `struct Driver`

	// 0x598, offset in `struct BotData` == 0x0
	struct Item item;

	// 0x5a0, offset in `struct BotData` == 0x8
	int unk5a0;

	// 0x5a4, offset in `struct BotData` == 0xc
	struct NavFrame *botNavFrame;

	// 0x5a8
	int navProgressRemainder;

	// 0x5ac
	int unk5ac;

	// 0x5b0
	// u32 BotFlags plus other AI-only state bits.
	// bits 9-16 might be = (navframe flags << 8)
	u32 botFlags;

	// 0x5b4
	// acceleration from start-line
	int botAccel;

	// 0x5b8
	// s16 path index
	s16 botPath;

	// 0x5ba
	s16 aiDamageState;


	/*
	 * regarding unk5bc:
	 *
	 * 0x5bc - 0x5bd: likely a s16, a timer of some sort, similar to squishTimer.
	 */

	// 0x5bc
	// incline rotXZ
	// probably only for AIs

	// unk5bc + 18 is botVelocity?
	// char unk5bc[0x34];
	union
	{
		char raw[0x34];
		struct
		{
			// 0x5bc
			s16 rotXZ;

			// 0x5be
			s16 ai_driftTarget;

			// 0x5c0
			s16 ai_mulDrift;

			// 0x5c2
			s16 ai_simpTurnState;

			// 0x5c4
			s16 ai_turboMeter;

			// 0x5c6
			s16 ai_fireLevel;

			// 0x5c8
			int ai_squishCooldown; // retail uses both halfword and word operations on this slot

			// 0x5cc
			int unk5cc;

			// 0x5d0
			int ai_speedY;

			// 0x5d4
			int ai_speedLinear;

			// 0x5d8
			int ai_accelAxis[3];

			// 0x5e4
			int ai_velAxis[3];
		};
	} unk5bc;

	// 0x5d4
	// AI speed

	// 0x5f0
	int ai_posBackup[3];

	// 0x5fc
	s16 ai_rot4[4];

	// 0x604
	int ai_progress_cooldown;

	// 0x608
	s16 ai_rotY_608;

	// 0x60a
	s16 ai_quadblock_checkpointIndex; // 0x60a almost certainly is a char and not a s16.

	// within the regions POTENTALNAVFRAMESTART/END, is this a navframe?
	// POTENTAL NAV FRAME START
	// 0x60c
	s16 estimatePos[3];

	// 0x612
	u8 estimateRotNav[3];

	// 0x615
	u8 estimateRotCurrY;

	// 0x616
	s16 distToNextNavXYZ;

	// 0x618
	s16 distToNextNavXZ;

	// 0x61A
	s16 unk61a;

	// 0x61c
	int unk61c;
	// POTENTAL NAV FRAME END

	// 0x620
	struct MaskHeadWeapon *maskObj;

	// 0x624
	s16 weaponCooldown;

	// 0x626
	u8 blastBounceCount;
	u8 desiredPath_BossOnly;

	// 0x628
	int unk628;
};

// for Players, AIs and Ghosts
struct Driver
{
	// 0x0
	struct Icon **wheelSprites;
	// 0x4
	u16 wheelSize;

	// 0x6
	// Front wheel rotation sprite frame offset
	// Also controls the LR panning of the Engine Sound
	// Default: 0
	// Steering left ranges between 0 to 64
	// Steering right ranges between 0 to -64 (if we display it as a signed number)
	s16 wheelRotation;
	// 0x8
	u32 tireColor;
	// 0xC
	s16 clockReceive;
	// 0xE
	s16 hazardTimer;
	// 0x10
	struct Instance *instBombThrow;
	// 0x14
	struct Instance *instBubbleHold;
	// 0x18
	struct Instance *instTntRecv; // on your head
	// 0x1C
	struct Instance *instSelf;

// Not in Aug Review
#if BUILD >= SepReview
	// 0x20
	struct Instance *instTntSend; // on the ground
#endif

	// 0x24
	int invincibleTimer;
	// 0x28
	int invisibleTimer;
	// 0x2C
	u32 instFlagsBackup;
	// 0x30
	s8 numWumpas;
	// 0x31
	char numCrystals;
	// 0x32
	char numTimeCrates;
	// 0x33
	s8 accelConst;
	// 0x34
	s8 turnConst;

	// 0x35
	// Super Engine from Beta,
	// ever used in retail?
	s8 turboConst;

	// 0x36
	u8 heldItemID;
	// 0x37
	u8 numHeldItems;
	// 0x38
	s16 superEngineTimer;
	// 0x3A
	s16 itemRollTimer;
	// 0x3C
	s16 noItemTimer;
	// 0x3E
	s16 unknown_noitemtimer_laptime;
	// 0x40
	int lapTime;
	// 0x44
	u8 lapIndex;
	// 0x45
	u8 clockSend;
	// 0x46
	s16 jumpMeter;
	// 0x48
	s16 jumpMeterTimer;
	// 0x4A
	u8 driverID;
	// 0x4B
	s8 simpTurnState;
	// 0x4C
	u8 matrixArray;
	// 0x4D
	u8 matrixIndex;

#if BUILD >= EurRetail
	s16 compilerPadding_0x4E;

	// 0x50
	// highest amount of consecutive turbos in a race
	// exclusive to Japan Retail
	int numTurbosHighScore;
#endif

#if BUILD >= SepReview
	// 0x4E -- UsaRetail
	// 0x54 -- EurRetail, JpnRetail
	s16 numTurbos;
	// 0x50
	u16 frameAgainstWall; // allocated in Sep3, does not function
#endif

#if BUILD < EurRetail
	// There is no "s16" on 0x52,
	// there is padding for the next
	// 4-byte void* that is unused
	s16 funcPtrs_compilerpadding;
#endif

	// 0x54 (UsaRetail) / 0x58 (EurRetail, JpnRetail) - OnInit, First function for spawn, drifting, damage, etc
	// 0x58 - OnUpdate, updates per frame for any generic purpose
	// 0x5C - OnPhysLinear
	// 0x60 - OnAudio, engine sounds (always same)
	// 0x64 - OnPhysAngular
	// 0x68 - VehPhysForce_OnApplyForces
	// 0x6C - COLL_FIXED_PlayerSearch
	// 0x70 - VehPhysForce_CollideDrivers
	// 0x74 - COLL_MOVED_PlayerSearch
	// 0x78 - OnWeapon (jump related?)
	// 0x7C - VehPhysForce_TranslateMatrix (pos, rot, scale)
	// 0x80 - OnAnimate
	// 0x84 - OnParticles
	void *funcPtrs[0xD];

	// 0x88
	Vec3 velocity;

	// 0x94
	Vec3 originToCenter;

	// 0xA0 - quadblock currently touched,
	// it is zero while airborne
	struct QuadBlock *currBlockTouching;

	// 0xA4
	SVec3 normalVecUP;
	s16 unkAA;

	// 0xac
	s16 spsHitPos[4];

	// 0xb4
	s16 spsNormalVec[4];

	// 0xBC
	// 0xBD is waterFlag
	u32 stepFlagSet;

	// 0xC0
	s16 ampTurnState;

	// 0xC2
	u8 currentTerrain;

	// 0xc3
	// eight frames, 0-7
	char skidmarkFrameIndex;

	// 0xC4
	// 0x010 per tire,
	// 0x040 per frame (4 tires)
	// 0x200 total (8 frames)
	char skidmarks[0x200];

	// 0x2C4
	u32 skidmarkEnableFlags;

	// actions:
	// 0x00000001 - touching quadblock
	// 0x00000002 - started touching quadblock this frame
	// 0x00000004 - holding L1 or R1
	// 0x00000008 - accel prevention (holding square)
	// 0x00000010 - ? steering
	// 0x00000020 - ? brakes+gas
	// 0x00000040 - ? COLL-related (ghost)
	// 0x00000080 - ? VehPhysForce_TranslateMatrix-related, turbo-related
	// 0x00000100 - driving wrong way
	// 0x00000200 - using turbo weapon
	// 0x00000400 - started jump this frame
	// 0x00000800 - back wheel skid
	// 0x00001000 - front wheel skid
	// 0x00002000 - driving against wall
	// 0x00004000 - warping (or blasted, or LastSpin)
	// 0x00008000 - want to fire weapon
	// 0x00010000 - echo engine
	// 0x00020000 - reversing engine
	// 0x00040000 - race timer frozen
	// 0x00080000 - airborne
	// 0x00100000 - driver is an AI
	// 0x00200000 - new boost this frame
	// 0x00400000 - ?
	// 0x00800000 - using mask weapon
	// 0x01000000 - behind start line
	// 0x02000000 - race finished for this driver
	// 0x04000000 - tracker chasing you
	// 0x08000000 - ? checkpointIndex-related
	// 0x10000000 - human-human collision
	// ?
	// ?
	// 0x80000000 - dropping mine


	// 0x2C8
	u32 actionsFlagSet;

	// 0x2CC
	u32 actionsFlagSetPrevFrame;

	// 0x2D0
	int quadBlockHeight;

	// 0x2D4
	Vec3 posCurr;

	// 0x2E0
	// used for velocity in 231
	Vec3 posPrev;

	// 0x2EC
	// This is render rotation, not velocity direction,
	// these are the variables that get turned into
	// instance matrix
	struct
	{
		s16 x;
		s16 y;
		s16 z;
		s16 w;
	} rotCurr;

	// 0x2F4
	// used for velocity in 231
	struct
	{
		s16 x;
		s16 y;
		s16 z;
		s16 w;
	} rotPrev;

	// 0x2FC
	int sfxDistortOffset;

	// 0x300
	// 0x300 = Kart skidmarks sound
	// 0x304 = No sound yet defined* (VehEmitter.c: VehEmitter_DriverMain)
	// 0x308 = Kart "kirb_dirt" sound
	// 0x30C = Kart "engine_jet" sound
	void *driverAudioPtrs[4];

	// 0x310
	MATRIX matrixMovingDir;

	// 0x330
	MATRIX matrixFacingDir;

	// 0x350
	// continues updating while driver is airborne,
	// used for VisMem (sometimes?)
	struct QuadBlock *underDriver;

	// 0x354
	// last "valid" quadblock the driver touched
	// used for mask grab if next block is invalid
	struct QuadBlock *lastValid;

	// 0x358
	// is it ice, gravel, or what?
	struct Terrain *terrainMeta1;

	// 0x35C
	struct Terrain *terrainMeta2;

	// each normalVec is 8 bytes apart,
	// used as an array of vec4s, with
	// variables sneaked in-between

	// 0x360
	// used in PhysLinear, reset in VehPhysForce_OnApplyForces, calculated in StartSearch,
	// all three are funcPtrs in the driver struct, in that order of operation
	SVec3 AxisAngle1_normalVec;

	// 0x366
	// forced to jump while on turtles,
	// should we say "forcedJump_turtles"?
	u8 forcedJump_trampoline;

	// 0x367
	// clock effect
	char clockFlash;

	// 0x368
	s16 AxisAngle2_normalVec[3];

	// 0x36e
	// Seems to control the speedometer needle to show base current speed
	// Altought the needle is also controlled a little bit by other variables that are not constants
	s16 unk36E;

	// 0x370
	s16 AxisAngle3_normalVec[3];

	// 0x376
	u8 kartState;

	// 0x377
	s8 Screen_OffsetY;

	// 0x378
	s16 AxisAngle4_normalVec[3];

	// 0x37e
	s16 unk37e;

	// 0x380
	char normalVecID;
	char unk381;

	// 0x382
	s16 buttonUsedToStartDrift;

	// 0x384
	s16 posWallColl[3];

	// 0x38A
	s16 scrubMeta8;

	// 0x38C
	s16 speed;

	// 0x38E
	s16 speedApprox;

	// 0x390
	s16 jumpHeightCurr;

	// 0x392
	s16 jumpHeightPrev;

	// 0x394
	s16 axisRotationY;

	// 0x396
	s16 axisRotationX;

	// 0x398
	s16 oh_no_anotherFiller;

	// 0x39A
	s16 angle;

	// 0x39C
	s16 baseSpeed;

	// 0x39E
	s16 fireSpeed;

	// 0x3A0
	int xSpeed;

	// 0x3A4
	int ySpeed;

	// 0x3A8
	int zSpeed;

	// 0x3AC
	s16 unkVectorX;
	// 0x3AE
	s16 unkVectorY;
	// 0x3B0
	s16 unkVectorZ;
	// 0x3B2
	s16 unk_offset3B2;

	// 0x3B4
	s16 rotationSpinRate; // again?

	// 0x3B6
	// both related to EngineSound
	// 0x3b6 controls the ¿volume? of the Engine Sound*
	// 0x3b8 controls the base pitch of the Engine Sound
	// The final pitch calculation where these vars are used
	// is also affected by something else I couldn't find
	s16 fill_3B6[2];

	// 0x3BA
	// in japanese VehFire_Increment
	s16 japanTurboUnknown;

	// 0x3BC
	// in VehPhysProc_Driving_PhysLinear
	s16 unkSpeedValue1;

	// 0x3BE
	// in VehPhysProc_Driving_PhysLinear
	s16 unkSpeedValue2;

	// 0x3C0
	// in VehPhysProc_Driving_PhysLinear
	s16 mashingXMakesItBig;

	// 0x3C2
	// in VehPhysProc_Driving_PhysLinear
	s16 mashXUnknown;

	// 0x3C4
	// in VehPhysProc_Driving_PhysLinear
	s16 unknowndriverBaseSpeed;

	// 0x3C6 0x3C8
	// in VehPhysProc_Driving_PhysLinear,
	// and VehPhysForce_AccelTerrainSlope,
	// and VehPhysProc_SpinLast_Update
	s16 turnAngleCurr;
	s16 turnAnglePrev;

	// 0x3CA
	s16 unk3CA;

	// 0x3CC
	// from VehPhysForce_CollideDrivers
	SVec3 accel;

	// 0x3D2
	s16 unk_LerpToForwards;

	// 0x3d4
	// This is a UNION between kart states
	s16 unk3D4[3];

	// 0x3DA
	// also drift direction
	s16 multDrift;

	// 0x3DC
	// turbo_MeterRoomLeft has values 0-1000 (dec),
	// can go negative and it draws farther left
	s16 turbo_MeterRoomLeft;

	// 0x3DE
	s16 turbo_outsideTimer;

	// 0x3E0
	s16 VehFire_AudioCooldown;

	// 0x3E2
	s16 reserves;

	// 0x3E4
	s16 fireSpeedCap;

	// 0x3E6
	s16 numFramesSpentSteering;

	// 0x3E8
	s16 forwardDir;

	// 0x3EA
	s16 previousFrameMultDrift; // previous frame or just repeat?

	// 0x3ec
	s16 timeUntilDriftSpinout;

	// 0x3ee
	s16 distanceFromGround;

	// 0x3F0
	// 10-frame buffer
	s16 jump_TenBuffer;

	// 0x3F2
	// so you can't spam jump too fast
	s16 jump_CooldownMS;

	// 0x3F4
	// time since driver left quadblock,
	// the speedrunners call this "coyote jump"
	s16 jump_CoyoteTimerMS;

	// 0x3F6
	// if not zero, and if touch ground,
	// it forces player to jump
	s16 jump_ForcedMS;

	// 0x3F8
	s16 jump_InitialVelY;

	// 0x3FA
	s16 jump_unknown;

	// 0x3FC
	s16 jump_LandingBoost;

	// 0x3FE
	s16 set_0xF0_OnWallRub;

	// 0x400
	s16 NoInputTimer;

	// 0x402
	s16 burnTimer;

	// 0x404
	s16 squishTimer;

	// 0x406
	s16 StartDriving_0x60;

	// 0x408
	s16 StartRollback_0x280;

	// 0x40A
	s16 unknownTraction;

	// 0x40C
	// when jumping and when hitting ground
	s16 jumpSquishStretch;

	s16 unk40E;

	// 0x410
	// used to calculate the other ^^
	s16 jumpSquishStretch2;

	s16 unk412;

	// 0x414 (physics/terrain related)
	s16 filler_short;

	/* Constant table - Metaphysics */

	// 0x416 - 0x00
	s16 const_Gravity; // OK

	// 0x418 - 0x01
	s16 const_JumpForce; // OK

	// 0x41A - 0x02
	s16 const_PedalFriction_Perpendicular; // OK applied sideways to the kart

	// 0x41C - 0x03
	s16 const_PedalFriction_Forward; // OK

	// 0x41E - 0x04
	s16 const_NoPedalFriction_Perpendicular; // OK applied sideways to the kart

	// 0x420 - 0x05
	s16 const_NoPedalFriction_Forward; // OK

	// 0x422 - 0x06
	s16 const_BrakeFriction; // OK

	// 0x424 - 0x07
	s16 const_DriftCurve; // OK

	// 0x426 - 0x08
	s16 const_DriftFriction; // OK

	// 0x428 - 0x09
	s16 const_Accel_ClassStat; // OK

	// 0x42A - 0x0A
	s16 const_Accel_Reserves; // OK

	// 0x42C - 0x0B
	s16 const_Speed_ClassStat; // OK

	// 0x42E - 0x0C
	s16 const_AccelSpeed_ClassStat; // OK

	// 0x430 - 0x0D
	s16 const_SingleTurboSpeed; // OK

	// 0x432 - 0x0E
	s16 const_SacredFireSpeed; // OK

	// 0x434 - 0x0F
	s16 const_BackwardSpeed; // OK

	// 0x436 - 0x10
	// for aku and uka weapons
	s16 const_MaskSpeed;

	// 0x438 - 0x11
	s16 const_DamagedSpeed;

	// 0x43A - 0x12
	u8 const_TurnRate; // OK

	// 0x43B - 0x13
	u8 const_BackwardTurnRate; // OK

	// 0x43C - 0x14
	s16 const_TurnDecreaseRate; // OK

	// 0x43E - 0x15
	s16 const_TurnInputDelay; // OK

	// 0x440 - 0x16
	s16 const_unk440;

	// 0x442 - 0x17
	s16 const_TerminalVelocity; // OK

	// 0x444 - 0x18
	s16 const_unk444;

	// 0x446 - 0x19
	s8 const_SteerAccel_Stage4_FirstFrame;

	// 0x447 - 0x1A
	s8 const_SteerAccel_Stage2_FirstFrame;

	// 0x448 - 0x1B
	s8 const_SteerAccel_Stage2_FrameLength;

	// not part of metaPhys
	u8 boolFirstFrameSinceRevEngine;

	// 0x44a - 0x1C
	s16 const_SteerAccel_Stage1_MaxSteer;

	// 0x44c - 0x1D
	s16 const_SteerAccel_Stage1_MinSteer;

	// 0x44e - 0x1E (const 0x80)
	s16 unk44e;

	// 0x450 - 0x1F (const zero)
	s16 unk450;

	// 0x452, 0x454 - 0x20, 0x21
	// exaggerate model rotation while steering
	s16 const_modelRotVelMax; // OK
	s16 const_modelRotVelMin;

	// 0x456 - [part of 0x22?]
	// either unused, or a s16,
	// and then 457 is just the top
	// byte of the s16
	char unusedPadding;

	// all related to VehPhysGeneral_LerpToForwards
	// only affected by steering without sliding
	u8 angleMaxCounterSteer; // 0x22 kart model angle lerp rotation Limit max*
	u8 unk458;               // 0x23 kart model angle lerp rotation Limit min*
	u8 unk459;               // 0x24 kart model angle lerp rotation strength/ratio max*
	u8 unk45a;               // 0x25 kart model angle lerp rotation strength/ratio min*

	char unk45b; // unused? skips straight to 0x45c

	// 0x45c, 0x45d - 0x26, 0x27
	// resist turning at low speed
	u8 const_turnResistMin;
	u8 const_turnResistMax;

	// 0x45e, 0x45f - 0x28, 0x29
	// hold drift in direction it started,
	// compared to switching direction "SwitchWay"
	s8 const_SteerVel_DriftSwitchWay;
	s8 const_SteerVel_DriftStandard;

	// all VehPhysProc_PowerSlide_PhysAngular
	// or VehPhysProc_PowerSlide_Finalize
	s8 unk460; // 0x460 - 0x2A
	s8 unk461; // 0x461 - 0x2B
	u8 unk462; // 0x462 - 0x2C

	// 0x463 - 0x2D
	u8 const_Drifting_FramesTillSpinout;

	// 0x464, 0x466, 0x468 - 0x2E, 0x2F, 0x30
	s16 unk464; // impact turning?
	s16 unk466; // impact turning?
	s16 const_Drifting_CameraSpinRate;

	// 0x46A, 0x46B - 0x31, 0x32
	u8 unk46a; // destinedRot max lerping
	u8 unk46b; // destinedRot min lerping?

	// 0x46C, 0x46E, 0x470, 0x472, 0x474 - 0x33, 0x34, 0x35, 0x36, 0x37
	s16 unk46c;
	s16 unk46e;
	s16 unk470; // sliding max angle rotation
	s16 unk472; // sliding min angle rotation
	s16 unk474; // sliding angle rotation factor?*

	// 0x476, 0x477, 0x478, 0x479, 0x47A - 0x38, 0x39, 0x3A, 0x3B, 0x3C
	u8 const_turboMaxRoom;        // point where turbo meter is empty
	u8 const_turboLowRoomWarning; // point where turbo turns red
	u8 const_turboFullBarReserveGain;
	u8 unk479; // 582 related -- somehow affects the force of the next variable (0x47a)
	u8 unk47A; // 582 related -- defines the angle when doing a turbo boost

	char unk47B; // unused? metaphys skips straight to 0x47C

	// 0x47C, 0x47E, 0x480 - 0x3D, 0x3E, 0x3F
	s16 unk47C;
	s16 unk47E;
	s16 unk480;

	// 0x484 - last of "main" constants,
	// skip over 482, which is non-const

	// 0x482
	// 1st, 2nd, 3rd, etc
	s16 driverRank;

	// 0x484 - MetaPhys stat no. 0x40
	// Used in Aug4 and Aug14
	int const_prototypeKey;

	// 0x484 - last of constants

	// 0x488
	u32 distanceToFinish_curr;

	// 0x48C
	u32 distanceToFinish_checkpoint;

	// 0x490
	u32 distanceDrivenBackwards;

	// 0x494
	// 494=char, 495=char
	u8 unknown_lap_related[2];

	// 0x496
	u16 engineVol;

	// 0x498
	struct Instance *instBigNum;

	// 0x49c
	struct Instance *instFruitDisp;

	// 0x4a0
	// raincloud when you hit red potion
	struct Thread *thCloud;

	// 0x4a4
	// pointer Tracking thread that
	// is chasing this driver (missile/warpball)
	struct Thread *thTrackingMe;

	// 0x4a8
	// Papu pyramid plants, see 231.c
	struct Thread *plantEatingMe;

	// 0x4ac
	int damageColorTimer;

	struct
	{
		// 0x4b0
		int cooldown;

		// 0x4b4
		s16 startX;
		s16 startY;

	} PickupTimeboxHUD;

	struct
	{
		// 0x4b8
		int cooldown;

		// 0x4bc
		s16 startX;
		s16 startY;

		// 0x4c0
		int numCollected;

	} PickupWumpaHUD;

	struct
	{
		// 0x4c4
		s16 cooldown;

		// 0x4c6
		s16 modelID;

		// 0x4c8
		s16 startX;
		s16 startY;

		// 0x4cc
		int numCollected;

	} PickupLetterHUD;

	struct
	{
		// 0x4d0
		int cooldown;

		// 0x4d4
		s16 startX;
		s16 startY;

		// 0x4d8
		int unk;

		// 0x4dc
		int scoreDelta; // -1, 0, 1

		// 0x4e0
		int juicedUpCooldown;

		// 0x4e4
		int numLives;

		// 0x4e8
		int teamID;

	} BattleHUD;

	// 0x4ec
	int framesSinceRaceEnded_forThisDriver;

	// 0x4f0
	// naughty dog bug
	s16 quip1;

	// 0x4f2
	s16 quip2;

	// 0x4f4
	s16 quip3;

	// 0x4f6
	s16 quip4;

	// 0x4f8
	struct Instance *wakeInst;

	// 0x4fc
	s16 wakeScale;

	// 0x4fe
	// 0, 1, 2, depending on rev level
	char revEngineState;

	// 0x4ff
	char ChangeState_param2;

	// 0x500
	int ChangeState_param3;

	// 0x504
	int ChangeState_param4;

	// 0x508
	// backup of alpha, used for turbo fire
	s16 alphaScaleBackup;

	// 0x50A
	s16 driverRankItemValue;

	// 0x50C
	char numTimesAttackingPlayer[8];

	// 0x514
	int timeElapsedInRace;

	// 0x518
	int distanceDriven;

	// 0x51c
	int timeSpentWithHighSpeed;

	// 0x520
	int timeSpentReversing;

	// 0x524
	int timeSpentInMud;

	// 0x528
	int timeSpentInLastPlace;

	// 0x52C
	int timeSpentInTenWumpa;

	// 0x530
	int timeSpentAgainstWall;

	// 0x534
	int timeSpentUsingReserves;

	// 0x538
	int timeSpentDrifting;

	// 0x53C
	int timeSpentJumping;

	// 0x540
	int timeSpentSpinningOut;

	// 0x544
	int timeSpentSquished;

	// 0x548
	int timeSpentEaten;

	// 0x54c
	int timeSpentBurnt;

	// 0x550 (not an int)
	s16 highestJump;

	// 0x552
	s16 longestShot;

	// 0x554
	s16 numberOfJumps;

	// 0x556
	char numTimesMovingPotionHitSomeone;

	// 0x557
	char numTimesMissileHitSomeone;

	// 0x558
	char numTimesClockWeaponUsed;

	// 0x559
	char numTimesAttacking;

	// 0x55a
	char numTimesBombsHitSomeone;

	// 0x55b
	char numTimesSquishedSomeone;

	// 0x55c
	char numTimesMissileLaunched;

	// 0x55d
	char numTimesMissileHitYou;

	// 0x55e
	char numTimesBombHitYou;

	// 0x55f
	char numTimesMotionlessPotionHitYou;

	// 0x560
	char numTimesAttackedByPlayer[8];

	// 0x568
	char numTimesHitWeaponBox;

	// 0x569
	char numTimesWumpa;

	// 0x56a
	char numTimesMaskGrab;

	// 0x56b
	// padding for the next int
	char unused_alignment_56b;

	// example, is this ever used btw?
	// lngIndex "Hit Most By..."
	// characterID "Crash Bandicoot"

	// 0x56c
	s16 *EndOfRaceComment_ptrQuip;

	// 0x570
	int EndOfRaceComment_characterID;

	// 0x574
	int NumMissilesComparedToNumAttacks;

	// 0x578
	// probably for "comeback kid!" comment
	int TimeWinningDriverSpentLastPlace;

	// 0x57c
	int numTimesAttacked;

	// ==== End of Quip Data ====

	// 0x580 - 0x598
	union
	{
		// state 2
		struct
		{
			// 0x580
			// used to determine spinout
			s16 numFramesDrifting;

			// 0x582
			s16 driftBoostTimeMS;

			// 0x584
			s16 driftTotalTimeMS;

			// 0x586
			s8 numBoostsAttempted;

			// 0x587
			s8 numBoostsSuccess;

			// == end ==

		} Drifting;

		// state 2
		struct
		{
			// 0x580
			s16 driftSpinRate;

			// 0x582
			s16 spinDir;

			// more?

		} Spinning;

		// state 4
		struct
		{
			// 0x580
			// object connected to thread
			struct MaskHeadWeapon *maskObj;

			// == Needs More Research ==

			// 0x584
			int boostMeter;

			// 0x588
			int fireLevel;

			// 0x58c
			s16 timeMS;

			// 0x58e
			s16 unk58e;

			// 0x590
			s16 unk590;

			// 0x592
			u8 unk[2];

			// 0x594
			int boolMaskGrab;

			// == end ==

		} RevEngine;

		// state 5
		struct
		{
			// 0x580
			// object connected to thread
			struct MaskHeadWeapon *maskObj;

			// 0x584
			s16 AngleAxis_NormalVec[3];

			// 0x58a
			s16 animFrame;

			// 0x58c
			u8 boolParticlesSpawned;
			u8 boolStillFalling;
			u8 boolLiftingPlayer;
			u8 boolWhistle;

			// 0x590
			u8 unused[8];

		} MaskGrab;

		// state 5
		struct
		{
			// 0x580
			u8 boolInited;

			// == end ==

		} EatenByPlant;

		// state 6
		struct
		{
			u8 boolPlayBackwards;
		} Blasted;

		// state 10
		struct
		{
			// 0x580
			int timer;
			// 0x584
			int heightOffset;
			// 0x588
			int quadHeight;
			// 0x58c
			int numParticle;
			// 0x590
			int beamHeight;
		} Warp;

	} KartStates;

	// 0x598
	// === Robotcar and Ghost ===
	struct BotData botData;

	// ===========================================

	// NTSC is 0x62C bytes large
	// PAL is 0x630 bytes large

	// ===========================================

	// 0x62C - 0x670 reserved for ghost

	// 0x62C
	struct GhostTape *ghostTape;

	// 0x630
	s16 ghostID;

	// 0x632
	s16 ghostBoolInit;

	// 0x634
	s16 ghostBoolStarted;
	s16 unk636;

	// 0x638
	// end of ghost struct (as determined by memset)

	// 0x30 in between are never used, "0x670" was simply the
	// largest Naughty Dog would allow themselves to push the driver,
	// but it is not the "real" size

	// 0x668 - size of pool object, minus object pool pointers
	// 0x670 - size of pool object
};

enum
{
	DRIVER_NTSC_RETAIL_SIZE = 0x62c,
};

_Static_assert(sizeof(struct MetaPhys) == 0x1C);

_Static_assert(sizeof(struct BotData) == 0x94);

_Static_assert(offsetof(struct Driver, ghostTape) == DRIVER_NTSC_RETAIL_SIZE);
_Static_assert(offsetof(struct Driver, rotCurr.x) == 0x2ec);
_Static_assert(offsetof(struct Driver, rotCurr.y) == 0x2ee);
_Static_assert(offsetof(struct Driver, AxisAngle3_normalVec) == 0x370);
