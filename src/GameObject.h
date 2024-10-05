typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef struct {
    /* 0x00 */ float x;
    /* 0x04 */ float y;
    /* 0x08 */ float z;
} Vec3f; /* 0xC */

enum eStaggerType {
    NONE = 0,
    FULL = 1, /* damage multiplier active */
    PARTIAL = 2, /* trash mob armor */
    FULL2 = 3, /* damage multiplier active */
};

enum eWillBarHalf {
    LEFT = 0,
    RIGHT = 1
};

typedef struct {
    /* 0x00 */ u8 unk_0x00[0x40];
    /* 0x40 */ u32 Health;
    /* 0x44 */ u8 unk_0x44[0x14];
    /* 0x58 */ u32 StaggerType; /* eStaggerType */
    /* 0x5C */ u8 Prone; /* Nonzero when entity is laying on the ground, used to detect when you can Mortal Blow. Forcing this to a nonzero value allows you to mortal blow enemies who would otherwise be unable to go prone. */
    /* 0x5D */ u8 unk_0x5D[0x7];
    /* 0x64 */ u32 Will;
    /* 0x68 */ float StaggerTimer; /* Time remaining for the current stagger. This does not include half staggers, but does include trash mobs' super armor. */
    /* 0x6C */ float StaggerTimer1; /* unsure what this is for, it is set with eStaggerType::FULL, possibly initial stagger timer, or stagger timer length? */
    /* 0x70 */ u8 unk_0x70[0x4];
    /* 0x74 */ u8 WillBarHalf; /* For enemies with will bars, determines the half of the will bar we are using. 0 = right, 1 = left. When Will reaches 0, if it is on the right, it switches to the left, and Will resets. */
    /* 0x75 */ u8 WillBarHalf1; /* Similar to the above value, but I'm not sure of it's purpose. */
    /* 0x76 */ u8 unk_0x76[0x2];
    /* 0x78 */ u32 StaggerDamageDealt; /* Raw damage dealth when an enemy is fully staggered. Used to determine damage multiplier? */
    /* 0x7C */ u32 StaggerDamageDealtScaled; /* Damage dealth after the damage multiplier when an enemy is fully staggered, what is displayed in the "Stagger Damage" message. */
    /* 0x80 */ u8 unk_0x80[0xAD8];
} CombatDetail; /* 0xB58 (from ctor) */

typedef struct {
    /* 0x000 */ u8 unk_0x000[0x1A0];
    /* 0x1A0 */ u32 CollisionFlags; // Bitfield? (1 << 0) = the actor cannot be hit by direct attacks, (1 << 1) = the actor cannot be hit by aoes?
    /* 0x1A4 */ u8 unk_0x1A4[0x3C];
} UnkCombatDetail; /* 0x1E0 (from ctor) */

typedef struct {
    /* 0x000 */ UnkCombatDetail unk_0x00;
    /* 0x1E0 */ CombatDetail* pCombat;
    /* 0x1E8 */ CombatDetail Combat;
} CombatDetail_UnkParentStruct1; /* Unknown total size */

typedef struct {
    /* 0x000 */ u8 unk_0x00[0x20]; // pointers
    /* 0x020 */ CombatDetail_UnkParentStruct1 unk_0x20;
} CombatDetail_UnkParentStruct; /* 0x4598 (from ctor) */

// has position data, seems to be separate from game objects?
typedef struct {
    /* 0x00 */ u8 unk_0x00[0x30];
    /* 0x30 */ Vec3f Position;
    /* 0x3C */ u8 unk_0x3C[0x84];
} UnkTransformStruct; /* 0xC0 */

/*
function which applies damage to entities:
aob: 48 89 5c 24 08 48 89 74 24 10 48 89 7c 24 18 41 56 48 83 ec ?? 8b fa
sig: u8 (CombatDetail* combat, s32 healthDelta)

function which applies will to entities:
aob: 48 89 5c 24 08 56 48 83 ec ?? 41 8a f1 48 8b d9 85 d2
sig: s32 (CombatDetail* combat, s32 willDelta, u8 arg3, u8 arg4)
*/

typedef struct {
    /* 0x0000 */ u8 unk_0x0000[0x8]; // vtable
    /* 0x0008 */ u64 TableIndex;
    /* 0x0010 */ u8 unk_0x0010[0x7288];
    /* 0x7298 */ CombatDetail* Combat;
    /* 0x72A0 */ u8 unk_0x72A0[0x2C90];
} Unk9f30Struct; /* 0x9f30 (from ctor) */
// (X + 0x58) + 0x7298 = CombatDetail

typedef struct {
    /* 0x00 */ u64 unk_0x00; // used as a counter
} Struct_Param2_Field_0x10;

typedef struct {
	/* 0x00 */ unsigned char unk_0x00[8];
	/* 0x08 */ Unk9f30Struct** unk_0x08; // object table
	/* 0x10 */ uintptr_t unk_0x10;
} Struct_Param2_Field0x00;

typedef struct {
	/* 0x00 */ Struct_Param2_Field0x00* unk_0x00;
    /* 0x08 */ uintptr_t unk_0x08; // used as counter limit, object table length?
    /* 0x10 */ Struct_Param2_Field_0x10* unk_0x10;
} Struct_Param2;

/*
object table(?) iterator
aob: 48 89 5c 24 ?? 57 48 83 ec ?? 48 8b 3a 48 8b da b8 ?? 00 00 00 f0 48 0f c1 43 ?? 48 3b 43 ?? 73 ?? 48 8b ?? ?? 48 8b ?? ?? e8 ?? ?? ?? ?? eb ?? 48 8b ?? ?? ?? 48 83 c4 ?? 5f c3 cc 48 8b 0a e9 ?? ?? ?? ?? 48 89 5c 24 ?? 48 89 74 24 ?? 57 48 83 ec ?? 48
sig: void (uintptr_t arg1, Struct_Param2* arg2)

example for how we can iterate:
u64 index = 0;
if (arg2 != NULL && arg2->unk_0x00 != NULL && arg2->unk_0x00->unk_0x08 != NULL) {
    while (1) {
        if (arg2->unk_0x08 <= index) {
            break;
        }
        Unk9f30Struct* entry = arg2->unk_0x00->unk_0x08[index];
        index++;
    }
}
*/
