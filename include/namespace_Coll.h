struct BoundingBox
{
	s16 min[3];
	s16 max[3];
};

enum CollSearchFlags
{
	COLL_SEARCH_TEST_INSTANCES = 0x1,
	COLL_SEARCH_HIGH_LOD = 0x2,
	COLL_SEARCH_REUSE_NORMALS = 0x8,
	COLL_SEARCH_WALL_PROJECTION_DONE = 0x10,
	COLL_SEARCH_REPEAT_SCRUB = 0x20,
	COLL_SEARCH_FORCE_INSTANCE_HIT = 0x40,
};

struct BspSearchVertex
{
	// 0x0
	s16 pos[3];

	// 0x6
	// FUN_8001f2dc - COLL_FIXED_TRIANGL_GetNormVec
	// FUN_8001ef50 - COLL_FIXED_TRIANGL_TestPoint
	u16 flags;

	// 0x8
	struct LevVertex *pLevelVertex;

	// 0xC
	s16 normalVec[4];

	// 0x14 large
};

struct BspSearchTriangle
{
	struct QuadBlock *quadblock;
	s32 triangleID;
	s32 numCollision;
};

struct BspSearchResult
{
	// 0x0
	s16 hitPos[3];

	// 0x6
	s16 normalAxis;

	// 0x8
	s16 normalVec[4];

	// 0x10
	s16 pushOut[3];

	// 0x16
	s8 reorderResult;
	u8 triangleID;

	// 0x18
	struct QuadBlock *ptrQuadblock;
};

// can be stored in normal RAM,
// usually 1f800108
struct ScratchpadStruct
{
	struct
	{
		// this "pos" for threads: center of object
		// this "pos" for quadblock: posMin of object

		// 0x0
		s16 pos[3];
		s16 hitRadius;

		// 0x8
		s32 hitRadiusSquared;

		// 0xC
		s32 modelID;
	} Input1;

	// 0x10
	union
	{
		// This is why pointer 1f800118 gets passed
		struct
		{
			// this "pos" for quadblock: posMax of object,
			// hitRadius could just be a copy

			// 0x10
			s16 pos[3];
			s16 hitRadius;

			// 0x18
			s32 hitRadiusSquared;

			// 0x1C
			s16 hitPos[3];

			// 0x22
			u16 searchFlags;

			// 0x24
			u32 qbFlagsWanted;

			// 0x28
			u32 qbFlagsIgnored;

		} QuadBlockColl;

		// when using this,
		// rest of struct is ignored
		struct
		{
			// 0x10
			s16 distance[3];
			s16 unk;

			// 0x18
			struct Thread *thread;

			// 0x1c
			s16 min[3];
			s16 max[3];

			// 0x28
			// could be non-union 0x28
			void *funcCallback;

		} ThBuckColl;
	} Union;

	// 0x2C
	struct mesh_info *ptr_mesh_info;

	// 0x30
	struct BoundingBox bbox;

	// 0x3C
	s16 numTrianglesTested;

	// 0x3e
	s16 boolDidTouchQuadblock;

	// 0x40
	// s16 boolDidTouch_What?
	s16 unk40;

	// 0x42
	s16 boolDidTouchHitbox;

	// 0x44
	struct mesh_info *ptr_mesh_info_2;

	// 0x48
	struct BSP *bspHitbox;

	// 0x4c
	struct BspSearchResult candidate;

	// 0x68
	struct BspSearchResult hit;

	// 0x84
	// 0x1000 means the whole segment is clear; lower values are the first hit fraction.
	s32 hitFraction;

	// 0x88
	// This can happen 15 times cause of FUN_80020410,
	// so this prevents any duplicate collisions
	struct BSP *bspHitboxesHit[15];

	// 0xc4
	s32 numBspHitboxesHit;

	// 0xc8, 0xca,
	s16 barycentrics[2];

	// 0xcc, 0xd0, 0xd4
	struct LevVertex *levVertHit[3];

	// 0xd8, 0xdc, 0xe0
	struct BspSearchVertex *bspSearchVertHit[3];

	// 0xe4, extra candidate hit delta slot
	s32 unkE4;

	// vec3, bsp->0x10 - position (FUN_8001d0c4)
	// 0xe8, 0xea, 0xec, 0xee
	s16 unkVecE8[4];

	// 0xf0
	struct BspSearchVertex bspSearchVert[9];

	// 0x1a4 - quadblock action flags
	// 0x1a8 - fastmath normalization
	char dataOutput[0x68];

	// offset 0x1c4
	// FUN_8001d0c4

	// 0x20C -- size of struct
};

// only stored in Scratchpad
// FUN_80020334
struct ScratchpadStructExtended
{
	// --- top half is for COLL ---

	struct ScratchpadStruct scratchpadStruct;

	// 0x20C
	struct BspSearchTriangle bspSearchTriangle[0xF];

	// 0x2c0
	s32 numTriangles;

	// 0x2c4 - 1f8003cc
	s32 unk1[2];


	// --- the rest is for pushBuffer funcs ---


	// 0x2cc - 1f8003d4
	MATRIX cameraMatrix;

	// 0x2ec - 1f8003f4
	s16 cameraRot[3];

	// 0x2f2 - 1f8003fa
	s16 unk2;

	// 0x2f4 - 1f8003fc
	s32 unk3;

	// 1f800400 end of memory
};

_Static_assert(sizeof(struct BoundingBox) == 0xC);
_Static_assert(sizeof(struct BspSearchVertex) == 0x14);
_Static_assert(sizeof(struct BspSearchTriangle) == 0xC);
_Static_assert(sizeof(struct BspSearchResult) == 0x1C);
_Static_assert(offsetof(struct BspSearchResult, hitPos) == 0x0);
_Static_assert(offsetof(struct BspSearchResult, normalAxis) == 0x6);
_Static_assert(offsetof(struct BspSearchResult, normalVec) == 0x8);
_Static_assert(offsetof(struct BspSearchResult, pushOut) == 0x10);
_Static_assert(offsetof(struct BspSearchResult, reorderResult) == 0x16);
_Static_assert(offsetof(struct BspSearchResult, triangleID) == 0x17);
_Static_assert(offsetof(struct BspSearchResult, ptrQuadblock) == 0x18);
_Static_assert(sizeof(struct ScratchpadStruct) == 0x20C);
_Static_assert(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.searchFlags) == 0x22);
_Static_assert(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.qbFlagsWanted) == 0x24);
_Static_assert(offsetof(struct ScratchpadStruct, Union.QuadBlockColl.qbFlagsIgnored) == 0x28);
_Static_assert(offsetof(struct ScratchpadStruct, numTrianglesTested) == 0x3C);
_Static_assert(offsetof(struct ScratchpadStruct, candidate) == 0x4C);
_Static_assert(offsetof(struct ScratchpadStruct, candidate.triangleID) == 0x63);
_Static_assert(offsetof(struct ScratchpadStruct, hit) == 0x68);
_Static_assert(offsetof(struct ScratchpadStruct, hit.reorderResult) == 0x7E);
_Static_assert(offsetof(struct ScratchpadStruct, hit.triangleID) == 0x7F);
_Static_assert(offsetof(struct ScratchpadStruct, hit.ptrQuadblock) == 0x80);
_Static_assert(offsetof(struct ScratchpadStruct, hitFraction) == 0x84);
_Static_assert(offsetof(struct ScratchpadStruct, bspHitboxesHit) == 0x88);
_Static_assert(offsetof(struct ScratchpadStruct, numBspHitboxesHit) == 0xC4);
