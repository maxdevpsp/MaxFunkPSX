/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "smspook.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../main.h"

//SMSpook character structure
enum
{
	SMSpook_ArcMain_Idle0,
	SMSpook_ArcMain_Idle1,
	SMSpook_ArcMain_Idle2,
	SMSpook_ArcMain_Left,
	SMSpook_ArcMain_Down,
	SMSpook_ArcMain_Up,
	SMSpook_ArcMain_Right,
	
	SMSpook_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[SMSpook_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
} Char_SMSpook;

//SMSpook character definitions
static const CharFrame char_smspook_frame[] = {
	{SMSpook_ArcMain_Idle0, {  0,   0,  97, 137}, { 61, 132}}, //0 idle 1
	{SMSpook_ArcMain_Idle0, { 98,   0,  94, 136}, { 59, 131}}, //1 idle 2
	{SMSpook_ArcMain_Idle1, {  0,   0,  90, 121}, { 55, 117}}, //2 idle 3
	{SMSpook_ArcMain_Idle1, { 91,   0,  89, 124}, { 56, 120}}, //3 idle 4
	{SMSpook_ArcMain_Idle2, {  0,   0, 124, 133}, { 76, 127}}, //4 idle 5
	{SMSpook_ArcMain_Idle2, {125,   0, 121, 131}, { 74, 127}}, //5 idle 6
	
	{SMSpook_ArcMain_Left, {  0,   0, 140, 134}, {103, 130}}, //6 left 1
	{SMSpook_ArcMain_Left, {115, 123, 140, 132}, {100, 128}}, //7 left 2
	
	{SMSpook_ArcMain_Down, {  0,   0, 117, 102}, { 66,  99}}, //8 down 1
	{SMSpook_ArcMain_Down, {118,   0, 113, 101}, { 65,  98}}, //9 down 2
	
	{SMSpook_ArcMain_Up, {  0,   0, 101, 143}, { 70, 139}}, //10 up 1
	{SMSpook_ArcMain_Up, {102,   0, 102, 142}, { 69, 138}}, //11 up 2
	
	{SMSpook_ArcMain_Right, {  0,   0, 111, 130}, { 42, 128}}, //12 right 1
	{SMSpook_ArcMain_Right, {112,   0, 110, 133}, { 39, 131}}, //13 right 2
};

static const Animation char_smspook_anim[CharAnim_Max] = {
	{2, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}}, //CharAnim_Idle
	{2, (const u8[]){ 6,  7, ASCR_BACK, 1}},          //CharAnim_Left
	{2, (const u8[]){ 0,  1,  2,  3, ASCR_BACK, 1}},  //CharAnim_LeftAlt
	{2, (const u8[]){ 8,  9, ASCR_BACK, 1}},          //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}}, //CharAnim_DownAlt
	{2, (const u8[]){10, 11, ASCR_BACK, 1}},          //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}}, //CharAnim_UpAlt
	{2, (const u8[]){12, 13, ASCR_BACK, 1}},          //CharAnim_Right
	{2, (const u8[]){ 4,  5,  2,  3, ASCR_BACK, 1}},  //CharAnim_RightAlt
};

//SMSpook character functions
void Char_SMSpook_SetFrame(void *user, u8 frame)
{
	Char_SMSpook *this = (Char_SMSpook*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_smspook_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_SMSpook_Tick(Character *character)
{
	Char_SMSpook *this = (Char_SMSpook*)character;
	
	//Perform smspooky dance
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
	{
		Character_CheckEndSing(character);
		
		if (stage.flag & STAGE_FLAG_JUST_STEP)
		{
			if ((Animatable_Ended(&character->animatable) || character->animatable.anim == CharAnim_LeftAlt || character->animatable.anim == CharAnim_RightAlt) &&
				(character->animatable.anim != CharAnim_Left &&
				 character->animatable.anim != CharAnim_Down &&
				 character->animatable.anim != CharAnim_Up &&
				 character->animatable.anim != CharAnim_Right) &&
				(stage.song_step & 0x3) == 0)
				character->set_anim(character, CharAnim_Idle);
		}
	}
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_SMSpook_SetFrame);
	Character_Draw(character, &this->tex, &char_smspook_frame[this->frame]);
}

void Char_SMSpook_SetAnim(Character *character, u8 anim)
{
	//Set animation
	if (anim == CharAnim_Idle)
	{
		if (character->animatable.anim == CharAnim_LeftAlt)
			anim = CharAnim_RightAlt;
		else
			anim = CharAnim_LeftAlt;
		character->sing_end = FIXED_DEC(0x7FFF,1);
	}
	else
	{
		Character_CheckStartSing(character);
	}
	Animatable_SetAnim(&character->animatable, anim);
}

void Char_SMSpook_Free(Character *character)
{
	Char_SMSpook *this = (Char_SMSpook*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_SMSpook_New(fixed_t x, fixed_t y)
{
	//Allocate smspook object
	Char_SMSpook *this = Mem_Alloc(sizeof(Char_SMSpook));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_SMSpook_New] Failed to allocate smspook object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_SMSpook_Tick;
	this->character.set_anim = Char_SMSpook_SetAnim;
	this->character.free = Char_SMSpook_Free;
	
	Animatable_Init(&this->character.animatable, char_smspook_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_i = 2;
	
	this->character.focus_x =  FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-80,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\SMSPOOK.ARC;1");
	
	const char **pathp = (const char *[]){
		"idle0.tim", //SMSpook_ArcMain_Idle0
		"idle1.tim", //SMSpook_ArcMain_Idle1
		"idle2.tim", //SMSpook_ArcMain_Idle2
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);

	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
