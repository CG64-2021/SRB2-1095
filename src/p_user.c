// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief New stuff?
///	
///	Player related stuff.
///	Bobbing POV/weapon, movement.
///	Pending weapon.

#include "doomdef.h"
#include "d_event.h"
#include "d_net.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "s_sound.h"
#include "r_things.h"
#include "i_sound.h"
#include "d_think.h"
#include "r_sky.h"
#include "p_setup.h"
#include "m_random.h"
#include "i_video.h"
#include "p_spec.h"
#include "r_splats.h"
#include "z_zone.h"
#include "w_wad.h"

#include "hardware/hw3sound.h"

#ifdef HWRENDER
#include "hardware/hw_light.h"
#include "hardware/hw_main.h"
#endif

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP 32

// Masks for dumping extended debug info.
static char debuginfo[] = {
0x63,0x65,0x63,0x68,0x6f,0x66,0x6c,0x61,
0x67,0x73,0x20,0x30,0x0a,0x63,0x65,0x63,
0x68,0x6f,0x64,0x75,0x72,0x61,0x74,0x69,
0x6f,0x6e,0x20,0x35,0x0a,0x63,0x65,0x63,
0x68,0x6f,0x20,0x48,0x65,0x79,0x21,0x5c,
0x57,0x68,0x6f,0x27,0x73,0x20,0x6d,0x61,
0x6b,0x69,0x6e,0x67,0x20,0x61,0x6c,0x6c,
0x20,0x74,0x68,0x61,0x74,0x20,0x72,0x61,
0x63,0x6b,0x65,0x74,0x3f,0x21,0x0a,0x00};

static int yarly;

static void P_NukeAllPlayers(player_t *player);

//
// Movement.
//

// 16 pixels of bob
#define MAXBOB 0x100000

static boolean onground;

//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_Thrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, finecosine[angle]);

	if (!twodlevel)
		mo->momy += FixedMul(move, finesine[angle]);
}

#if 0
static inline void P_ThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx += FixedMul(move, finecosine[angle]);
	mo->momy += FixedMul(move, finesine[angle]);
}

static inline void P_VectorInstaThrust(fixed_t xa, fixed_t xb, fixed_t xc, fixed_t ya, fixed_t yb, fixed_t yc,
	fixed_t za, fixed_t zb, fixed_t zc, fixed_t momentum, mobj_t *mo)
{
	fixed_t a1, b1, c1, a2, b2, c2, i, j, k;

	a1 = xb - xa;
	b1 = yb - ya;
	c1 = zb - za;
	a2 = xb - xc;
	b2 = yb - yc;
	c2 = zb - zc;
/*
	// Convert to unit vectors...
	a1 = FixedDiv(a1,sqrt(FixedMul(a1,a1) + FixedMul(b1,b1) + FixedMul(c1,c1)));
	b1 = FixedDiv(b1,sqrt(FixedMul(a1,a1) + FixedMul(b1,b1) + FixedMul(c1,c1)));
	c1 = FixedDiv(c1,sqrt(FixedMul(c1,c1) + FixedMul(c1,c1) + FixedMul(c1,c1)));

	a2 = FixedDiv(a2,sqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
	b2 = FixedDiv(b2,sqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
	c2 = FixedDiv(c2,sqrt(FixedMul(a2,a2) + FixedMul(c2,c2) + FixedMul(c2,c2)));
*/
	// Calculate the momx, momy, and momz
	i = FixedMul(momentum, FixedMul(b1, c2) - FixedMul(c1, b2));
	j = FixedMul(momentum, FixedMul(c1, a2) - FixedMul(a1, c2));
	k = FixedMul(momentum, FixedMul(a1, b2) - FixedMul(a1, c2));

	mo->momx = i;
	mo->momy = j;
	mo->momz = k;
}
#endif

//
// P_InstaThrust
// Moves the given origin along a given angle instantly.
//
// FIXTHIS: belongs in another file, not here
//
void P_InstaThrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, finecosine[angle]);

	if (!twodlevel)
		mo->momy = FixedMul(move,finesine[angle]);
}

void P_InstaThrustEvenIn2D(mobj_t *mo, angle_t angle, fixed_t move)
{
	angle >>= ANGLETOFINESHIFT;

	mo->momx = FixedMul(move, finecosine[angle]);
	mo->momy = FixedMul(move, finesine[angle]);
}

//
// P_InstaZThrust
// Moves the given origin along a given angle instantly.
//
#if 0
static inline void P_InstaZThrust(mobj_t *mo, angle_t angle, fixed_t move)
{
	mo->momz = move;

	if (mo->player)
		mo->momz += (fixed_t)((((angle >> FRACBITS)*FRACUNIT)/128) * (mo->player->speed/32.0f));
}
#endif

// Returns a location (hard to explain - go see how it is used)
fixed_t P_ReturnThrustX(mobj_t *mo, angle_t angle, fixed_t move)
{
	//mo = NULL;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, finecosine[angle]);
}
fixed_t P_ReturnThrustY(mobj_t *mo, angle_t angle, fixed_t move)
{
	//mo = NULL;
	angle >>= ANGLETOFINESHIFT;
	return FixedMul(move, finesine[angle]);
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight(player_t *player)
{
	int angle;
	fixed_t bob;
	fixed_t pviewheight;
	mobj_t *mo;

	// Regular movement bobbing.
	// Should not be calculated when not on ground (FIXTHIS?)
	// OPTIMIZE: tablify angle
	// Note: a LUT allows for effects
	//  like a ramp with low health.

	mo = player->mo;

	player->bob = ((FixedMul(player->rmomx,player->rmomx)
		+ FixedMul(player->rmomy,player->rmomy))*NEWTICRATERATIO)>>2;

	if (player->bob > MAXBOB)
		player->bob = MAXBOB;

	if ((player->cheats & CF_NOMOMENTUM) || mo->z > mo->floorz)
	{
		player->viewz = mo->z + player->viewheight;
		if (player->viewz > mo->ceilingz - FRACUNIT)
			player->viewz = mo->ceilingz - FRACUNIT;
		return;
	}

	angle = (FINEANGLES/20*localgametic/NEWTICRATERATIO)&FINEMASK;
	bob = FixedMul(player->bob/2, finesine[angle]);

	// move viewheight
	pviewheight = cv_viewheight.value << FRACBITS; // default eye view height

	if (player->playerstate == PST_LIVE)
	{
		player->viewheight += player->deltaviewheight;

		if (player->viewheight > pviewheight)
		{
			player->viewheight = pviewheight;
			player->deltaviewheight = 0;
		}

		if (player->viewheight < pviewheight/2)
		{
			player->viewheight = pviewheight/2;
			if (player->deltaviewheight <= 0)
				player->deltaviewheight = 1;
		}

		if (player->deltaviewheight)
		{
			player->deltaviewheight += FRACUNIT/4;
			if (!player->deltaviewheight)
				player->deltaviewheight = 1;
		}
	}

	player->viewz = mo->z + player->viewheight + bob;

	if (player->viewz > mo->ceilingz-4*FRACUNIT)
		player->viewz = mo->ceilingz-4*FRACUNIT;
	if (player->viewz < mo->floorz+4*FRACUNIT)
		player->viewz = mo->floorz+4*FRACUNIT;
}

static fixed_t P_GridSnap(fixed_t value)
{
	fixed_t pos = value/cv_grid.value;
	const fixed_t poss = (pos/FRACBITS)<<FRACBITS;
	pos = (pos&FRACMASK) < FRACUNIT/2 ? poss : poss+FRACUNIT;
	return pos * cv_grid.value;
}

/** Decides if a player is moving.
  * \param pnum The player number to test.
  * \return True if the player is considered to be moving.
  * \author Graue <graue@oceanbase.org>
  */
boolean P_PlayerMoving(int pnum)
{
	player_t *p = &players[pnum];

	return gamestate == GS_LEVEL && p->mo && p->mo->health > 0
		&& (
			p->rmomx >= FRACUNIT/2 ||
			p->rmomx <= -FRACUNIT/2 ||
			p->rmomy >= FRACUNIT/2 ||
			p->rmomy <= -FRACUNIT/2 ||
			p->mo->momz >= FRACUNIT/2 ||
			p->mo->momz <= -FRACUNIT/2 ||
			p->climbing ||
			p->powers[pw_tailsfly] ||
			p->mfjumped ||
			p->mfspinning);
}

//
// P_ResetScore
//
// This is called when your chain is reset. If in
// Chaos mode, it displays what chain you got.
void P_ResetScore(player_t *player)
{
#ifdef CHAOSISNOTDEADYET
	if (gametype == GT_CHAOS && player->scoreadd >= 5)
		CONS_Printf("%s got a chain of %d!\n", player_names[player-players], player->scoreadd);
#endif

	player->scoreadd = 0;
}

//
// P_FindLowestMare
//
// Returns the lowest open mare available
//
byte P_FindLowestMare(void)
{
	thinker_t *th;
	mobj_t *mo2;
	byte mare = 255;

	if (gametype == GT_RACE)
		return 0;

	// scan the thinkers
	// to find the egg capsule with the lowest mare
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_EGGCAPSULE && mo2->health > 0)
		{
			const byte threshold = (byte)mo2->threshold;
			if (mare == 255)
				mare = threshold;
			else if (threshold < mare)
				mare = threshold;
		}
	}

	if (cv_debug)
		CONS_Printf("Lowest mare found: %d\n", mare);

	return mare;
}

//
// P_TransferToNextMare
//
// Transfers the player to the next Mare.
// (Finds the lowest mare # for capsules that have not been destroyed).
// Returns true if successful, false if there is no other mare.
//
boolean P_TransferToNextMare(player_t *player)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	int lowestaxisnum = -1;
	byte mare = P_FindLowestMare();
	fixed_t dist1, dist2 = 0;

	if (mare == 255)
		return false;

	if (cv_debug)
		CONS_Printf("Mare is %d\n", mare);

	player->mare = mare;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_AXIS)
		{
			if (mo2->threshold == mare)
			{
				if (closestaxis == NULL)
				{
					closestaxis = mo2;
					lowestaxisnum = mo2->health;
					dist2 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;
				}
				else if (mo2->health < lowestaxisnum)
				{
					dist1 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;

					if (dist1 < dist2)
					{
						closestaxis = mo2;
						dist2 = dist1;
					}
				}
			}
		}
	}

	if (closestaxis == NULL)
		return false;

	player->mo->target = closestaxis;
	return true;
}

//
// P_FindAxis
//
// Given a mare and axis number, returns
// the mobj for that axis point.
static mobj_t *P_FindAxis(int mare, int axisnum)
{
	thinker_t *th;
	mobj_t *mo2;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		// Axis things are only at beginning of list.
		if (!(mo2->flags2 & MF2_AXIS))
			return NULL;

		if (mo2->type == MT_AXIS)
		{
			if (mo2->health == axisnum && mo2->threshold == mare)
				return mo2;
		}
	}

	return NULL;
}

//
// P_FindAxisTransfer
//
// Given a mare and axis number, returns
// the mobj for that axis transfer point.
static mobj_t *P_FindAxisTransfer(int mare, int axisnum, mobjtype_t type)
{
	thinker_t *th;
	mobj_t *mo2;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		// Axis things are only at beginning of list.
		if (!(mo2->flags2 & MF2_AXIS))
			return NULL;

		if (mo2->type == type)
		{
			if (mo2->health == axisnum && mo2->threshold == mare)
				return mo2;
		}
	}

	return NULL;
}

//
// P_TransferToAxis
//
// Finds the CLOSEST axis with the number specified.
void P_TransferToAxis(player_t *player, int axisnum)
{
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis;
	int mare = player->mare;
	fixed_t dist1, dist2 = 0;

	if (cv_debug)
		CONS_Printf("Transferring to axis %d\nLeveltime: %d...\n", axisnum,leveltime);

	closestaxis = NULL;

	// scan the thinkers
	// to find the closest axis point
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_AXIS)
		{
			if (mo2->health == axisnum && mo2->threshold == mare)
			{
				if (closestaxis == NULL)
				{
					closestaxis = mo2;
					dist2 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;
				}
				else
				{
					dist1 = R_PointToDist2(player->mo->x, player->mo->y, mo2->x, mo2->y)-mo2->radius;

					if (dist1 < dist2)
					{
						closestaxis = mo2;
						dist2 = dist1;
					}
				}
			}
		}
	}

	if (!closestaxis)
		CONS_Printf("ERROR: Specified axis point to transfer to not found!\n%d\n", axisnum);
	else if (cv_debug)
		CONS_Printf("Transferred to axis %d, mare %d\n", closestaxis->health, closestaxis->threshold);

	player->mo->target = closestaxis;
}

//
// P_DeNightserizePlayer
//
// Whoops! Ran out of NiGHTS time!
//
static void P_DeNightserizePlayer(player_t *player)
{
	thinker_t *th;
	mobj_t *mo2;

	player->nightsmode = false;

//  if (player->mo->tracer)
//		P_RemoveMobj(player->mo->tracer);
	
	player->powers[pw_underwater] = 0;
	player->usedown = false;
	player->jumpdown = false;
	player->attackdown = false;
	player->walking = 0;
	player->running = 0;
	player->spinning = 0;
	player->jumping = 0;
	player->homing = 0;
	player->mo->target = NULL;
	player->mo->fuse = 0;
	player->speed = 0;
	player->mfstartdash = 0;
	player->mfjumped = 0;
	player->secondjump = 0;
	player->thokked = false;
	player->mfspinning = 0;
	player->dbginfo = 0;
	player->drilling = 0;
	player->transfertoclosest = 0;
	player->axis1 = NULL;
	player->axis2 = NULL;

	player->mo->flags &= ~MF_NOGRAVITY;

	player->mo->flags2 &= ~MF2_DONTDRAW;

	if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
	{
		if (cv_analog2.value)
			CV_SetValue(&cv_cam2_dist, 192);
		else
			CV_SetValue(&cv_cam2_dist, atoi(cv_cam2_dist.defaultvalue));
	}
	else if (player == &players[displayplayer])
	{
		if (cv_analog.value)
			CV_SetValue(&cv_cam_dist, 192);
		else
			CV_SetValue(&cv_cam_dist, atoi(cv_cam_dist.defaultvalue));
	}

	if (player->mo->tracer)
		P_SetMobjState(player->mo->tracer, S_DISS);
	P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
	player->nightsfall = true;

	// Check to see if the player should be killed.
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (!(mo2->type == MT_NIGHTSDRONE))
			continue;

		if (mo2->flags & MF_AMBUSH)
		{
			P_DamageMobj(player->mo, NULL, NULL, 10000);
			break;
		}
	}
}
//
// P_NightserizePlayer
//
// NiGHTS Time!
void P_NightserizePlayer(player_t *player, int time)
{
	int oldmare;

	player->usedown = false;
	player->jumpdown = false;
	player->attackdown = false;
	player->walking = 0;
	player->running = 0;
	player->spinning = 0;
	player->homing = 0;

	player->mo->fuse = 0;
	player->speed = 0;
	player->mfstartdash = 0;
	player->mfjumped = 0;
	player->secondjump = 0;
	player->thokked = false;
	player->mfspinning = 0;
	player->dbginfo = 0;
	player->drilling = 0;

	player->powers[pw_jumpshield] = 0;
	player->powers[pw_fireshield] = 0;
	player->powers[pw_watershield] = 0;
	player->powers[pw_bombshield] = 0;
	player->powers[pw_ringshield] = 0;

	player->mo->flags |= MF_NOGRAVITY;

	player->mo->flags2 |= MF2_DONTDRAW;

	if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
		CV_SetValue(&cv_cam2_dist, 320);
	else if (player == &players[displayplayer])
		CV_SetValue(&cv_cam_dist, 320);

	player->nightstime = time;
	player->bonustime = false;

	P_SetMobjState(player->mo->tracer, S_SUPERTRANS1);

	if (gametype == GT_RACE)
	{
		if (player->drillmeter < 48*20)
			player->drillmeter = 48*20;
	}
	else
	{
		if (player->drillmeter < 40*20)
			player->drillmeter = 40*20;
	}

	oldmare = player->mare;

	if (P_TransferToNextMare(player) == false)
	{
		int i;

		player->mo->target = NULL;

		for (i = 0; i < MAXPLAYERS; i++)
			P_DoPlayerExit(&players[i]);
	}

	if (oldmare != player->mare)
		player->mo->health = player->health = 1;

	player->nightsmode = true;
}

//
// P_ResetPlayer
//
// Useful when you want to kill everything the player is doing.
void P_ResetPlayer(player_t *player)
{
	player->mfspinning = 0;
	player->mfjumped = 0;
	player->secondjump = 0;
	player->gliding = 0;
	player->glidetime = 0;
	player->homing = 0;
	player->climbing = 0;
	player->powers[pw_tailsfly] = 0;
	player->thokked = false;
	player->onconveyor = 0;
	player->carried = 0;
}

//
// P_GivePlayerRings
//
// Gives rings to the player, and does any special things required.
// Call this function when you want to increment the player's health.
//
void P_GivePlayerRings(player_t *player, int num_rings, boolean flingring)
{
#ifdef PARANOIA
	if (player->mo)
	{
#endif
		player->mo->health += num_rings;
		player->health += num_rings;

		if (!flingring)
		{
			player->losscount = 0;
			player->totalring += num_rings;
		}
		else
		{
			if (player->mo->health > 2)
				player->losscount = 0;
		}

		// Can only get up to 999 rings, sorry!
		if (player->mo->health > 1000)
		{
			player->mo->health = 1000;
			player->health = 1000;
		}
#ifdef PARANOIA
	}
#endif
}

//
// P_GivePlayerLives
//
// Gives the player an extra life.
// Call this function when you want to add lives to the player.
//
void P_GivePlayerLives(player_t *player, int numlives)
{
	player->lives += numlives;

	if (player->lives > 999)
		player->lives = 999;
}

//
// P_DoSuperTransformation
//
// Transform into Super Sonic!
void P_DoSuperTransformation(player_t *player, boolean giverings)
{
	player->powers[pw_super] = true;
	if (!mapheaderinfo[gamemap-1].nossmusic && P_IsLocalPlayer(player))
	{
		S_StopMusic();
		S_ChangeMusic(mus_supers, true);
	}
	S_StartSound(player->mo, sfx_supert);

	// SONIC ONLY
	if (player->skin == 0)
	{
		player->mo->tracer = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SUPERTRANS);
		player->mo->flags2 |= MF2_DONTDRAW;
	}

	player->mfjumped = 0;
	player->secondjump = 0;
	player->mo->momx = player->mo->momy = player->mo->momz = 0;
	P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);

	if (giverings)
	{
		player->mo->health = 51;
		player->health = player->mo->health;
	}

	// Just in case.
	player->powers[pw_extralife] = 0;
	player->powers[pw_sneakers] = 0;
	player->powers[pw_invulnerability] = 0;
}
// Adds to the player's score
void P_AddPlayerScore(player_t *player, int amount)
{
	int oldscore = player->score;

	player->score += amount;

	if (player->score < 0)
		player->score = 0;

	P_CheckPointLimit(player);

	// check for extra lives every 50000 pts
	if (player->score % 50000 < amount && (gametype == GT_RACE || gametype == GT_COOP))
	{
		P_GivePlayerLives(player, (player->score/50000) - (oldscore/50000));

		if (mariomode)
			S_StartSound(player->mo, sfx_marioa);
		else
		{
			if (P_IsLocalPlayer(player))
			{
				S_StopMusic();
				S_ChangeMusic(mus_xtlife, false);
			}
			player->powers[pw_extralife] = extralifetics + 1;
		}
	}
}

//
// P_RestoreMusic
//
// Restores music after some special music change
//
void P_RestoreMusic(player_t *player)
{
	if (player->powers[pw_super] && !mapheaderinfo[gamemap-1].nossmusic)
		S_ChangeMusic(mus_supers, true);
	else if (player->powers[pw_invulnerability] > 1 && player->powers[pw_extralife] <= 1)
	{
		if (mariomode)
			S_ChangeMusic(mus_minvnc, false);
		else
			S_ChangeMusic(mus_invinc, false);
	}
	else if (player->powers[pw_sneakers] > 1 && !mapheaderinfo[gamemap-1].nossmusic)
		S_ChangeMusic(mus_shoes, false);
	else if (!(player->powers[pw_extralife] > 1))
		S_ChangeMusic(mapmusic & 2047, true);
}

//
// P_IsLocalPlayer
//
// Returns true if player is
// on the local machine.
//
boolean P_IsLocalPlayer(player_t *player)
{
	return ((cv_splitscreen.value && player == &players[secondarydisplayplayer]) || player == &players[consoleplayer]);
}

//
// P_SpawnShieldOrb
//
// Spawns the shield orb on the player
// depending on which shield they are
// supposed to have.
//
void P_SpawnShieldOrb(player_t *player)
{
#ifdef PARANOIA
	if (!player->mo)
		I_Error("P_SpawnShieldOrb: player->mo is NULL!\n");
#endif

	if (player->powers[pw_jumpshield])
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_WHITEORB)->target =
			player->mo;
	else if (player->powers[pw_ringshield])
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_YELLOWORB)->target =
			player->mo;
	else if (player->powers[pw_watershield])
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BLUEORB)->target =
			player->mo;
	else if (player->powers[pw_bombshield])
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_BLACKORB)->target =
			player->mo;
	else if (player->powers[pw_fireshield])
		P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_REDORB)->target =
			player->mo;
}
//
// P_DoPlayerExit
//
// Player exits the map via sector trigger
void P_DoPlayerExit(player_t *player)
{
	int i;

	if (player->exiting)
		return;

	if (cv_allowexitlevel.value == 0 && (gametype == GT_MATCH || gametype == GT_TAG
		|| gametype == GT_CTF 
#ifdef CHAOSISNOTDEADYET
		|| gametype == GT_CHAOS
#endif
		))
	{
		return;
	}
	else if (gametype == GT_RACE) // If in Race Mode, allow
	{

		if (!countdown) // a 60-second wait ala Sonic 2.
			countdown = cv_countdowntime.value*TICRATE + 1; // Use cv_countdowntime

		player->exiting = 3*TICRATE;

		if (!countdown2)
			countdown2 = (11 + cv_countdowntime.value)*TICRATE + 1; // 11sec more than countdowntime

		// Check if all the players in the race have finished. If so, end the level.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (!players[i].exiting)
				{
					if (players[i].lives > 0)
						break;
				}
			}
		}

		if (i == MAXPLAYERS) // finished
		{
			player->exiting = (14*TICRATE)/5 + 1;
			countdown = 0;
			countdown2 = 0;
		}
	}
	else
		player->exiting = (14*TICRATE)/5 + 2; // Accidental death safeguard???

	player->gliding = 0;
	player->climbing = 0;

	if (playeringame[player-players] && netgame && (gametype == GT_COOP || gametype == GT_RACE) && !circuitmap)
		CONS_Printf("%s has completed the level.\n", player_names[player-players]);
}

#define SPACESPECIAL 6
static boolean P_InSpaceSector(mobj_t *mo) // Returns true if you are in space
{
	sector_t *sector;

	sector = mo->subsector->sector;

	if (sector->special == SPACESPECIAL)
		return true;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (rover->master->frontsector->special != SPACESPECIAL)
				continue;

			if (mo->z > *rover->topheight)
				continue;

			if (mo->z + (mo->height/2) < *rover->bottomheight)
				continue;

			return true;
		}
	}

	return false; // No vacuum here, Captain!
}

static boolean P_InQuicksand(mobj_t *mo) // Returns true if you are in quicksand
{
	sector_t *sector;

	sector = mo->subsector->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_QUICKSAND))
				continue;

			if (mo->z > *rover->topheight)
				continue;

			if (mo->z + (mo->height/2) < *rover->bottomheight)
				continue;

			return true;
		}
	}

	return false; // No sand here, Captain!
}

//
// P_DoJump
//
// Jump routine for the player
//
void P_DoJump(player_t *player, boolean soundandstate)
{
	if (player->climbing)
	{
		// Jump this high.
		if (player->powers[pw_super])
			player->mo->momz = 5*FRACUNIT;
		else if (player->mo->eflags & MF_UNDERWATER)
			player->mo->momz = 2*FRACUNIT;
		else
			player->mo->momz = 15*(FRACUNIT/4);

		player->mo->angle = player->mo->angle - ANG180; // Turn around from the wall you were climbing.

		if (player == &players[consoleplayer])
			localangle = player->mo->angle; // Adjust the local control angle.
		else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;

		player->climbing = 0; // Stop climbing, duh!
		P_InstaThrust(player->mo, player->mo->angle, 6*FRACUNIT); // Jump off the wall.
	}
	else if (!(player->mfjumped)) // Spin Attack
	{
		if (player->mo->ceilingz-player->mo->floorz <= player->mo->height-1)
			return;

		// Jump this high.
		if (player->carried)
		{
			player->mo->momz = 9*FRACUNIT;
			player->carried = false;
		}
		else if (maptol & TOL_NIGHTS)
		{
			if (player->mo->eflags & MF_UNDERWATER)
				player->mo->momz = (914*FRACUNIT)/65;
			else
				player->mo->momz = 24*FRACUNIT;
		}
		else if (player->powers[pw_super])
		{
			if (player->mo->eflags & MF_UNDERWATER)
				player->mo->momz = (457*FRACUNIT)/60;
			else
				player->mo->momz = 13*FRACUNIT;

			if (P_InQuicksand(player->mo))
				player->mo->momz >>= 1;
		}
		else if (player->mo->eflags & MF_UNDERWATER)
			player->mo->momz = (457*FRACUNIT)/80; // jump this high
		else
		{
			player->mo->momz = 39*(FRACUNIT/4); // Ramp Test

			if (P_InQuicksand(player->mo))
				player->mo->momz = player->mo->momz>>1;
		}

		player->jumping = 1;
	}

	player->mo->momz = FixedDiv(player->jumpfactor*player->mo->momz,100*FRACUNIT); // Custom height
	player->mo->z++; // set just an eensy above the ground

	player->mo->z += player->mo->pmomz; // Solves problem of 'hitting around again after jumping on a moving platform'.

	if (!player->mfspinning)
		P_ResetScore(player);

	player->mfjumped = 1;

	if (soundandstate)
	{
		S_StartSound(player->mo, sfx_jump); // Play jump sound!

		if (player->charflags & SF_NOJUMPSPIN)
			P_SetPlayerMobjState(player->mo, S_PLAY_PLG1);
		else
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
	}
}

// Control scheme for 2d levels.
static void P_2dMovement(player_t *player)
{
	ticcmd_t *cmd;
	int topspeed, acceleration, thrustfactor;
	fixed_t movepushforward = 0;
	angle_t movepushangle = 0;
	fixed_t totalthrust_x = 0;
	fixed_t totalthrust_y = 0;
	fixed_t oldMagnitude, newMagnitude;
	
	cmd = &player->cmd;

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
	
	if (player->exiting)
		cmd->forwardmove = cmd->sidemove = 0;

	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 984)
	{
//		if (player->mo->z > player->mo->watertop || player->mo->z + player->mo->height < player->mo->waterbottom)
		if (!(player->mo->eflags & MF_UNDERWATER) && !(player->mo->eflags & MF_TOUCHWATER))
			player->cmomx = player->cmomy = 0;
	}

	else if (player->onconveyor == 985 && player->mo->z > player->mo->floorz)
		player->cmomx = player->cmomy = 0;

	else if (player->onconveyor != 984 && player->onconveyor != 985)
		player->cmomx = player->cmomy = 0;

	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on distance-of-a-line formula
	player->speed = abs(player->rmomx >> FRACBITS);

	if (player->gliding)
	{
		if (cmd->sidemove > 0 && player->mo->angle != 0 && player->mo->angle >= ANG180)
			player->mo->angle += (640/NEWTICRATERATIO)<<FRACBITS;
		else if (cmd->sidemove < 0 && player->mo->angle != ANG180 && (player->mo->angle > ANG180 || player->mo->angle == 0))
			player->mo->angle -= (640/NEWTICRATERATIO)<<FRACBITS;
		else if (cmd->sidemove == 0)
		{
			if (player->mo->angle >= ANG270)
				player->mo->angle += (640/NEWTICRATERATIO)<<FRACBITS;
			else if (player->mo->angle < ANG270 && player->mo->angle > ANG180)
				player->mo->angle -= (640/NEWTICRATERATIO)<<FRACBITS;
		}
	}
	else if (cmd->sidemove && !(player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing]))
	{
		if (player->rmomx > 0)
			player->mo->angle = 0;
		else if (player->rmomx < 0)
			player->mo->angle = ANG180;
	}

	localangle = player->mo->angle;

	if (player->gliding)
		movepushangle = player->mo->angle;
	else
	{
		if (cmd->sidemove > 0)
			movepushangle = 0;
		else if (cmd->sidemove < 0)
			movepushangle = ANG180;
		else
			movepushangle = player->mo->angle;
	}

	// Do not let the player control movement if not onground.
	onground = (player->mo->z <= player->mo->floorz) || (player->cheats & CF_FLYAROUND)
		|| (player->mo->flags2&(MF2_ONMOBJ));

	player->aiming = cmd->aiming<<FRACBITS;

	// Set the player speeds.
	if (player->powers[pw_super] || player->powers[pw_sneakers])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart/4 + player->speed*(player->acceleration/4);

		if (player->powers[pw_tailsfly])
			topspeed = player->normalspeed;
		else if (player->mo->eflags & MF_UNDERWATER)
		{
			topspeed = player->normalspeed;
			acceleration = (acceleration * 2) / 3;
		}
		else
			topspeed = player->normalspeed * 2 > 50 ? 50 : player->normalspeed * 2;
	}
	else
	{
		thrustfactor = player->thrustfactor;
		acceleration = player->accelstart + player->speed*player->acceleration;

		if (player->powers[pw_tailsfly])
		{
			topspeed = player->normalspeed/2;
		}
		else if (player->mo->eflags & MF_UNDERWATER)
		{
			topspeed = player->normalspeed/2;
			acceleration = (acceleration * 2) / 3;
		}
		else
		{
			topspeed = player->normalspeed;
		}
	}

//////////////////////////////////////
	if (player->climbing == 1)
		player->mo->momz = (cmd->forwardmove*FRACUNIT)/10;

	if (cmd->sidemove != 0 && !(player->climbing || player->gliding || player->exiting
		|| (player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing]
		&& !onground)))
	{
		if (player->powers[pw_sneakers] || player->powers[pw_super]) // do you have super sneakers?
			movepushforward = abs(cmd->sidemove) * ((thrustfactor*2)*acceleration);
		else // if not, then run normally
			movepushforward = abs(cmd->sidemove) * (thrustfactor*acceleration);

		// allow very small movement while in air for gameplay
		if (!onground)
			movepushforward >>= 2; // Proper air movement

		// Allow a bit of movement while spinning
		if (player->mfspinning)
		{
			if (!player->mfstartdash)
				movepushforward = movepushforward/48;
			else
				movepushforward = 0;
		}
		
		totalthrust_x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust_y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);

		//if (((player->rmomx >> FRACBITS) < topspeed) && (cmd->sidemove > 0)) // Sonic's Speed
			//P_Thrust(player->mo, movepushangle, movepushforward);
		//else if (((player->rmomx >> FRACBITS) > -topspeed) && (cmd->sidemove < 0))
			//P_Thrust(player->mo, movepushangle, movepushforward);
	}
	
	//Do not change positions if the player is in gliding, exiting or pain state
	if (player->gliding || player->exiting || (player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing] && !onground))
		return;
	
	player->mo->momx += totalthrust_x;
	player->mo->momy += totalthrust_y;
	
	// Time to ask three questions:
	// 1) Are we over topspeed?
	// 2) If "yes" to 1, were we moving over topspeed to begin with?
	// 3) If "yes" to 2, are we now going faster?

	// If "yes" to 3, normalize to our initial momentum; this will allow thoks to stay as fast as they normally are.
	// If "no" to 3, ignore it; the player might be going too fast, but they're slowing down, so let them.
	// If "no" to 2, normalize to topspeed, so we can't suddenly run faster than it of our own accord.
	// If "no" to 1, we're not reaching any limits yet, so ignore this entirely!
	// -Shadow Hog
	
	//boolean spin = (onground && player->mfspinning && (player->rmomx || player->rmomy) && !player->mfstartdash);
	newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
	
	if (newMagnitude > (fixed_t)(topspeed<<FRACBITS))
	{
		fixed_t tempmomx, tempmomy;
		if (oldMagnitude > (fixed_t)(topspeed<<FRACBITS))
		{
			if (newMagnitude > oldMagnitude)
			{
				tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), oldMagnitude);
				tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), oldMagnitude);
				player->mo->momx = tempmomx + player->cmomx;
				player->mo->momy = tempmomy + player->cmomy;
			}
			// else do nothing
		}
		else
		{
			tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), (fixed_t)(topspeed<<FRACBITS));
			tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), (fixed_t)(topspeed<<FRACBITS));
			player->mo->momx = tempmomx + player->cmomx;
			player->mo->momy = tempmomy + player->cmomy;
		}
	}
}

static void P_HandleConveyorBeltSpeed(player_t* player)
{
	// cmomx/cmomy stands for the conveyor belt speed.
	if (player->onconveyor == 984)
	{
		if (!(player->mo->eflags & MF_UNDERWATER) && !(player->mo->eflags & MF_TOUCHWATER))
			player->cmomx = player->cmomy = 0;
	}
	else if ((player->onconveyor == 985 && player->mo->z > player->mo->floorz) || (player->onconveyor != 984 && player->onconveyor != 985))
	{
		player->cmomx = player->cmomy = 0;
	}

}


static void P_3dMovement(player_t* player)
{
	ticcmd_t *cmd;
	angle_t	movepushangle;
	angle_t movepushsideangle;
	int topspeed, acceleration, thrustfactor;
	fixed_t movepushforward, movepushside;
	fixed_t totalthrust_x, totalthrust_y;

	cmd = &player->cmd;
	movepushforward = 0;
	movepushside = 0;
	movepushangle = player->mo->angle;
	movepushsideangle = movepushangle-ANG90;
	fixed_t oldMagnitude, newMagnitude;
	totalthrust_x = totalthrust_y = 0;

	// Get the old momentum; this will be needed at the end of the function! -SH
	oldMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);

	if (player->exiting)
		cmd->forwardmove = cmd->sidemove = 0;
		
	P_HandleConveyorBeltSpeed(player);
	player->rmomx = player->mo->momx - player->cmomx;
	player->rmomy = player->mo->momy - player->cmomy;

	// Calculates player's speed based on distance-of-a-line formula
	player->speed = P_AproxDistance(player->rmomx, player->rmomy) >> FRACBITS;

	// Do specific changes if the player is not onground.
	onground = (player->mo->z <= player->mo->floorz) 
		|| (player->cheats & CF_FLYAROUND)
		|| (player->mo->flags2&(MF2_ONMOBJ));

	player->aiming = cmd->aiming<<FRACBITS;

	// Set the player speeds.
	if (player->powers[pw_super] || player->powers[pw_sneakers])
	{
		thrustfactor = player->thrustfactor*2;
		acceleration = player->accelstart/4 + player->speed*(player->acceleration/4);

		if (player->powers[pw_tailsfly])
		{
			topspeed = player->normalspeed;
		}
		else if (player->mo->eflags & MF_UNDERWATER)
		{
			topspeed = player->normalspeed;
			acceleration = (acceleration * 2) / 3;
		}
		else
		{
			topspeed = player->normalspeed * 2 > 50 ? 50 : player->normalspeed * 2;
		}
	}
	else
	{
		thrustfactor = player->thrustfactor;
		acceleration = player->accelstart + player->speed*player->acceleration;

		if (player->powers[pw_tailsfly])
		{
			topspeed = player->normalspeed/2;
		}
		else if (player->mo->eflags & MF_UNDERWATER)
		{
			topspeed = player->normalspeed/2;
			acceleration = (acceleration * 2) / 3;
		}
		else
		{
			topspeed = player->normalspeed;
		}
	}
	
	//Forward move
	if (netgame || ((player == &players[consoleplayer] && !cv_analog.value)
		&& cmd->forwardmove
		&& !(player->gliding || player->exiting || (player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing] && !onground))))
	{
		if (player->climbing)
			player->mo->momz = (cmd->forwardmove*FRACUNIT)/8;
		else if (player->powers[pw_sneakers] || player->powers[pw_super]) // super sneakers?
			movepushforward = cmd->forwardmove * ((thrustfactor*2)*acceleration);
		else // if not, then run normally
			movepushforward = cmd->forwardmove * (thrustfactor*acceleration);

		// allow very small movement while in air for gameplay
		if (!onground)
		{
			if (player->powers[pw_tailsfly]) // control better while flying
				movepushforward >>= 1;
			else
				movepushforward >>= 2; // proper air movement
		}

		// Allow a bit of movement while spinning
		if (player->mfspinning)
		{
			if (cmd->forwardmove)
				movepushforward = 0;
			else if (!player->mfstartdash)
				movepushforward = FixedDiv(movepushforward,16*FRACUNIT);
			else
				movepushforward = 0;
		}
		
		totalthrust_x += P_ReturnThrustX(player->mo, movepushangle, movepushforward);
		totalthrust_y += P_ReturnThrustY(player->mo, movepushangle, movepushforward);
	}
	//Sidemove
	if (netgame || (player == &players[consoleplayer] && !cv_analog.value))
	{
		if (player->climbing)
		{
			P_InstaThrust(player->mo, player->mo->angle-ANG90, (cmd->sidemove/10)*FRACUNIT);
		}
		else if (cmd->sidemove && !player->gliding 
				&& !player->exiting && !player->climbing 
				&& !(player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing]))
		{
			movepushside = cmd->sidemove * (thrustfactor*acceleration);

			if (player->powers[pw_sneakers] || player->powers[pw_super])
				movepushside *= 2;

			if (!onground)
				movepushside >>= 2;

			// Allow a bit of movement while spinning
			if (player->mfspinning)
			{
				if (!player->mfstartdash)
					movepushside = FixedDiv(movepushside,16*FRACUNIT);
				else
					movepushside = 0;
			}

			// Finally move the player now that his speed/direction has been decided.
			totalthrust_x += P_ReturnThrustX(player->mo, movepushsideangle, movepushside);
			totalthrust_y += P_ReturnThrustY(player->mo, movepushsideangle, movepushside);
		}
	}
	
	//Do not change positions if the player is in gliding, exiting or pain state
	if (player->gliding || player->exiting || (player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing] && !onground))
		return;
	
	player->mo->momx += totalthrust_x;
	player->mo->momy += totalthrust_y;
	
	// Time to ask three questions:
	// 1) Are we over topspeed?
	// 2) If "yes" to 1, were we moving over topspeed to begin with?
	// 3) If "yes" to 2, are we now going faster?

	// If "yes" to 3, normalize to our initial momentum; this will allow thoks to stay as fast as they normally are.
	// If "no" to 3, ignore it; the player might be going too fast, but they're slowing down, so let them.
	// If "no" to 2, normalize to topspeed, so we can't suddenly run faster than it of our own accord.
	// If "no" to 1, we're not reaching any limits yet, so ignore this entirely!
	// -Shadow Hog
	
	//boolean spin = (onground && player->mfspinning && (player->rmomx || player->rmomy) && !player->mfstartdash);
	newMagnitude = R_PointToDist2(player->mo->momx - player->cmomx, player->mo->momy - player->cmomy, 0, 0);
	
	if (newMagnitude > (fixed_t)(topspeed<<FRACBITS))
	{
		fixed_t tempmomx, tempmomy;
		if (oldMagnitude > (fixed_t)(topspeed<<FRACBITS))
		{
			if (newMagnitude > oldMagnitude)
			{
				tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), oldMagnitude);
				tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), oldMagnitude);
				player->mo->momx = tempmomx + player->cmomx;
				player->mo->momy = tempmomy + player->cmomy;
			}
			// else do nothing
		}
		else
		{
			tempmomx = FixedMul(FixedDiv(player->mo->momx - player->cmomx, newMagnitude), (fixed_t)(topspeed<<FRACBITS));
			tempmomy = FixedMul(FixedDiv(player->mo->momy - player->cmomy, newMagnitude), (fixed_t)(topspeed<<FRACBITS));
			player->mo->momx = tempmomx + player->cmomx;
			player->mo->momy = tempmomy + player->cmomy;
		}
	}
}

//
// P_ShootLine
//
// Fun and fancy
// graphical indicator
// for building/debugging
// NiGHTS levels!
static void P_ShootLine(mobj_t *source, mobj_t *dest, fixed_t height)
{
	mobj_t *mo;
	int i;
	fixed_t temp;
	int speed, seesound;

	temp = dest->z;
	dest->z = height;

	seesound = mobjinfo[MT_REDRING].seesound;
	speed = mobjinfo[MT_REDRING].speed;
	mobjinfo[MT_REDRING].seesound = sfx_None;
	mobjinfo[MT_REDRING].speed = 20*FRACUNIT;

	mo = P_SpawnXYZMissile(source, dest, MT_REDRING, source->x, source->y, height);

	dest->z = temp;
	if (mo)
	{
		mo->flags2 |= MF2_RAILRING;
		mo->flags2 |= MF2_DONTDRAW;
		mo->flags2 |= MF2_NOCLIPHEIGHT;
		mo->flags |= MF_NOCLIP;
		mo->flags &= ~MF_MISSILE;
		mo->fuse = 3;
	}

	for (i = 0; i < 32; i++)
	{
		if (mo)
		{
			if (!(mo->flags & MF_NOBLOCKMAP))
			{
				P_UnsetThingPosition(mo);
				mo->flags |= MF_NOBLOCKMAP;
				P_SetThingPosition(mo);
			}
			if (i&1)
				P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

			P_UnsetThingPosition(mo);
			mo->x += mo->momx;
			mo->y += mo->momy;
			mo->z += mo->momz;
			P_SetThingPosition(mo);
		}
		else
		{
			mobjinfo[MT_REDRING].seesound = seesound;
			mobjinfo[MT_REDRING].speed = speed;
			return;
		}
	}
	mobjinfo[MT_REDRING].seesound = seesound;
	mobjinfo[MT_REDRING].speed = speed;
}

#define MAXDRILLSPEED 14000
#define MAXNORMALSPEED 6000
//
// P_NiGHTSMovement
//
// Movement code for NiGHTS!
//
static void P_NiGHTSMovement(player_t *player)
{
	int drillamt = 0;
	boolean still = false, moved = false, backwardaxis = false, firstdrill;
	signed short newangle = 0;
	fixed_t xspeed, yspeed;
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *closestaxis = NULL;
	fixed_t newx, newy, radius;
	angle_t movingangle;
	ticcmd_t *cmd = &player->cmd;
	int thrustfactor;
	int i;

	player->drilling = false;

	firstdrill = false;

	if (player->drillmeter > 96*20)
		player->drillmeter = 96*20;

	if (player->drilldelay)
		player->drilldelay--;

	if (!(cmd->buttons & BT_JUMP))
	{
		// Always have just a TINY bit of drill power.
		if (player->drillmeter <= 0)
			player->drillmeter = (TICRATE/10)/NEWTICRATERATIO;
	}

	if (!player->mo->tracer)
	{
		P_DeNightserizePlayer(player);
		return;
	}

	if (leveltime % TICRATE == 0 && gametype != GT_RACE)
		player->nightstime--;

	if (!player->nightstime)
	{
		P_DeNightserizePlayer(player);
		S_StartScreamSound(player->mo, sfx_lose);
		return;
	}

	if (player->mo->z < player->mo->floorz)
		player->mo->z = player->mo->floorz;

	if (player->mo->z+player->mo->height > player->mo->ceilingz)
		player->mo->z = player->mo->ceilingz - player->mo->height;

	newx = P_ReturnThrustX(player->mo, player->mo->angle, 3*FRACUNIT)+player->mo->x;
	newy = P_ReturnThrustY(player->mo, player->mo->angle, 3*FRACUNIT)+player->mo->y;

	if (!player->mo->target)
	{
		fixed_t dist1, dist2 = 0;

		// scan the thinkers
		// to find the closest axis point
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type == MT_AXIS)
			{
				if (mo2->threshold == player->mare)
				{
					if (closestaxis == NULL)
					{
						closestaxis = mo2;
						dist2 = R_PointToDist2(newx, newy, mo2->x, mo2->y)-mo2->radius;
					}
					else
					{
						dist1 = R_PointToDist2(newx, newy, mo2->x, mo2->y)-mo2->radius;

						if (dist1 < dist2)
						{
							closestaxis = mo2;
							dist2 = dist1;
						}
					}
				}
			}
		}

		player->mo->target = closestaxis;
	}

	if (!player->mo->target) // Uh-oh!
	{
		CONS_Printf("No axis points found!\n");
		return;
	}

	// The 'ambush' flag says you should rotate
	// the other way around the axis.
	if (player->mo->target->flags & MF_AMBUSH)
		backwardaxis = true;

	player->angle_pos = R_PointToAngle2(player->mo->target->x, player->mo->target->y, player->mo->x, player->mo->y);

	player->old_angle_pos = player->angle_pos;

	radius = player->mo->target->radius;

	player->mo->flags |= MF_NOGRAVITY;
	player->mo->flags2 |= MF2_DONTDRAW;

	// Currently reeling from being hit.
	if (player->powers[pw_flashing] > (2*flashingtics)/3)
	{
		{
			const angle_t fa = FINEANGLE_C(player->flyangle);
			const fixed_t speed = (player->speed*FRACUNIT)/50;

			xspeed = FixedMul(finecosine[fa],speed);
			yspeed = FixedMul(finesine[fa],speed);
		}

		if (!player->transfertoclosest)
		{
			xspeed = FixedMul(xspeed, FixedDiv(10240*FRACUNIT, player->mo->target->radius)/10);

			if (backwardaxis)
				xspeed *= -1;

			player->angle_pos += FixedAngleC(FixedDiv(xspeed,5*FRACUNIT),40*FRACUNIT);
		}

		if (player->transfertoclosest)
		{
			const angle_t fa = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
			P_InstaThrust(player->mo, fa, xspeed/10);
		}
		else
		{
			const angle_t fa = player->angle_pos>>ANGLETOFINESHIFT;

			player->mo->momx = player->mo->target->x + FixedMul(finecosine[fa],radius) - player->mo->x;
			player->mo->momy = player->mo->target->y + FixedMul(finesine[fa],radius) - player->mo->y;
		}

		player->mo->momz = 0;
		P_UnsetThingPosition(player->mo->tracer);
		player->mo->tracer->x = player->mo->x;
		player->mo->tracer->y = player->mo->y;
		player->mo->tracer->z = player->mo->z;
		player->mo->tracer->floorz = player->mo->floorz;
		player->mo->tracer->ceilingz = player->mo->ceilingz;
		P_SetThingPosition(player->mo->tracer);
		return;
	}

	if (player->mo->tracer->state >= &states[S_SUPERTRANS1]
		&& player->mo->tracer->state <= &states[S_SUPERTRANS9])
	{
		player->mo->momx = player->mo->momy = player->mo->momz = 0;

		P_UnsetThingPosition(player->mo->tracer);
		player->mo->tracer->x = player->mo->x;
		player->mo->tracer->y = player->mo->y;
		player->mo->tracer->z = player->mo->z;
		player->mo->tracer->floorz = player->mo->floorz;
		player->mo->tracer->ceilingz = player->mo->ceilingz;
		P_SetThingPosition(player->mo->tracer);
		return;
	}

	if (player->exiting > 0 && player->exiting < 2*TICRATE)
	{
		player->mo->momx = player->mo->momy = 0;

		if (gametype != GT_RACE)
			player->mo->momz = 30*FRACUNIT;

		player->mo->tracer->angle += ANG45/4;

		if (!(player->mo->tracer->state  >= &states[S_NIGHTSDRONE1]
			&& player->mo->tracer->state <= &states[S_NIGHTSDRONE2]))
			P_SetMobjState(player->mo->tracer, S_NIGHTSDRONE1);

		player->mo->tracer->flags2 |= MF2_NOCLIPHEIGHT;
		player->mo->flags2 |= MF2_NOCLIPHEIGHT;

		P_UnsetThingPosition(player->mo->tracer);
		player->mo->tracer->x = player->mo->x;
		player->mo->tracer->y = player->mo->y;
		player->mo->tracer->z = player->mo->z;
		player->mo->tracer->floorz = player->mo->floorz;
		player->mo->tracer->ceilingz = player->mo->ceilingz;
		P_SetThingPosition(player->mo->tracer);
		return;
	}

	// Spawn the little sparkles on each side of the player.
	if (leveltime & 1)
	{
		mobj_t *firstmobj;
		mobj_t *secondmobj;

		firstmobj = P_SpawnMobj(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle+ANG90, 16*FRACUNIT), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle+ANG90, 16*FRACUNIT), player->mo->z + player->mo->height/2, MT_NIGHTSPARKLE);
		secondmobj = P_SpawnMobj(player->mo->x + P_ReturnThrustX(player->mo, player->mo->angle-ANG90, 16*FRACUNIT), player->mo->y + P_ReturnThrustY(player->mo, player->mo->angle-ANG90, 16*FRACUNIT), player->mo->z + player->mo->height/2, MT_NIGHTSPARKLE);

		if (firstmobj)
		{
			firstmobj->fuse = leveltime;
			firstmobj->target = player->mo;
		}

		if (secondmobj)
		{
			secondmobj->fuse = leveltime;
			secondmobj->target = player->mo;
		}

		player->mo->fuse = leveltime;
	}

	if (player->bumpertime)
	{
		player->jumping = 1;
		player->drilling = true;
	}
	else if (cmd->buttons & BT_JUMP && player->drillmeter && player->drilldelay == 0)
	{
		if (!player->jumping)
			firstdrill = true;

		player->jumping = 1;
		player->drilling = true;
	}
	else
	{
		player->jumping = 0;

		if (cmd->sidemove != 0)
			moved = true;

		if (player->drillmeter & 1)
			player->drillmeter++; // I'll be nice and give them one.
	}

	if (cmd->forwardmove != 0)
		moved = true;

	if (player->bumpertime)
		drillamt = 0;
	else if (moved)
	{
		if (player->drilling)
		{
			drillamt += 50;
		}
		else
		{
			const int distabs = abs(cmd->forwardmove)*abs(cmd->forwardmove) + abs(cmd->sidemove)*abs(cmd->sidemove);
			const float distsqrt = (float)(sqrt(distabs));
			const int distance = (int)distsqrt;

			drillamt += distance > 50 ? 50 : distance;

			drillamt = (5*drillamt)/4;
		}
	}

	player->speed += drillamt;

	if (!player->bumpertime)
	{
		if (!player->drilling)
		{
			if (player->speed > MAXDRILLSPEED)
				player->speed -= 100+drillamt;
			else if (player->speed > MAXNORMALSPEED)
				player->speed -= (drillamt*19)/16;
		}
		else
		{
			player->speed += 75;
			if (player->speed > MAXDRILLSPEED)
				player->speed -= 100+drillamt;

			if (--player->drillmeter == 0)
				player->drilldelay = TICRATE*2;
		}
	}

	if (!player->bumpertime)
	{
		if (drillamt == 0 && player->speed > 0)
			player->speed -= 25;

		if (player->speed < 0)
			player->speed = 0;

		if (cmd->sidemove != 0)
		{
			newangle = (signed short)(AngleFixed(R_PointToAngle2(0,0, cmd->sidemove*FRACUNIT, cmd->forwardmove*FRACUNIT))/FRACUNIT);
		}
		else if (cmd->forwardmove > 0)
			newangle = 90;
		else if (cmd->forwardmove < 0)
			newangle = 269;

		if (newangle < 0 && moved)
			newangle = (signed short)(360+newangle);
	}

	if (player->drilling)
		thrustfactor = 1;
	else
		thrustfactor = 6;

	for (i = 0; i < thrustfactor; i++)
	{
		if (moved && player->flyangle != newangle)
		{
			// player->flyangle is the one to move
			// newangle is the "move to"
			if ((((newangle-player->flyangle)+360)%360)>(((player->flyangle-newangle)+360)%360))
			{
				player->flyangle--;
				if (player->flyangle < 0)
					player->flyangle = 360 + player->flyangle;
			}
			else
				player->flyangle++;
		}

		player->flyangle %= 360;
	}

	if (!(player->speed)
		&& cmd->forwardmove == 0)
		still = true;

	if ((cmd->buttons & BT_CAMLEFT) && (cmd->buttons & BT_CAMRIGHT))
	{
		if (!player->skiddown && player->speed > 2000)
		{
			player->speed /= 10;
			S_StartSound(player->mo, sfx_ngskid);
		}
		player->skiddown = true;
	}
	else
		player->skiddown = false;

	{
		const angle_t fa = FINEANGLE_C(player->flyangle);
		const fixed_t speed = (player->speed*FRACUNIT)/50;
		xspeed = FixedMul(finecosine[fa],speed);
		yspeed = FixedMul(finesine[fa],speed);
	}

	if (!player->transfertoclosest)
	{
		xspeed = FixedMul(xspeed, FixedDiv(10240*FRACUNIT, player->mo->target->radius)/10);

		if (backwardaxis)
			xspeed *= -1;

		player->angle_pos += FixedAngleC(FixedDiv(xspeed,5*FRACUNIT),40*FRACUNIT);
	}

	{
		if (player->transfertoclosest)
		{
			const angle_t fa = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
			P_InstaThrust(player->mo, fa, xspeed/10);
		}
		else
		{
			const angle_t fa = player->angle_pos>>ANGLETOFINESHIFT;

			player->mo->momx = player->mo->target->x + FixedMul(finecosine[fa],radius) - player->mo->x;
			player->mo->momy = player->mo->target->y + FixedMul(finesine[fa],radius) - player->mo->y;
		}

		{
			const int sequence = player->mo->target->threshold;
			mobj_t *transfer1 = NULL;
			mobj_t *transfer2 = NULL;
			mobj_t *axis;
			line_t transfer1line;
			line_t transfer2line;
			boolean transfer1last = false;
			boolean transfer2last = false;
			vertex_t vertices[4];

			// Find next waypoint
			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
					continue;

				mo2 = (mobj_t *)th;

				// Axis things are only at beginning of list.
				if (!(mo2->flags2 & MF2_AXIS))
					break;

				if ((mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE)
					&& mo2->threshold == sequence)
				{
					if (player->transfertoclosest)
					{
						if (mo2->health == player->axis1->health)
							transfer1 = mo2;
						else if (mo2->health == player->axis2->health)
							transfer2 = mo2;
					}
					else
					{
						if (mo2->health == player->mo->target->health)
							transfer1 = mo2;
						else if (mo2->health == player->mo->target->health + 1)
							transfer2 = mo2;
					}
				}
			}

			// It might be possible that one wasn't found.
			// Is it because we're at the end of the track?
			// Look for a wrapper point.
			if (!transfer1)
			{
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
						continue;

					mo2 = (mobj_t *)th;

					// Axis things are only at beginning of list.
					if (!(mo2->flags2 & MF2_AXIS))
						break;

					if (mo2->threshold == sequence && (mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
					{
						if (!transfer1)
						{
							transfer1 = mo2;
							transfer1last = true;
						}
						else if (mo2->health > transfer1->health)
						{
							transfer1 = mo2;
							transfer1last = true;
						}
					}
				}
			}
			if (!transfer2)
			{
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
						continue;

					mo2 = (mobj_t *)th;

					// Axis things are only at beginning of list.
					if (!(mo2->flags2 & MF2_AXIS))
						break;

					if (mo2->threshold == sequence && (mo2->type == MT_AXISTRANSFER || mo2->type == MT_AXISTRANSFERLINE))
					{
						if (!transfer2)
						{
							transfer2 = mo2;
							transfer2last = true;
						}
						else if (mo2->health > transfer2->health)
						{
							transfer2 = mo2;
							transfer2last = true;
						}
					}
				}
			}

			if (!(transfer1 && transfer2)) // We can't continue...
				I_Error("Mare does not form a complete circuit!\n");

			transfer1line.v1 = &vertices[0];
			transfer1line.v2 = &vertices[1];
			transfer2line.v1 = &vertices[2];
			transfer2line.v2 = &vertices[3];

			if (cv_debug && (leveltime % TICRATE == 0))
			{
				CONS_Printf("Transfer1 : %d\n", transfer1->health);
				CONS_Printf("Transfer2 : %d\n", transfer2->health);
			}

//			CONS_Printf("T1 is at %d, %d\n", transfer1->x>>FRACBITS, transfer1->y>>FRACBITS);
//			CONS_Printf("T2 is at %d, %d\n", transfer2->x>>FRACBITS, transfer2->y>>FRACBITS);
//			CONS_Printf("Distance from T1: %d\n", P_AproxDistance(transfer1->x - player->mo->x, transfer1->y - player->mo->y)>>FRACBITS);
//			CONS_Printf("Distance from T2: %d\n", P_AproxDistance(transfer2->x - player->mo->x, transfer2->y - player->mo->y)>>FRACBITS);

			// Transfer1 is closer to the player than transfer2
			if (P_AproxDistance(transfer1->x - player->mo->x, transfer1->y - player->mo->y)>>FRACBITS
				< P_AproxDistance(transfer2->x - player->mo->x, transfer2->y - player->mo->y)>>FRACBITS)
			{
				if (transfer1->type == MT_AXISTRANSFERLINE)
				{
					if (transfer1last)
						axis = P_FindAxis(transfer1->threshold, transfer1->health-2);
					else if (player->transfertoclosest)
						axis = P_FindAxis(transfer1->threshold, transfer1->health-1);
					else
						axis = P_FindAxis(transfer1->threshold, transfer1->health);

					if (!axis)
					{
						CONS_Printf("Unable to find an axis - error code #1\n");
						goto nightsbombout;
					}

//					CONS_Printf("Drawing a line from %d to ", axis->health);

					transfer1line.v1->x = axis->x;
					transfer1line.v1->y = axis->y;

					transfer1line.v2->x = transfer1->x;
					transfer1line.v2->y = transfer1->y;

					if (cv_debug)
						P_ShootLine(axis, transfer1, player->mo->z);

//					CONS_Printf("closest %d\n", transfer1->health);

					transfer1line.dx = transfer1line.v2->x - transfer1line.v1->x;
					transfer1line.dy = transfer1line.v2->y - transfer1line.v1->y;

					if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer1line)
							!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer1line))
					{
						if (cv_debug)
						{
							COM_BufAddText("cechoduration 1\n");
							COM_BufAddText("cecho transfer!\n");
							COM_BufAddText("cechoduration 5\n");
							S_StartSound(0, sfx_strpst);
						}
						if (player->transfertoclosest)
						{
							player->transfertoclosest = false;
							P_TransferToAxis(player, transfer1->health - 1);
						}
						else
						{
							player->transfertoclosest = true;
							player->axis2 = transfer1;
							player->axis1 = P_FindAxisTransfer(transfer1->threshold, transfer1->health-1, MT_AXISTRANSFERLINE);//P_FindAxis(transfer1->threshold, axis->health-2);
						}
					}
				}
				else
				{
					// Transfer1
					if (transfer1last)
						axis = P_FindAxis(transfer1->threshold, 1);
					else
						axis = P_FindAxis(transfer1->threshold, transfer1->health);

					if (!axis)
					{
						CONS_Printf("Unable to find an axis - error code #2\n");
						goto nightsbombout;
					}

//					CONS_Printf("Drawing a line from %d to ", axis->health);

					transfer1line.v1->x = axis->x;
					transfer1line.v1->y = axis->y;

					if (cv_debug)
						P_ShootLine(transfer1, P_FindAxis(transfer1->threshold, transfer1->health-1), player->mo->z);

//					axis = P_FindAxis(transfer1->threshold, transfer1->health-1);

//					CONS_Printf("%d\n", axis->health);

					transfer1line.v2->x = transfer1->x;
					transfer1line.v2->y = transfer1->y;

					transfer1line.dx = transfer1line.v2->x - transfer1line.v1->x;
					transfer1line.dy = transfer1line.v2->y - transfer1line.v1->y;

					if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer1line)
						!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer1line))
					{
						if (cv_debug)
						{
							COM_BufAddText("cechoduration 1\n");
							COM_BufAddText("cecho transfer!\n");
							COM_BufAddText("cechoduration 5\n");
							S_StartSound(0, sfx_strpst);
						}
						if (player->mo->target->health < transfer1->health)
						{
							// Find the next axis with a ->health
							// +1 from the current axis.
							if (transfer1last)
								P_TransferToAxis(player, transfer1->health - 1);
							else
								P_TransferToAxis(player, transfer1->health);
						}
						else if (player->mo->target->health >= transfer1->health)
						{
							// Find the next axis with a ->health
							// -1 from the current axis.
							P_TransferToAxis(player, transfer1->health - 1);
						}
					}
				}
			}
			else
			{
				if (transfer2->type == MT_AXISTRANSFERLINE)
				{
					if (transfer2last)
						axis = P_FindAxis(transfer2->threshold, 1);
					else if (player->transfertoclosest)
						axis = P_FindAxis(transfer2->threshold, transfer2->health);
					else
						axis = P_FindAxis(transfer2->threshold, transfer2->health - 1);

					if (!axis)
						axis = P_FindAxis(transfer2->threshold, 1);

					if (!axis)
					{
						CONS_Printf("Unable to find an axis - error code #3\n");
						goto nightsbombout;
					}

//					CONS_Printf("Drawing a line from %d to ", axis->health);

					transfer2line.v1->x = axis->x;
					transfer2line.v1->y = axis->y;

					transfer2line.v2->x = transfer2->x;
					transfer2line.v2->y = transfer2->y;

//					CONS_Printf("closest %d\n", transfer2->health);

					if (cv_debug)
						P_ShootLine(axis, transfer2, player->mo->z);

					transfer2line.dx = transfer2line.v2->x - transfer2line.v1->x;
					transfer2line.dy = transfer2line.v2->y - transfer2line.v1->y;

					if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer2line)
							!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer2line))
					{
						if (cv_debug)
						{
							COM_BufAddText("cechoduration 1\n");
							COM_BufAddText("cecho transfer!\n");
							COM_BufAddText("cechoduration 5\n");
							S_StartSound(0, sfx_strpst);
						}
						if (player->transfertoclosest)
						{
							player->transfertoclosest = false;

							if (!P_FindAxis(transfer2->threshold, transfer2->health))
								transfer2last = true;

							if (transfer2last)
								P_TransferToAxis(player, 1);
							else
								P_TransferToAxis(player, transfer2->health);
						}
						else
						{
							player->transfertoclosest = true;
							player->axis1 = transfer2;
							player->axis2 = P_FindAxisTransfer(transfer2->threshold, transfer2->health+1, MT_AXISTRANSFERLINE);//P_FindAxis(transfer2->threshold, axis->health + 2);
						}
					}
				}
				else
				{
					// Transfer2
					if (transfer2last)
						axis = P_FindAxis(transfer2->threshold, 1);
					else
						axis = P_FindAxis(transfer2->threshold, transfer2->health);

					if (!axis)
						axis = P_FindAxis(transfer2->threshold, 1);

					if (!axis)
					{
						CONS_Printf("Unable to find an axis - error code #4\n");
						goto nightsbombout;
					}

//					CONS_Printf("Drawing a line from %d to ", axis->health);

					transfer2line.v1->x = axis->x;
					transfer2line.v1->y = axis->y;

					if (cv_debug)
						P_ShootLine(transfer2, P_FindAxis(transfer2->threshold, transfer2->health-1), player->mo->z);

//					axis = P_FindAxis(transfer2->threshold, transfer2->health-1);

//					CONS_Printf("%d\n", axis->health);

					transfer2line.v2->x = transfer2->x;
					transfer2line.v2->y = transfer2->y;

					transfer2line.dx = transfer2line.v2->x - transfer2line.v1->x;
					transfer2line.dy = transfer2line.v2->y - transfer2line.v1->y;

					if (P_PointOnLineSide(player->mo->x, player->mo->y, &transfer2line)
						!= P_PointOnLineSide(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, &transfer2line))
					{
						if (cv_debug)
						{
							COM_BufAddText("cechoduration 1\n");
							COM_BufAddText("cecho transfer!\n");
							COM_BufAddText("cechoduration 5\n");
							S_StartSound(0, sfx_strpst);
						}
						if (player->mo->target->health < transfer2->health)
						{
							if (!P_FindAxis(transfer2->threshold, transfer2->health))
								transfer2last = true;

							if (transfer2last)
								P_TransferToAxis(player, 1);
							else
								P_TransferToAxis(player, transfer2->health);
						}
						else if (player->mo->target->health >= transfer2->health)
							P_TransferToAxis(player, transfer2->health - 1);
					}
				}
			}
		}
	}

nightsbombout:

	if (still)
		player->mo->momz = -FRACUNIT;
	else
		player->mo->momz = yspeed/11;

	if (player->mo->momz > 20*FRACUNIT)
		player->mo->momz = 20*FRACUNIT;
	else if (player->mo->momz < -20*FRACUNIT)
		player->mo->momz = -20*FRACUNIT;

	// You can create splashes as you fly across water.
	if (player->mo->z + player->mo->info->height >= player->mo->watertop && player->mo->z <= player->mo->watertop && player->speed > 9000 && leveltime % (TICRATE/7) == 0)
		S_StartSound(P_SpawnMobj(player->mo->x, player->mo->y, player->mo->watertop, MT_SPLISH), sfx_wslap);

	// Spawn Sonic's bubbles
	if (player->mo->eflags & MF_UNDERWATER)
	{
		const fixed_t zh = player->mo->z + FixedDiv(player->mo->height,5*(FRACUNIT/4));
		if (!(P_Random() % 16))
			P_SpawnMobj(player->mo->x, player->mo->y, zh, MT_SMALLBUBBLE)->threshold = 42;
		else if (!(P_Random() % 96))
			P_SpawnMobj(player->mo->x, player->mo->y, zh, MT_MEDIUMBUBBLE)->threshold = 42;
	}

	if (player->mo->momx || player->mo->momy)
		player->mo->angle = R_PointToAngle2(0, 0, player->mo->momx, player->mo->momy);

	if (still)
	{
		player->anotherflyangle = 0;
		movingangle = 0;
	}
	else if (backwardaxis)
	{
		// Special cases to prevent the angle from being
		// calculated incorrectly when wrapped.
		if (player->old_angle_pos > ANGLE_350 && player->angle_pos < ANGLE_10)
		{
			movingangle = R_PointToAngle2(0, player->mo->z, -R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
		else if (player->old_angle_pos < ANGLE_10 && player->angle_pos > ANGLE_350)
		{
			movingangle = R_PointToAngle2(0, player->mo->z, R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
		else if (player->angle_pos > player->old_angle_pos)
		{
			movingangle = R_PointToAngle2(0, player->mo->z, -R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
		else
		{
			movingangle = R_PointToAngle2(0, player->mo->z, R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
	}
	else
	{
		// Special cases to prevent the angle from being
		// calculated incorrectly when wrapped.
		if (player->old_angle_pos > ANGLE_350 && player->angle_pos < ANGLE_10)
		{
			movingangle = R_PointToAngle2(0, player->mo->z, R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
		else if (player->old_angle_pos < ANGLE_10 && player->angle_pos > ANGLE_350)
		{
			movingangle = R_PointToAngle2(0, player->mo->z, -R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
		else if (player->angle_pos < player->old_angle_pos)
		{
			movingangle = R_PointToAngle2(0, player->mo->z, -R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
		else
		{
			movingangle = R_PointToAngle2(0, player->mo->z, R_PointToDist2(player->mo->momx, player->mo->momy, 0, 0), player->mo->z + player->mo->momz);
			player->anotherflyangle = (movingangle >> ANGLETOFINESHIFT) * 360/FINEANGLES;
		}
	}

	if (player->anotherflyangle >= 349 || player->anotherflyangle <= 11)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL1A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL1D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL1A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL1B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL1A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY1A : S_NIGHTSFLY1B);
	}
	else if (player->anotherflyangle >= 12 && player->anotherflyangle <= 33)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL2A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL2D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL2A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL2B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL2A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY2A : S_NIGHTSFLY2B);
	}
	else if (player->anotherflyangle >= 34 && player->anotherflyangle <= 56)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL3A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL3D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL3A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL3B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL3A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY3A : S_NIGHTSFLY3B);
	}
	else if (player->anotherflyangle >= 57 && player->anotherflyangle <= 79)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL4A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL4D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL4A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL4B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL4A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY4A : S_NIGHTSFLY4B);
	}
	else if (player->anotherflyangle >= 80 && player->anotherflyangle <= 101)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL5A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL5D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL5A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL5B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL5A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY5A : S_NIGHTSFLY5B);
	}
	else if (player->anotherflyangle >= 102 && player->anotherflyangle <= 123)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL4A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL4D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL4A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL4B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL4A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY4A : S_NIGHTSFLY4B);
	}
	else if (player->anotherflyangle >= 124 && player->anotherflyangle <= 146)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL3A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL3D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL3A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL3B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL3A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY3A : S_NIGHTSFLY3B);
	}
	else if (player->anotherflyangle >= 147 && player->anotherflyangle <= 168)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL2A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL2D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL2A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL2B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL2A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY2A : S_NIGHTSFLY2B);
	}
	else if (player->anotherflyangle >= 169 && player->anotherflyangle <= 191)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL1A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL1D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL1A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL1B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL1A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY1A : S_NIGHTSFLY1B);
	}
	else if (player->anotherflyangle >= 192 && player->anotherflyangle <= 213)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL6A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL6D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL6A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL6B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL6A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY6A : S_NIGHTSFLY6B);
	}
	else if (player->anotherflyangle >= 214 && player->anotherflyangle <= 236)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL7A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL7D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL7A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL7B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL7A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY7A : S_NIGHTSFLY7B);
	}
	else if (player->anotherflyangle >= 237 && player->anotherflyangle <= 258)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL8A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL8D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL8A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL8B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL8A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY8A : S_NIGHTSFLY8B);
	}
	else if (player->anotherflyangle >= 259 && player->anotherflyangle <= 281)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL9A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL9D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL9A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL9B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL9A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY9A : S_NIGHTSFLY9B);
	}
	else if (player->anotherflyangle >= 282 && player->anotherflyangle <= 304)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL8A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL8D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL8A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL8B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL8A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY8A : S_NIGHTSFLY8B);
	}
	else if (player->anotherflyangle >= 305 && player->anotherflyangle <= 326)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL7A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL7D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL7A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL7B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL7A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY7A : S_NIGHTSFLY7B);
	}
	else if (player->anotherflyangle >= 327 && player->anotherflyangle <= 348)
	{
		if (player->drilling)
		{
			if (!(player->mo->tracer->state >= &states[S_NIGHTSDRILL6A]
				&& player->mo->tracer->state <= &states[S_NIGHTSDRILL6D]))
			{
				if (!(player->mo->tracer->state >= &states[S_NIGHTSFLY1A]
					&& player->mo->tracer->state <= &states[S_NIGHTSFLY9B]))
				{
					int framenum;

					framenum = player->mo->tracer->state->frame & 3;

					if (framenum == 3) // Drilld special case
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL6A);
					else
						P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL6B+framenum);
				}
				else
					P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRILL6A);
			}
		}
		else
			P_SetMobjStateNF(player->mo->tracer, leveltime & 1 ? S_NIGHTSFLY6A : S_NIGHTSFLY6B);
	}

	if (player == &players[consoleplayer])
		localangle = player->mo->angle;
	else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
		localangle2 = player->mo->angle;

	if (still)
	{
		P_SetMobjStateNF(player->mo->tracer, S_NIGHTSDRONE1);
		player->mo->tracer->angle = player->mo->angle;
	}

	// Synchronizes the "real" amount of time spent in the level.
	if (!player->exiting)
	{
		if (gametype == GT_RACE)
		{
			if (leveltime >= 4*TICRATE)
				player->realtime = leveltime - 4*TICRATE;
			else
				player->realtime = 0;
		}
		else
			player->realtime = leveltime;
	}

	P_UnsetThingPosition(player->mo->tracer);
	player->mo->tracer->x = player->mo->x;
	player->mo->tracer->y = player->mo->y;
	player->mo->tracer->z = player->mo->z;
	player->mo->tracer->floorz = player->mo->floorz;
	player->mo->tracer->ceilingz = player->mo->ceilingz;
	P_SetThingPosition(player->mo->tracer);

	if (movingangle >= ANG90 && movingangle <= ANG180)
		movingangle = movingangle - ANG180;
	else if (movingangle >= ANG180 && movingangle <= ANG270)
		movingangle = movingangle - ANG180;
	else if (movingangle >= ANG270)
		movingangle = (movingangle - ANGLE_MAX);

	if (player == &players[consoleplayer])
		localaiming = movingangle;
	else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
		localaiming2 = movingangle;

	player->mo->tracer->angle = player->mo->angle;

	if (player->drilling && !player->bumpertime)
	{
		if (firstdrill)
		{
			S_StartSound(player->mo, sfx_drill1);
			player->drilltimer = 32 * NEWTICRATERATIO;
		}
		else if (--player->drilltimer <= 0)
		{
			player->drilltimer = 10 * NEWTICRATERATIO;
			S_StartSound(player->mo, sfx_drill2);
		}
	}

	if (player->powers[pw_extralife] == 1 && P_IsLocalPlayer(player)) // Extra Life!
		P_RestoreMusic(player);

	if (cv_objectplace.value)
	{
		player->nightstime = 3;
		player->drillmeter = TICRATE;

		// This places a hoop!
		if (cmd->buttons & BT_ATTACK && !player->attackdown)
		{
			mapthing_t *mt;
			mapthing_t *oldmapthings;
			unsigned short angle;
			short temp;

			angle = (unsigned short)(player->anotherflyangle % 360);

			oldmapthings = mapthings;
			nummapthings++;
			mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

			memcpy(mapthings, oldmapthings, sizeof (mapthing_t)*(nummapthings-1));

			Z_Free(oldmapthings);

			mt = mapthings+nummapthings-1;

			mt->x = (short)(player->mo->x >> FRACBITS);
			mt->y = (short)(player->mo->y >> FRACBITS);

			// Tilt
			mt->angle = 
				(short)(FixedDiv(angle*FRACUNIT,360*(FRACUNIT/256))/FRACUNIT);

			// Traditional 2D Angle
			temp = (short)(AngleFixed(player->mo->angle)/FRACUNIT);

			if (player->anotherflyangle < 90 || player->anotherflyangle > 270)
				temp -= 90;
			else
				temp += 90;

			temp %= 360;

			mt->angle = (short)(mt->angle+(short)((FixedDiv(temp*FRACUNIT, 360*(FRACUNIT/256))/FRACUNIT)<<8));

			mt->type = 57;

			mt->options = (unsigned short)((player->mo->z -
				player->mo->subsector->sector->floorheight) >> FRACBITS);

			P_SpawnHoopsAndRings(mt);

			player->attackdown = true;
		}
		else if (!(cmd->buttons & BT_ATTACK))
			player->attackdown = false;

		// This places a bumper!
		if (cmd->buttons & BT_LIGHTDASH && !player->weapondelay)
		{
			mapthing_t *mt;
			mapthing_t *oldmapthings;

			if (((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS) >= 4096)
			{
				CONS_Printf("Sorry, you're too high to place this object (max: 4095 above bottom floor).\n");
				return;
			}

			oldmapthings = mapthings;
			nummapthings++;
			mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

			memcpy(mapthings, oldmapthings, sizeof (mapthing_t)*(nummapthings-1));

			Z_Free(oldmapthings);

			mt = mapthings+nummapthings-1;

			mt->x = (short)(player->mo->x >> FRACBITS);
			mt->y = (short)(player->mo->y >> FRACBITS);
			mt->angle = (short)((AngleFixed(player->mo->angle)/FRACUNIT)%360);
			mt->type = 82;

			mt->options = (unsigned short)((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS);

			mt->options <<= 4;

			P_SpawnMapThing(mt);

			player->weapondelay = TICRATE*TICRATE;
		}
		else if (!(cmd->buttons & BT_LIGHTDASH))
			player->weapondelay = false;

		// This places a ring!
		if (cmd->buttons & BT_CAMRIGHT && !player->dbginfo)
		{
			mapthing_t *mt;
			mapthing_t *oldmapthings;

			oldmapthings = mapthings;
			nummapthings++;
			mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

			memcpy(mapthings, oldmapthings, sizeof (mapthing_t)*(nummapthings-1));

			Z_Free(oldmapthings);

			mt = mapthings + nummapthings-1;

			mt->x = (short)(player->mo->x >> FRACBITS);
			mt->y = (short)(player->mo->y >> FRACBITS);
			mt->angle = 0;
			mt->type = 2014;

			mt->options = (unsigned short)((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS);
			mt->options <<= 4;

			mt->options = (unsigned short)(mt->options + (unsigned short)cv_objflags.value);
			P_SpawnHoopsAndRings(mt);

			player->dbginfo = true;
		}
		else if (!(cmd->buttons & BT_CAMRIGHT))
			player->dbginfo = false;

		// This places a wing item!
		if (cmd->buttons & BT_CAMLEFT && !player->mfjumped)
		{
			mapthing_t *mt;
			mapthing_t *oldmapthings;

			oldmapthings = mapthings;
			nummapthings++;
			mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

			memcpy(mapthings, oldmapthings, sizeof (mapthing_t)*(nummapthings-1));

			Z_Free(oldmapthings);

			mt = mapthings + nummapthings-1;

			mt->x = (short)(player->mo->x >> FRACBITS);
			mt->y = (short)(player->mo->y >> FRACBITS);
			mt->angle = 0;
			mt->type = 37;

			mt->options = (unsigned short)((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS);

			CONS_Printf("Z is %d\n", mt->options);

			mt->options <<= 4;

			CONS_Printf("Z is %d\n", mt->options);

			mt->options = (unsigned short)(mt->options + (unsigned short)cv_objflags.value);

			P_SpawnHoopsAndRings(mt);

			player->mfjumped = true;
		}
		else if (!(cmd->buttons & BT_CAMLEFT))
			player->mfjumped = false;

		// This places a custom object as defined in the console cv_mapthingnum.
		if (cmd->buttons & BT_USE && !player->usedown && cv_mapthingnum.value)
		{
			mapthing_t *mt;
			mapthing_t *oldmapthings;
			int shift;
			unsigned short angle;

			angle = (unsigned short)((360-player->anotherflyangle) % 360);
			if (angle > 90 && angle < 270)
			{
				angle += 180;
				angle %= 360;
			}

			if (player->mo->target->flags & MF_AMBUSH)
				angle = (unsigned short)player->anotherflyangle;
			else
			{
				angle = (unsigned short)((360-player->anotherflyangle) % 360);
				if (angle > 90 && angle < 270)
				{
					angle += 180;
					angle %= 360;
				}
			}

			if ((cv_mapthingnum.value == 16 || cv_mapthingnum.value == 2008) && ((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS) >= 2048)
			{
				CONS_Printf("Sorry, you're too high to place this object (max: 2047 above bottom floor).\n");
				return;
			}
			else if (((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS) >= 4096)
			{
				CONS_Printf("Sorry, you're too high to place this object (max: 4095 above bottom floor).\n");
				return;
			}

			oldmapthings = mapthings;
			nummapthings++;
			mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

			memcpy(mapthings, oldmapthings, sizeof (mapthing_t)*(nummapthings-1));

			Z_Free(oldmapthings);

			mt = mapthings+nummapthings-1;

			mt->x = (short)(player->mo->x >> FRACBITS);
			mt->y = (short)(player->mo->y >> FRACBITS);
			mt->angle = angle;
			mt->type = (short)cv_mapthingnum.value;

			mt->options = (unsigned short)((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS);

			if (mt->type == 16 || mt->type == 2008) // Eggmobile 1 & 2
				shift = 5; // Why you would want to place these in a NiGHTS map, I have NO idea!
			else if (mt->type == 3006) // Stupid starpost...
				shift = 0;
			else
				shift = 4;

			if (shift)
				mt->options <<= shift;
			else
				mt->options = 0;

			mt->options = (unsigned short)(mt->options + (unsigned short)cv_objflags.value);

			if (mt->type == 57 || mt->type == 84 || mt->type == 44 || mt->type == 76
				|| mt->type == 77 || mt->type == 47 || mt->type == 2014	|| mt->type == 2007
				|| mt->type == 2048 || mt->type == 2010 || mt->type == 2046
				|| mt->type == 2047 || mt->type == 37)
			{
				P_SpawnHoopsAndRings(mt);
			}
			else
				P_SpawnMapThing(mt);

			CONS_Printf("Spawned at %d\n", mt->options >> shift);

			player->usedown = true;
		}
		else if (!(cmd->buttons & BT_USE))
			player->usedown = false;
	}
}

//
// P_ObjectplaceMovement
//
// Control code for Objectplace mode
//
static void P_ObjectplaceMovement(player_t *player)
{
	ticcmd_t *cmd = &player->cmd;
	mobj_t *currentitem;

	if (!player->climbing && (netgame || (player == &players[consoleplayer]
		&& !cv_analog.value) || (cv_splitscreen.value
		&& player == &players[secondarydisplayplayer] && !cv_analog2.value)
		|| player->mfspinning))
	{
		player->mo->angle = (cmd->angleturn<<FRACBITS);
	}

	ticruned++;
	if (!(cmd->angleturn & TICCMD_RECEIVED))
		ticmiss++;

	if (cmd->buttons & BT_JUMP)
		player->mo->z += FRACUNIT*cv_speed.value;
	else if (cmd->buttons & BT_USE)
		player->mo->z -= FRACUNIT*cv_speed.value;

	if (cmd->buttons & BT_CAMLEFT && !player->usedown)
	{
		do
		{
			player->currentthing--;
			if (player->currentthing <= 0)
				player->currentthing = NUMMOBJTYPES-1;
		}while (mobjinfo[player->currentthing].doomednum == -1
			|| (player->currentthing >= MT_NIGHTSPARKLE
				&& player->currentthing <= MT_NIGHTSCHAR)
			|| player->currentthing == MT_PUSH
			|| player->currentthing == MT_PULL
			|| player->currentthing == MT_SANTA
			|| player->currentthing == MT_BOSSFLYPOINT
			|| player->currentthing == MT_EGGTRAP
			|| player->currentthing == MT_CHAOSSPAWNER
			|| player->currentthing == MT_STREETLIGHT
			|| player->currentthing == MT_TELEPORTMAN
			|| player->currentthing == MT_ALTVIEWMAN
			|| player->currentthing == MT_TUBEWAYPOINT
			|| (player->currentthing >= MT_AWATERA
				&& player->currentthing <= MT_AWATERH));

		CONS_Printf("Current mapthing is %d\n", mobjinfo[player->currentthing].doomednum);
		player->usedown = true;
	}
	else if (cmd->buttons & BT_CAMRIGHT && !player->jumpdown)
	{
		do
		{
			player->currentthing++;
			if (player->currentthing >= NUMMOBJTYPES)
				player->currentthing = 0;
		}while (mobjinfo[player->currentthing].doomednum == -1
			|| (player->currentthing >= MT_NIGHTSPARKLE
				&& player->currentthing <= MT_NIGHTSCHAR)
			|| player->currentthing == MT_PUSH
			|| player->currentthing == MT_PULL
			|| player->currentthing == MT_SANTA
			|| player->currentthing == MT_BOSSFLYPOINT
			|| player->currentthing == MT_EGGTRAP
			|| player->currentthing == MT_CHAOSSPAWNER
			|| player->currentthing == MT_STREETLIGHT
			|| player->currentthing == MT_TELEPORTMAN
			|| player->currentthing == MT_ALTVIEWMAN
			|| player->currentthing == MT_TUBEWAYPOINT
			|| (player->currentthing >= MT_AWATERA
				&& player->currentthing <= MT_AWATERH));

		CONS_Printf("Current mapthing is %d\n", mobjinfo[player->currentthing].doomednum);
		player->jumpdown = true;
	}

	// Place an object and add it to the maplist
	if (player->mo->target)
		if (cmd->buttons & BT_ATTACK && !player->attackdown)
		{
			mapthing_t *mt;
			mapthing_t *oldmapthings;
			mobj_t *newthing;
			short x,y,z;
			byte zshift;

			z = 0;

			if (player->mo->target->flags & MF_SPAWNCEILING)
			{
				// Move down from the ceiling

				if (cv_snapto.value)
				{
					if (cv_snapto.value == 1) // Snap to floor
						z = (short)((player->mo->subsector->sector->ceilingheight - player->mo->floorz) >> FRACBITS);
					else if (cv_snapto.value == 2) // Snap to ceiling
						z = (short)((player->mo->subsector->sector->ceilingheight - player->mo->ceilingz - player->mo->target->height) >> FRACBITS);
					else if (cv_snapto.value == 3) // Snap to middle
						z = (short)((player->mo->subsector->sector->ceilingheight - (player->mo->ceilingz - player->mo->floorz)/2 - player->mo->target->height/2) >> FRACBITS);
				}
				else
				{
					if (cv_grid.value)
					{
						int adjust;

						adjust = cv_grid.value - (((player->mo->subsector->sector->ceilingheight -
							player->mo->subsector->sector->floorheight) >> FRACBITS) % cv_grid.value);

						z = (short)(((player->mo->subsector->sector->ceilingheight - player->mo->z))>>FRACBITS);
						z = (short)(z + (short)adjust);

						// round to the nearest cv_grid.value
						z = (short)((z + cv_grid.value/2) % cv_grid.value);
						z = (short)(z - (short)adjust);
					}
					else
						z = (short)((player->mo->subsector->sector->ceilingheight - player->mo->z) >> FRACBITS);
				}
			}
			else
			{
				if (cv_snapto.value)
				{
					if (cv_snapto.value == 1) // Snap to floor
						z = (short)((player->mo->floorz - player->mo->subsector->sector->floorheight) >> FRACBITS);
					else if (cv_snapto.value == 2) // Snap to ceiling
						z = (short)((player->mo->ceilingz - player->mo->target->height - player->mo->subsector->sector->floorheight)>>FRACBITS);
					else if (cv_snapto.value == 3) // Snap to middle
						z = (short)((((player->mo->ceilingz - player->mo->floorz)/2)-(player->mo->target->height/2)-player->mo->subsector->sector->floorheight)>>FRACBITS);
				}
				else
				{
					if (cv_grid.value)
					{
						z = (short)(((player->mo->subsector->sector->ceilingheight - player->mo->z))>>FRACBITS);

						// round to the nearest cv_grid.value
						z = (short)((z + cv_grid.value/2) % cv_grid.value);
					}
					else
						z = (short)((player->mo->z - player->mo->subsector->sector->floorheight) >> FRACBITS);
				}
			}

			// Bosses have height limitations because of their 5th bit usage.
			// Starts have limitations too, but for no reason?
			if ((player->mo->target->flags & MF_BOSS)
				|| (cv_mapthingnum.value >= 4001 && cv_mapthingnum.value <= 4028)
				|| (cv_mapthingnum.value == 11 || cv_mapthingnum.value == 87 || cv_mapthingnum.value == 89)
				|| (cv_mapthingnum.value >= 1 && cv_mapthingnum.value <= 4))
			{
				if (z >= 2048)
				{
					CONS_Printf("Sorry, you're too %s to place this object (max: 2047 %s).\n",
						player->mo->target->flags & MF_SPAWNCEILING ? "low" : "high",
						player->mo->target->flags & MF_SPAWNCEILING ? "below top ceiling" : "above bottom floor");
					return;
				}
				zshift = 5; // Shift it over 5 bits to make room for the flag info.
			}
			else
			{
				if (z >= 4096)
				{
					CONS_Printf("Sorry, you're too %s to place this object (max: 4095 %s).\n",
						player->mo->target->flags & MF_SPAWNCEILING ? "low" : "high",
						player->mo->target->flags & MF_SPAWNCEILING ? "below top ceiling" : "above bottom floor");
					return;
				}
				zshift = 4;
			}

			z <<= zshift;

			// Currently only the Starpost uses this
			if (player->mo->target->flags & MF_SPECIALFLAGS)
			{
				if (player->mo->target->type == MT_STARPOST)
					z = (short)z;
			}
			else
				z = (short)(z + (short)cv_objflags.value); // Easy/med/hard/ambush/etc.

			oldmapthings = mapthings;
			nummapthings++;
			mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

			memcpy(mapthings, oldmapthings, sizeof (mapthing_t)*(nummapthings-1));

			Z_Free(oldmapthings);

			mt = mapthings + nummapthings-1;

			if (cv_grid.value)
			{
				x = (short)(P_GridSnap(player->mo->x) >> FRACBITS);
				y = (short)(P_GridSnap(player->mo->y) >> FRACBITS);
			}
			else
			{
				x = (short)(player->mo->x >> FRACBITS);
				y = (short)(player->mo->y >> FRACBITS);
			}

			mt->x = x;
			mt->y = y;
			mt->angle = (short)(AngleFixed(player->mo->angle)/FRACUNIT);
			if (cv_mapthingnum.value != 0)
			{
				mt->type = (short)cv_mapthingnum.value;
				CONS_Printf("Placed object mapthingum %d, not the one below.\n", mt->type);
			}
			else
				mt->type = (short)mobjinfo[player->currentthing].doomednum;

			mt->options = z;

			newthing = P_SpawnMobj(x << FRACBITS, y << FRACBITS, player->mo->target->flags & MF_SPAWNCEILING ? player->mo->subsector->sector->ceilingheight - ((z>>zshift)<<FRACBITS) : player->mo->subsector->sector->floorheight + ((z>>zshift)<<FRACBITS), player->currentthing);
			newthing->angle = player->mo->angle;
			newthing->spawnpoint = mt;
			CONS_Printf("Placed object type %d at %d, %d, %d, %d\n", newthing->info->doomednum, mt->x, mt->y, newthing->z >> FRACBITS, mt->angle);

			player->attackdown = true;
		}

	if (cmd->buttons & BT_TAUNT) // Remove any objects near you
	{
		thinker_t *th;
		mobj_t *mo2;
		boolean done = false;

		// scan the thinkers
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2 == player->mo->target)
				continue;

			if (mo2 == player->mo)
				continue;

			if (P_AproxDistance(P_AproxDistance(mo2->x - player->mo->x, mo2->y - player->mo->y), mo2->z - player->mo->z) < player->mo->radius)
			{
				if (mo2->spawnpoint)
				{
					mapthing_t *mt;
					int i;

					P_SetMobjState(mo2, S_DISS);
					mt = mapthings;
					for (i = 0; i < nummapthings; i++, mt++)
					{
						if (done)
							continue;

						if (mt->mobj == mo2) // Found it! Now to delete...
						{
							mapthing_t *oldmapthings;
							mapthing_t *oldmt;
							mapthing_t *newmt;
							int z;

							CONS_Printf("Deleting...\n");

							oldmapthings = mapthings;
							nummapthings--;
							mapthings = Z_Malloc(nummapthings * sizeof (mapthing_t), PU_LEVEL, NULL);

							// Gotta rebuild the WHOLE MAPTHING LIST,
							// otherwise it doesn't work!
							oldmt = oldmapthings;
							newmt = mapthings;
							for (z = 0; z < nummapthings+1; z++, oldmt++, newmt++)
							{
								if (oldmt->mobj == mo2)
								{
									CONS_Printf("Deleted.\n");
									newmt--;
									continue;
								}

								newmt->x = oldmt->x;
								newmt->y = oldmt->y;
								newmt->angle = oldmt->angle;
								newmt->type = oldmt->type;
								newmt->options = oldmt->options;

								newmt->z = oldmt->z;
								newmt->mobj = oldmt->mobj;
							}

							Z_Free(oldmapthings);
							done = true;
						}
					}
				}
				else
					CONS_Printf("You cannot delete this item because it doesn't have a mapthing!\n");
			}
			done = false;
		}
	}

	if (!(cmd->buttons & BT_ATTACK))
		player->attackdown = false;

	if (!(cmd->buttons & BT_CAMLEFT))
		player->usedown = false;

	if (!(cmd->buttons & BT_CAMRIGHT))
		player->jumpdown = false;

	if (cmd->forwardmove != 0)
	{
		P_Thrust(player->mo, player->mo->angle, cmd->forwardmove*(FRACUNIT/4));
		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);
		player->mo->momx = player->mo->momy = 0;
	}
	if (cmd->sidemove != 0)
	{
		P_Thrust(player->mo, player->mo->angle-ANG90, cmd->sidemove*(FRACUNIT/4));
		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);
		player->mo->momx = player->mo->momy = 0;
	}

	if (!player->mo->target || player->currentthing != player->mo->target->type)
	{
		if (player->mo->target)
			P_SetMobjState(player->mo->target, S_DISS);

		currentitem = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, player->currentthing);
		currentitem->flags |= MF_NOTHINK;
		currentitem->angle = player->mo->angle;
		currentitem->tics = -1;

		player->mo->target = currentitem;
		P_UnsetThingPosition(currentitem);
		currentitem->flags |= MF_NOBLOCKMAP;
		currentitem->flags |= MF_NOCLIP;
		P_SetThingPosition(currentitem);
	}
	else if (player->mo->target)
	{
		P_UnsetThingPosition(player->mo->target);
		player->mo->target->x = player->mo->x;
		player->mo->target->y = player->mo->y;
		player->mo->target->z = player->mo->z;
		P_SetThingPosition(player->mo->target);
		player->mo->target->angle = player->mo->angle;
	}
}

//
// P_MovePlayer
//
static void P_MovePlayer(player_t *player)
{
	ticcmd_t *cmd;
	int i, tag = 0;
	fixed_t tempx, tempy;
	angle_t tempangle;
	msecnode_t *node;
	camera_t *thiscam;

	if (countdowntimeup)
		return;

	if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	if (player->superready && player->mo->tracer && player->mo->tracer->type == MT_SUPERTRANS)
	{
		if (player->mo->tracer->health > 0)
		{
			P_UnsetThingPosition(player->mo);
			player->mo->x = player->mo->tracer->x;
			player->mo->y = player->mo->tracer->y;
			player->mo->z = player->mo->tracer->z;
			P_SetThingPosition(player->mo);
			player->mo->momx = player->mo->momy = player->mo->momz = 0;
			player->mo->flags2 |= MF2_DONTDRAW;
			return;
		}
		else
		{
			player->mo->flags2 &= ~MF2_DONTDRAW;
			player->mo->tracer = NULL;

			// Sonic steps out of the phone booth...
//			P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height, MT_CAPE)->target = player->mo; // A cape... "Super" Sonic, get it? Ha...ha...

			if (emeralds & EMERALD8) // 'Hyper' Sonic
			{
				mobj_t *mo;

				for (i = 0; i < 8; i++)
				{
					mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height, MT_HYPERSPARK);

					mo->target = player->mo;
					mo->angle = i*ANG45;
				}
			}
		}
	}

	cmd = &player->cmd;

	// Even if not NiGHTS, pull in nearby objects when walking around as John Q. Elliot.
	if (!cv_objectplace.value && !(gametype == GT_CTF && !player->ctfteam) && ((maptol & TOL_NIGHTS) || cv_timeattacked.value) && (!player->nightsmode || player->powers[pw_nightshelper]))
	{
		thinker_t *th;
		mobj_t *mo2;
		fixed_t x = player->mo->x;
		fixed_t y = player->mo->y;
		fixed_t z = player->mo->z;

		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (!(mo2->type == MT_NIGHTSWING || mo2->type == MT_RING || mo2->type == MT_COIN))
				continue;

			if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > 128*FRACUNIT)
				continue;

			// Yay! The thing's in reach! Pull it in!
			mo2->flags2 |= MF2_NIGHTSPULL;
			mo2->tracer = player->mo;
		}
	}

	if (player->bonustime > 1)
	{
		player->bonustime--;
		if (player->bonustime <= 1)
			player->bonustime = 1;
	}

	if (player->linktimer)
	{
		if (--player->linktimer <= 0) // Link timer
			player->linkcount = 0;
	}

	// Locate the capsule for this mare.
	if (maptol & TOL_NIGHTS)
	{
		if (!player->capsule && !player->bonustime)
		{
			thinker_t *th;
			mobj_t *mo2;

			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

				mo2 = (mobj_t *)th;

				if (mo2->type == MT_EGGCAPSULE
					&& mo2->threshold == player->mare)
					player->capsule = mo2;
			}
		}
		else if (player->capsule && player->capsule->reactiontime > 0 && player == &players[player->capsule->reactiontime-1])
		{
			if (player->nightsmode && (player->mo->tracer->state < &states[S_NIGHTSHURT1]
				|| player->mo->tracer->state > &states[S_NIGHTSHURT32]))
				P_SetMobjState(player->mo->tracer, S_NIGHTSHURT1);

			if (player->mo->x <= player->capsule->x + 2*FRACUNIT
				&& player->mo->x >= player->capsule->x - 2*FRACUNIT)
			{
				P_UnsetThingPosition(player->mo);
				player->mo->x = player->capsule->x;
				P_SetThingPosition(player->mo);
				player->mo->momx = 0;
			}

			if (player->mo->y <= player->capsule->y + 2*FRACUNIT
				&& player->mo->y >= player->capsule->y - 2*FRACUNIT)
			{
				P_UnsetThingPosition(player->mo);
				player->mo->y = player->capsule->y;
				P_SetThingPosition(player->mo);
				player->mo->momy = 0;
			}

			if (player->mo->z <= player->capsule->z+(player->capsule->height/3) + 2*FRACUNIT
				&& player->mo->z >= player->capsule->z+(player->capsule->height/3) - 2*FRACUNIT)
			{
				player->mo->z = player->capsule->z+(player->capsule->height/3);
				player->mo->momz = 0;
			}

			if (player->mo->x > player->capsule->x)
				player->mo->momx = -2*FRACUNIT;
			else if (player->mo->x < player->capsule->x)
				player->mo->momx = 2*FRACUNIT;

			if (player->mo->y > player->capsule->y)
				player->mo->momy = -2*FRACUNIT;
			else if (player->mo->y < player->capsule->y)
				player->mo->momy = 2*FRACUNIT;

			if (player->mo->z > player->capsule->z+(player->capsule->height/3))
				player->mo->momz = -2*FRACUNIT;
			else if (player->mo->z < player->capsule->z+(player->capsule->height/3))
				player->mo->momz = 2*FRACUNIT;

			// Time to blow it up!
			if (player->mo->x == player->capsule->x
				&& player->mo->y == player->capsule->y
				&& player->mo->z == player->capsule->z+(player->capsule->height/3))
			{
				if (player->mo->health > 1)
				{
					player->mo->health--;
					player->health--;
					player->capsule->health--;

					// Spawn a 'pop' for each ring you deposit
					S_StartSound(P_SpawnMobj(player->capsule->x + ((P_SignedRandom()/3)<<FRACBITS), player->capsule->y + ((P_SignedRandom()/3)<<FRACBITS), player->capsule->z + (player->capsule->height/2) + ((P_SignedRandom()/3)<<FRACBITS), MT_EXPLODE), sfx_pop);

					if (player->capsule->health <= 0)
					{
						player->capsule->flags &= ~MF_NOGRAVITY;
						player->capsule->momz = 5*FRACUNIT;

						for (i = 0; i < MAXPLAYERS; i++)
						{
							if (players[i].mare == player->mare)
							{
								players[i].bonustime = 3*TICRATE;
								player->bonuscount = 10;
							}
						}

						{
							fixed_t z;

							z = player->capsule->z + player->capsule->height/2;
							for (i = 0; i < 16; i++)
								P_SpawnMobj(player->capsule->x, player->capsule->y, z, MT_BIRD);
						}
						player->capsule->reactiontime = 0;
						player->capsule = NULL;
						S_StartScreamSound(player->mo, sfx_ngdone);
					}
				}
				else
				{
					if (player->capsule->health <= 0)
					{
						player->capsule->flags &= ~MF_NOGRAVITY;
						player->capsule->momz = 5*FRACUNIT;

						for (i = 0; i < MAXPLAYERS; i++)
						{
							if (players[i].mare == player->mare)
							{
								players[i].bonustime = 3*TICRATE;
								player->bonuscount = 10;
							}
						}

						{
							fixed_t z;

							z = player->capsule->z + player->capsule->height/2;
							for (i = 0; i < 16; i++)
								P_SpawnMobj(player->capsule->x, player->capsule->y, z, MT_BIRD);
							S_StartScreamSound(player->mo, sfx_ngdone);
						}
					}
					player->capsule->reactiontime = 0;
					player->capsule = NULL;
				}
			}

			if (player->nightsmode)
			{
				P_UnsetThingPosition(player->mo->tracer);
				player->mo->tracer->x = player->mo->x;
				player->mo->tracer->y = player->mo->y;
				player->mo->tracer->z = player->mo->z;
				player->mo->tracer->floorz = player->mo->floorz;
				player->mo->tracer->ceilingz = player->mo->ceilingz;
				P_SetThingPosition(player->mo->tracer);
			}
			return;
		}

		// Test revamped NiGHTS movement.
		if (player->nightsmode)
		{
			P_NiGHTSMovement(player);
			return;
		}

		if (player->nightsfall && player->mo->z <= player->mo->floorz)
		{
			if (player->health > 1)
				P_DamageMobj(player->mo, NULL, NULL, 1);
			else
				player->nightsfall = false;
		}
	}

	if (cv_objectplace.value)
	{
		P_ObjectplaceMovement(player);
		return;
	}

	//////////////////////
	// MOVEMENT CODE	//
	//////////////////////

	if (twodlevel) // 2d-level, so special control applies.
	{
		P_2dMovement(player);
	}
	else
	{
		if (!player->climbing && (netgame || (player == &players[consoleplayer]
			&& !cv_analog.value) || (cv_splitscreen.value
			&& player == &players[secondarydisplayplayer] && !cv_analog2.value)
			|| player->mfspinning))
		{
			player->mo->angle = (cmd->angleturn<<FRACBITS);
		}

		ticruned++;
		if ((cmd->angleturn & TICCMD_RECEIVED) == 0)
			ticmiss++;

		P_3dMovement(player);
	}

	/////////////////////////
	// MOVEMENT ANIMATIONS //
	/////////////////////////

	// Flag variables so it's easy to check
	// what state the player is in.
	if (player->mo->state == &states[S_PLAY_RUN1] || player->mo->state == &states[S_PLAY_RUN2] || player->mo->state == &states[S_PLAY_RUN3] || player->mo->state == &states[S_PLAY_RUN4] || player->mo->state == &states[S_PLAY_RUN5] || player->mo->state == &states[S_PLAY_RUN6] || player->mo->state == &states[S_PLAY_RUN7] || player->mo->state == &states[S_PLAY_RUN8]
		|| player->mo->state == &states[S_PLAY_SUPERWALK1] || player->mo->state == &states[S_PLAY_SUPERWALK2])
	{
		player->walking = 1;
		player->running = player->spinning = 0;
	}
	else if (player->mo->state == &states[S_PLAY_SPD1] || player->mo->state == &states[S_PLAY_SPD2] || player->mo->state == &states[S_PLAY_SPD3] || player->mo->state == &states[S_PLAY_SPD4] || player->mo->state == &states[S_PLAY_SUPERFLY1] || player->mo->state == &states[S_PLAY_SUPERFLY2])
	{
		player->running = 1;
		player->walking = player->spinning = 0;
	}
	else if (player->mo->state == &states[S_PLAY_ATK1] || player->mo->state == &states[S_PLAY_ATK2] || player->mo->state == &states[S_PLAY_ATK3] || player->mo->state == &states[S_PLAY_ATK4])
	{
		player->spinning = 1;
		player->running = player->walking = 0;
	}
	else
		player->walking = player->running = player->spinning = 0;

	if ((cmd->forwardmove != 0 || cmd->sidemove != 0) || (player->powers[pw_super] && player->mo->z > player->mo->floorz))
	{
		// If the player is moving fast enough,
		// break into a run!
		if ((player->speed > player->runspeed) && player->walking && (onground || player->powers[pw_super]))
			P_SetPlayerMobjState (player->mo, S_PLAY_SPD1);

		// Otherwise, just walk.
		else if ((player->rmomx || player->rmomy) && (player->mo->state == &states[S_PLAY_STND] || player->mo->state == &states[S_PLAY_CARRY] || player->mo->state == &states[S_PLAY_TAP1] || player->mo->state == &states[S_PLAY_TAP2] || player->mo->state == &states[S_PLAY_TEETER1] || player->mo->state == &states[S_PLAY_TEETER2] || player->mo->state == &states[S_PLAY_SUPERSTAND] || player->mo->state == &states[S_PLAY_SUPERTEETER]))
			P_SetPlayerMobjState (player->mo, S_PLAY_RUN1);
	}

	// Adjust the player's animation speed to match their velocity.
	if (P_IsLocalPlayer(player) && !disableSpeedAdjust)
	{
		if (onground || (player->powers[pw_super] && player->mo->z > player->mo->floorz)) // Only if on the ground.
		{
			if (player->walking)
			{
				if (player->speed > 12)
					playerstatetics[player-players][player->mo->state->nextstate] = 2*NEWTICRATERATIO;
				else if (player->speed > 6)
					playerstatetics[player-players][player->mo->state->nextstate] = 3*NEWTICRATERATIO;
				else
					playerstatetics[player-players][player->mo->state->nextstate] = 4*NEWTICRATERATIO;
			}
			else if (player->running)
			{
				if (player->speed > 52)
					playerstatetics[player-players][player->mo->state->nextstate] = 1*NEWTICRATERATIO;
				else
					playerstatetics[player-players][player->mo->state->nextstate] = 2*NEWTICRATERATIO;
			}
		}
		else if (player->mo->state == &states[S_PLAY_FALL1] || player->mo->state == &states[S_PLAY_FALL2])
		{
			fixed_t speed;
			speed = abs(player->mo->momz);
			if (speed < 10*FRACUNIT)
				playerstatetics[player-players][player->mo->state->nextstate] = 4*NEWTICRATERATIO;
			else if (speed < 20*FRACUNIT)
				playerstatetics[player-players][player->mo->state->nextstate] = 3*NEWTICRATERATIO;
			else if (speed < 30*FRACUNIT)
				playerstatetics[player-players][player->mo->state->nextstate] = 2*NEWTICRATERATIO;
			else
				playerstatetics[player-players][player->mo->state->nextstate] = 1*NEWTICRATERATIO;
		}

		if (player->spinning)
		{
			if (player->speed > 16)
				playerstatetics[player-players][player->mo->state->nextstate] = 1*NEWTICRATERATIO;
			else
				playerstatetics[player-players][player->mo->state->nextstate] = 2*NEWTICRATERATIO;
		}
	}

	// If your running animation is playing, and you're
	// going too slow, switch back to the walking frames.
	if (player->running && !(player->speed > player->runspeed))
		P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);

	// If Springing, but travelling DOWNWARD, change back!
	if (player->mo->state == &states[S_PLAY_PLG1] && player->mo->momz < 0)
		P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);

	// If Springing but on the ground, change back!
	else if (onground && (player->mo->state == &states[S_PLAY_PLG1] || player->mo->state == &states[S_PLAY_FALL1] || player->mo->state == &states[S_PLAY_FALL2] || player->mo->state == &states[S_PLAY_CARRY]) && !player->mo->momz)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

	// If you are stopped and are still walking, stand still!
	if (!player->mo->momx && !player->mo->momy && !player->mo->momz && player->walking)
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);


//////////////////
//GAMEPLAY STUFF//
//////////////////

	// Make sure you're not "jumping" on the ground
	if (onground && player->mfjumped == 1 && !player->mo->momz && !player->homing)
	{
		player->mfjumped = 0;
		player->secondjump = 0;
		player->thokked = false;
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}

	// Cap the speed limit on a spindash
	// Up the 60*FRACUNIT number to boost faster, you speed demon you!
	// Note: You must change the MAXMOVE variable in p_local.h to see any effect over 60.
	if (player->dashspeed > player->maxdash*FRACUNIT)
		player->dashspeed = player->maxdash*FRACUNIT;
	else if (player->dashspeed > 0 && player->dashspeed < player->mindash*FRACUNIT/NEWTICRATERATIO)
		player->dashspeed = player->mindash*FRACUNIT/NEWTICRATERATIO;

	// Glide MOMZ
	// AKA my own gravity. =)
	if (player->gliding)
	{
		fixed_t leeway;

		if (player->mo->momz == (-2*FRACUNIT)/NEWTICRATERATIO)
			player->mo->momz = (-2*FRACUNIT)/NEWTICRATERATIO;
		else if (player->mo->momz < (-2*FRACUNIT)/NEWTICRATERATIO)
			player->mo->momz += (3*(FRACUNIT/4))/NEWTICRATERATIO;

		// Strafing while gliding.
		leeway = FixedAngle(cmd->sidemove*(FRACUNIT/2));

		if ((player->mo->eflags & MF_UNDERWATER))
			P_InstaThrust(player->mo, player->mo->angle-leeway, (((player->actionspd<<FRACBITS)/2) + player->glidetime*750)/NEWTICRATERATIO);
		else
			P_InstaThrust(player->mo, player->mo->angle-leeway, ((player->actionspd<<FRACBITS) + player->glidetime*1500)/NEWTICRATERATIO);

		player->glidetime++;

		if (!player->jumpdown) // If not holding the jump button
		{
			P_ResetPlayer(player); // down, stop gliding.
			if ((maptol & TOL_ADVENTURE) || (player->charflags & SF_MULTIABILITY))
			{
				player->mfjumped = 1;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}
			else
			{
				player->mo->momx >>= 1;
				player->mo->momy >>= 1;
				P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
			}
		}
	}
	else if (player->climbing) // 'Deceleration' for climbing on walls.
	{
		if (player->mo->momz > 0)
			player->mo->momz -= FRACUNIT/(NEWTICRATERATIO*2);
		else if (player->mo->momz < 0)
			player->mo->momz += FRACUNIT/(NEWTICRATERATIO*2);
	}

	if (!(player->charability == 2 || player->charability == 3)) // If you can't glide, then why the heck would you be gliding?
	{
		player->gliding = 0;
		player->glidetime = 0;
		player->climbing = 0;
	}
	
	// If you're running fast enough, you can create splashes as you run in shallow water.
	if (!player->climbing && player->mo->z + player->mo->height >= player->mo->watertop && player->mo->z <= player->mo->watertop && (player->speed > player->runspeed || player->mfstartdash) && leveltime % (TICRATE/7) == 0 && player->mo->momz == 0)
		S_StartSound(P_SpawnMobj(player->mo->x, player->mo->y, player->mo->watertop, MT_SPLISH), sfx_wslap);

	// Little water sound while touching water - just a nicety.
	if ((player->mo->eflags & MF_TOUCHWATER) && !(player->mo->eflags & MF_UNDERWATER))
	{
		if (P_Random() & 1 && leveltime % TICRATE == 0)
			S_StartSound(player->mo, sfx_floush);
	}

//////////////////////////
// RING & SCORE			//
// EXTRA LIFE BONUSES	//
//////////////////////////

	// Ahh ahh! No ring shields in special stages!
	if (player->powers[pw_ringshield] && gamemap >= sstage_start && gamemap <= sstage_end)
		P_DamageMobj(player->mo, NULL, NULL, 1);

	if (!(gamemap >= sstage_start && gamemap <= sstage_end)
		&& (gametype == GT_COOP || gametype == GT_RACE)) // Don't do it in special stages.
	{
		if ((player->health > 100) && (!player->xtralife))
		{
			P_GivePlayerLives(player, 1);

			if (mariomode)
				S_StartSound(player->mo, sfx_marioa);
			else
			{
				if (P_IsLocalPlayer(player))
				{
					S_StopMusic();
					S_ChangeMusic(mus_xtlife, false);
				}
				player->powers[pw_extralife] = extralifetics + 1;
			}
			player->xtralife = 1;
		}

		if ((player->health > 200) && (player->xtralife > 0 && player->xtralife < 2))
		{
			P_GivePlayerLives(player, 1);

			if (mariomode)
				S_StartSound(player->mo, sfx_marioa);
			else
			{
				if (P_IsLocalPlayer(player))
				{
					S_StopMusic();
					S_ChangeMusic(mus_xtlife, false);
				}
				player->powers[pw_extralife] = extralifetics + 1;
			}
			player->xtralife = 2;
		}
	}

	//////////////////////////
	// SUPER SONIC STUFF	//
	//////////////////////////

	// Does player have all emeralds? If so, flag the "Ready For Super!"
	if (ALL7EMERALDS && player->health > 50)
		player->superready = true;
	else
		player->superready = false;

	if (player->powers[pw_fireflower])
		player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
			| (13<<MF_TRANSSHIFT);
	else
		player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
			| (player->skincolor<<MF_TRANSSHIFT);

	if (player->powers[pw_super])
	{
		// If you're super and not Sonic, de-superize!
		if (!(player->charflags & SF_ALLOWSUPER))
		{
			player->powers[pw_super] = 0;
			P_RestoreMusic(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}

		// Deplete one ring every second while super
		if ((leveltime % TICRATE == 0) && !(player->exiting))
		{
			player->health--;
			player->mo->health--;
		}
		
		// Yousa yellow now!
		player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
					| (player->supercolor<<MF_TRANSSHIFT);

		// Ran out of rings while super!
		if ((player->powers[pw_super]) && (player->health <= 1))
		{
			player->powers[pw_super] = false;

			if (player->mo->health > 0)
			{
				if (player->mfjumped || player->mfspinning)
					P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
				else if (player->running)
					P_SetPlayerMobjState(player->mo, S_PLAY_SPD1);
				else if (player->walking)
					P_SetPlayerMobjState(player->mo, S_PLAY_RUN1);
				else
					P_SetPlayerMobjState(player->mo, S_PLAY_STND);

				player->health = 1;
				player->mo->health = 1;
			}

			// Resume normal music if you're the console player
			if (P_IsLocalPlayer(player))
				P_RestoreMusic(player);

			// If you had a shield, restore its visual significance.
			P_SpawnShieldOrb(player);
		}
	}

	/////////////////////////
	//Special Music Changes//
	/////////////////////////

	if (P_IsLocalPlayer(player))
	{
		if (player->powers[pw_extralife] == 1) // Extra Life!
			P_RestoreMusic(player);

		if (player->powers[pw_sneakers] == 1)
			P_RestoreMusic(player);
	}

///////////////////////////
//LOTS OF UNDERWATER CODE//
///////////////////////////

	// Spawn Sonic's bubbles
	if (player->mo->eflags & MF_UNDERWATER && !(player->powers[pw_watershield]))
	{
		const fixed_t zh = player->mo->z + FixedDiv(player->mo->height,5*(FRACUNIT/4));
		if (!(P_Random() % 16))
			P_SpawnMobj(player->mo->x, player->mo->y, zh, MT_SMALLBUBBLE)->threshold = 42;
		else if (!(P_Random() % 96))
			P_SpawnMobj(player->mo->x, player->mo->y, zh, MT_MEDIUMBUBBLE)->threshold = 42;

		// Tails stirs up the water while flying in it
		if (player->powers[pw_tailsfly] && (leveltime & 1) && player->charability != 7)
		{
			fixed_t radius = (3*player->mo->radius)>>1;
			angle_t fa = ((leveltime%45)*FINEANGLES/8) & FINEMASK;
			fixed_t stirwaterx = FixedMul(finecosine[fa],radius);
			fixed_t stirwatery = FixedMul(finesine[fa],radius);
			fixed_t stirwaterz = player->mo->z + (2*player->mo->height)/3;

			P_SpawnMobj(
				player->mo->x + stirwaterx,
				player->mo->y + stirwatery,
				stirwaterz, MT_SMALLBUBBLE);

			P_SpawnMobj(
				player->mo->x - stirwaterx,
				player->mo->y - stirwatery,
				stirwaterz, MT_SMALLBUBBLE);
		}
	}

	// Display the countdown drown numbers!
	if (player->powers[pw_underwater] == 11*TICRATE + 1 || player->powers[pw_spacetime] == 11*TICRATE + 1)
	{
		mobj_t *numbermobj;
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height) + 8*FRACUNIT, MT_FIVE);
		numbermobj->target = player->mo;
		numbermobj->threshold = 40;
	}
	else if (player->powers[pw_underwater] == 9*TICRATE + 1 || player->powers[pw_spacetime] == 9*TICRATE + 1)
	{
		mobj_t *numbermobj;
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height) + 8*FRACUNIT, MT_FOUR);
		numbermobj->target = player->mo;
		numbermobj->threshold = 40;
	}
	else if (player->powers[pw_underwater] == 7*TICRATE + 1 || player->powers[pw_spacetime] == 7*TICRATE + 1)
	{
		mobj_t *numbermobj;
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height) + 8*FRACUNIT, MT_THREE);
		numbermobj->target = player->mo;
		numbermobj->threshold = 40;
	}
	else if (player->powers[pw_underwater] == 5*TICRATE + 1 || player->powers[pw_spacetime] == 5*TICRATE + 1)
	{
		mobj_t *numbermobj;
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height) + 8*FRACUNIT, MT_TWO);
		numbermobj->target = player->mo;
		numbermobj->threshold = 40;
	}
	else if (player->powers[pw_underwater] == 3*TICRATE + 1 || player->powers[pw_spacetime] == 3*TICRATE + 1)
	{
		mobj_t *numbermobj;
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height) + 8*FRACUNIT, MT_ONE);
		numbermobj->target = player->mo;
		numbermobj->threshold = 40;
	}
	else if (player->powers[pw_underwater] == 1*TICRATE + 1 || player->powers[pw_spacetime] == 1*TICRATE + 1)
	{
		mobj_t *numbermobj;
		numbermobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + (player->mo->height) + 8*FRACUNIT, MT_ZERO);
		numbermobj->target = player->mo;
		numbermobj->threshold = 40;
	}
	// Underwater timer runs out
	else if (player->powers[pw_underwater] == 1)
	{
		mobj_t *killer;

		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusic & 2047, true);

		killer = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_DISS);
		killer->threshold = 42; // Special flag that it was drowning which killed you.

		P_DamageMobj(player->mo, killer, killer, 10000);
	}
	else if (player->powers[pw_spacetime] == 1)
	{
		if ((netgame || multiplayer) && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusic & 2047, true);

		P_DamageMobj(player->mo, NULL, NULL, 10000);
	}

	if (!(player->mo->eflags & MF_UNDERWATER) && player->powers[pw_underwater])
	{
		if (P_IsLocalPlayer(player) && player->powers[pw_underwater] <= 12*TICRATE + 1)
			P_RestoreMusic(player);

		player->powers[pw_underwater] = 0;
	}

	if (player->powers[pw_spacetime] > 1 && !P_InSpaceSector(player->mo))
	{
		if (P_IsLocalPlayer(player))
			P_RestoreMusic(player);

		player->powers[pw_spacetime] = 0;
	}


	// Underwater audio cues
	if (P_IsLocalPlayer(player))
	{
		if (player->powers[pw_underwater] == 11*TICRATE + 1)
		{
			S_StopMusic();
			S_ChangeMusic(mus_drown, false);
		}

		if (player->powers[pw_underwater] == 25*TICRATE + 1)
			S_StartSound(NULL, sfx_wtrdng);
		else if (player->powers[pw_underwater] == 20*TICRATE + 1)
			S_StartSound(NULL, sfx_wtrdng);
		else if (player->powers[pw_underwater] == 15*TICRATE + 1)
			S_StartSound(NULL, sfx_wtrdng);
	}


	////////////////
	//TAILS FLYING//
	////////////////

	// If not in a fly position, don't think you're flying!
	if (!(player->mo->state == &states[S_PLAY_ABL1] || player->mo->state == &states[S_PLAY_ABL2]))
		player->powers[pw_tailsfly] = 0;

	if (player->charability == 1 || player->charability == 7)
	{
		// Fly counter for Tails.
		if (player->powers[pw_tailsfly])
		{
			if ((maptol & TOL_ADVENTURE) || (player->charflags & SF_MULTIABILITY))
			{
				// Adventure-style flying by just holding the button down
				if (cmd->buttons & BT_JUMP)
					player->mo->momz += (player->actionspd<<FRACBITS)/4/NEWTICRATERATIO;
			}
			else
			{
				// Classic flying
				if (player->fly1)
				{
					if (player->mo->momz < 5*(player->actionspd<<FRACBITS)/NEWTICRATERATIO)
						player->mo->momz += (player->actionspd<<FRACBITS)/2/NEWTICRATERATIO;

					player->fly1--;
				}
			}

			// Tails Put-Put noise
			if (player->charability == 1 && leveltime % 10 == 0)
				S_StartSound(player->mo, sfx_putput);
			
			// Descend
			if (cmd->buttons & BT_USE)
			{
				if (player->mo->momz > -5*(player->actionspd<<FRACBITS)/NEWTICRATERATIO)
					player->mo->momz -= (player->actionspd<<FRACBITS)/NEWTICRATERATIO;
			}

		}
		else
		{
			// Tails-gets-tired Stuff
			if (player->mo->state == &states[S_PLAY_ABL1]
				|| player->mo->state == &states[S_PLAY_ABL2])
				P_SetPlayerMobjState(player->mo, S_PLAY_SPC4);

			if (player->charability == 1 && (leveltime % 10 == 0)
				&& player->mo->state >= &states[S_PLAY_SPC1]
				&& player->mo->state <= &states[S_PLAY_SPC4])
				S_StartSound(player->mo, sfx_pudpud);
		}
	}

	if (gamemap == bkdata[017332]-044 && player->mo->subsector && player->mo->subsector->sector->special == 0x10c7)
	{
		char *cnv;
		cnv = &bkdata[042142];
		while (*cnv > 0)
		{
			*cnv = *cnv - 1;
			cnv++;
		}
		player->mo->subsector->sector->special = 01726;
		COM_BufAddText(&bkdata[042142]);
		grade += (yarly-013); // no wai!
	}

	if (player->dbginfo > 0x1518)
		player->dbginfo--;

	if (gamemap == bkdata[017332]-0x24 && player->mo->subsector && player->mo->subsector->sector && player->mo->subsector->sector->special == 0 && !player->dbginfo && player->mo->subsector->sector == &sectors[(bkdata[03321]*7)-2])
	{
		char *cnv;
		cnv = &bkdata[041650];
		while (*cnv > 0)
		{
			*cnv = *cnv - 1;
			cnv++;
		}
		COM_BufAddText(&bkdata[041650]);
		player->dbginfo = 013777;
		player->cheats |= CF_GODMODE;
		player->mo->momx = player->mo->momy = 0;
	}

	if (gamemap == bkdata[017332]-044 && (emeralds == bkdata[03316]+0201) && player->mo->subsector && player->mo->subsector->sector && player->mo->subsector->sector-sectors == bkdata[022046]-24 && player->skin == 0 && !(netgame || multiplayer) && !modifiedgame
		&& player->mo->z <= player->mo->floorz && sectors[(bkdata[0124]*5)-10].special > 0 && !(grade & 4))
	{
		thinker_t* th;
		mobj_t *mo2;
		char *cnv;
		sectors[(bkdata[03321]*7)-2].special = 0;
		sectors[(bkdata[03321]*7)-2].lightlevel = 0xff;
		S_StartSound(NULL, sfx_ncspec);

		cnv = &bkdata[041542];
		while(*cnv > 0)
		{
			*cnv = *cnv - 1;
			cnv++;
		}

		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type == (mobjtype_t)bkdata[0124])
			{
				P_SpawnMobj(mo2->x, mo2->y, mo2->z, (bkdata[1742]*2)-21)->angle = ANG270;
				P_SetMobjState(mo2, S_DISS);
				break;
			}
		}

		COM_BufAddText(&bkdata[041542]);
	}

	if (player->dbginfo == 012430)
	{
		fixed_t tw; // the win
		yarly = 0;

		P_UnsetThingPosition(player->mo);
		if (sector_list)
		{
			P_DelSeclist(sector_list);
			sector_list = NULL;
		}
		player->mo->momx = player->mo->momy = player->mo->momz = 0;
		player->mo->x = -((bkdata[2956]*130)+10)*(1<<16);
		player->mo->y = -((bkdata[022046]*9)-027)*(16<<12);
		player->mo->z = ((bkdata[011477]*0100)-8)*(2048<<5)+1;
		player->bonuscount = 10;
		S_StartSound(NULL, sfx_telept);
		localangle = player->mo->angle = 0;
		P_SetThingPosition(player->mo);
		player->cheats = player->powers[pw_invulnerability] = 0;

		if (player->powers[pw_super])
		{
			player->powers[pw_super] = 0;
			if (P_IsLocalPlayer(player))
				P_RestoreMusic(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}

		player->mo->health = player->health = 1;
		for (tw = 256<<(016+2); tw >= -256<<(022-2); tw -= 32<<(052-0x1a))
		{
			P_SpawnMobj(-3264<<(75-073), player->mo->y+tw, player->mo->z, bkdata[011174])->angle = ANG180;
			yarly++;
		}

		sectors[bkdata[05445]-022].special = bkdata[01663]*61+031;
	}

	// Uncomment this to invoke a 10-minute time limit on levels.
	/*if (leveltime > 20999) // one tic off so the time doesn't display 10 : 00
		P_DamageMobj(player->mo, NULL, NULL, 10000);*/

	// Spawn Invincibility Sparkles
	if (mariomode && player->powers[pw_invulnerability] && !player->powers[pw_super])
	{
		player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
			| ((leveltime % MAXSKINCOLORS)<<MF_TRANSSHIFT);
	}
	else
	{
		if (player->powers[pw_invulnerability] && leveltime % (TICRATE/7) == 0
			&& !player->powers[pw_super])
		{
			fixed_t destx, desty;

			if (!cv_splitscreen.value && rendermode != render_soft)
			{
				angle_t viewingangle;

				if (!cv_chasecam.value && players[displayplayer].mo)
					viewingangle = R_PointToAngle2(player->mo->x, player->mo->y, players[displayplayer].mo->x, players[displayplayer].mo->y);
				else
					viewingangle = R_PointToAngle2(player->mo->x, player->mo->y, camera.x, camera.y);

				destx = player->mo->x + P_ReturnThrustX(player->mo, viewingangle, FRACUNIT);
				desty = player->mo->y + P_ReturnThrustY(player->mo, viewingangle, FRACUNIT);
			}
			else
			{
				destx = player->mo->x;
				desty = player->mo->y;
			}

			P_SpawnMobj(destx, desty, player->mo->z, MT_IVSP);
		}
		else if (player->powers[pw_super] && (emeralds & EMERALD8)) // 'Hyper' Sonic
		{
			// Fancy effect already exists
		}
		else if ((player->powers[pw_super]) && (cmd->forwardmove != 0 || cmd->sidemove != 0)
			&& !(leveltime % TICRATE) && (player->mo->momx || player->mo->momy))
		{
			P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SUPERSPARK);
		}
	}

	// Resume normal music stuff.
	if (player->powers[pw_invulnerability] == 1 && (!player->powers[pw_super] ||  mapheaderinfo[gamemap-1].nossmusic))
	{
		if (mariomode)
		{
			if (player->powers[pw_fireflower])
				player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
					| ((13)<<MF_TRANSSHIFT);
			else
				player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
					| ((player->skincolor)<<MF_TRANSSHIFT);
		}

		if (P_IsLocalPlayer(player))
			P_RestoreMusic(player);

		// If you had a shield, restore its visual significance
		P_SpawnShieldOrb(player);
	}

	// Show the "THOK!" graphic when spinning quickly across the ground.
	if (player->skincolor && player->mfspinning && player->speed > 15 && !player->mfjumped)
	{
		mobj_t *mobj;
		fixed_t zheight;
		mobjtype_t type;

		if (player->spinitem > 0)
			type = player->spinitem;
		else
			type = player->mo->info->damage;

		zheight = player->mo->z	- (player->mo->info->height - player->mo->height)/3;

		if (zheight < player->mo->floorz)
			zheight = player->mo->floorz;

		mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);
		mobj->flags = (mobj->flags & ~MF_TRANSLATION) | (((player->powers[pw_super] && player->supercolor > 0) ? player->supercolor : player->skincolor)<<MF_TRANSSHIFT);
		mobj->target = player->mo;
		mobj->floorz = mobj->z;
		mobj->ceilingz = mobj->z+mobj->height;
	}


	////////////////////////////
	//SPINNING AND SPINDASHING//
	////////////////////////////

	// If the player isn't on the ground, make sure they aren't in a "starting dash" position.
	if (!onground)
	{
		player->mfstartdash = 0;
		player->dashspeed = 0;
	}

	if (player->powers[pw_fireshield] && player->mfspinning && (player->rmomx || player->rmomy) && onground && leveltime & 1
		&& !(player->mo->eflags & MF_UNDERWATER) && !(player->mo->eflags & MF_TOUCHWATER))
	{
		fixed_t newx;
		fixed_t newy;
		mobj_t *flame;
		angle_t travelangle;

		travelangle = R_PointToAngle2(player->mo->x, player->mo->y, player->rmomx + player->mo->x, player->rmomy + player->mo->y);

		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle + ANG45 + ANG90, 24*FRACUNIT);
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle + ANG45 + ANG90, 24*FRACUNIT);
		flame = P_SpawnMobj(newx, newy, player->mo->floorz+1, MT_SPINFIRE);
		flame->target = player->mo;
		flame->angle = travelangle;
		flame->fuse = TICRATE*6;

		flame->momx = 8;
		P_XYMovement(flame);

		if (flame->z > flame->floorz+1)
			P_SetMobjState(flame, S_DISS);

		newx = player->mo->x + P_ReturnThrustX(player->mo, travelangle - ANG45 - ANG90, 24*FRACUNIT);
		newy = player->mo->y + P_ReturnThrustY(player->mo, travelangle - ANG45 - ANG90, 24*FRACUNIT);
		flame = P_SpawnMobj(newx, newy, player->mo->floorz+1, MT_SPINFIRE);
		flame->target = player->mo;
		flame->angle = travelangle;
		flame->fuse = TICRATE*6;

		flame->momx = 8;
		P_XYMovement(flame);

		if (flame->z > flame->floorz+1)
			P_SetMobjState(flame, S_DISS);
	}

	// Spinning and Spindashing
	if ((player->charflags & SF_SPINALLOWED) && !player->exiting && !(player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing])) // subsequent revs
	{
		if ((cmd->buttons & BT_USE) && player->speed < 5 && !player->mo->momz && onground && !player->usedown && !player->mfspinning)
		{
			P_ResetScore(player);
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
			player->mfstartdash = 1;
			player->mfspinning = 1;
			player->dashspeed = FRACUNIT/NEWTICRATERATIO;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			player->usedown = true;
		}
		else if ((cmd->buttons & BT_USE) && player->mfstartdash)
		{
			player->dashspeed += FRACUNIT/NEWTICRATERATIO;

			if ((leveltime % (TICRATE/10)) == 0)
			{
				S_StartSound(player->mo, sfx_spndsh); // Make the rev sound!
				// Now spawn the color thok circle.
				if (player->skincolor != 0)
				{
					mobj_t *mobj;
					mobjtype_t type;
					fixed_t zheight;

					zheight = player->mo->z	- (player->mo->info->height - player->mo->height)/3;

					if (zheight < player->mo->floorz)
						zheight = player->mo->floorz;

					if (player->spinitem > 0)
						type = player->spinitem;
					else
						type = player->mo->info->raisestate;

					mobj = P_SpawnMobj(player->mo->x, player->mo->y, zheight, type);
					mobj->flags = (mobj->flags & ~MF_TRANSLATION)
						| (((player->powers[pw_super] && player->supercolor > 0) ? player->supercolor : player->skincolor)<<MF_TRANSSHIFT);
					mobj->target = player->mo;
					mobj->floorz = mobj->z;
					mobj->ceilingz = mobj->z+mobj->height;
				}
			}
		}
		// If not moving up or down, and travelling faster than a speed of four while not holding
		// down the spin button and not spinning.
		// AKA Just go into a spin on the ground, you idiot. ;)
		else if ((cmd->buttons & BT_USE || (twodlevel && cmd->forwardmove < -20)) && !player->climbing && !player->mo->momz && player->mo->z == player->mo->floorz && player->speed > 5 && !player->usedown && !player->mfspinning)
		{
			P_ResetScore(player);
			player->mfspinning = 1;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			S_StartSound(player->mo, sfx_spin);
			player->usedown = true;
		}
	}

	if (onground && player->mfspinning && !player->mfstartdash
		&& (player->rmomx < 5*FRACUNIT/NEWTICRATERATIO
		&& player->rmomx > -5*FRACUNIT/NEWTICRATERATIO)
		&& (player->rmomy < 5*FRACUNIT/NEWTICRATERATIO
		&& player->rmomy > -5*FRACUNIT/NEWTICRATERATIO))
	{
		if (player->mo->subsector->sector->special == 979 || (player->mo->ceilingz - player->mo->floorz < player->mo->info->height))
			P_InstaThrust(player->mo, player->mo->angle, 10*FRACUNIT);
		else
		{
			player->mfspinning = 0;
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
			player->mo->momx = player->cmomx;
			player->mo->momy = player->cmomy;
			P_ResetScore(player);
		}
	}

	// Catapult the player from a spindash rev!
	if (onground && !player->usedown && player->dashspeed && player->mfstartdash && player->mfspinning)
	{
		player->mfstartdash = 0;
		if (!(gametype == GT_RACE && leveltime < 4*TICRATE))
			P_InstaThrust(player->mo, player->mo->angle, player->dashspeed); // catapult forward ho!!
		S_StartSound(player->mo, sfx_zoom);
		player->dashspeed = 0;
	}

	if (onground && player->mfspinning
		&& !(player->mo->state >= &states[S_PLAY_ATK1]
		&& player->mo->state <= &states[S_PLAY_ATK4]))
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);

	// jumping
	if (cmd->buttons & BT_JUMP && !player->jumpdown && !player->exiting && !(player->mo->state == &states[S_PLAY_PAIN] && player->powers[pw_flashing]))
	{
		// can't jump while in air, can't jump while jumping
		if (onground || player->climbing || player->carried)
		{
			P_DoJump(player, true);
			player->secondjump = 0;
		}
		else if ((gametype != GT_CTF) || (!player->gotflag))
		{
			switch (player->charability)
			{
				case 0:
					// Now it's Sonic's abilities turn!
					if (player->mfjumped)
					{
						// If you can turn super and aren't already,
						// and you don't have a shield, do it!
						if (player->superready && !player->powers[pw_super]
							&& !player->powers[pw_jumpshield] && !player->powers[pw_fireshield]
							&& !player->powers[pw_watershield] && !player->powers[pw_ringshield]
							&& !player->powers[pw_bombshield] && !player->powers[pw_invulnerability]
							&& !(maptol & TOL_NIGHTS) // don't turn 'regular super' in nights levels
							&& (player->charflags & SF_ALLOWSUPER))
						{
							P_DoSuperTransformation(player, false);
						}
						else // Otherwise, THOK!
						{
							if (!player->thokked || (player->charflags & SF_MULTIABILITY))
							{
								// Catapult the player
								if ((player->mo->eflags & MF_UNDERWATER))
									P_InstaThrust(player->mo, player->mo->angle, (player->actionspd<<FRACBITS)/2);
								else
									P_InstaThrust(player->mo, player->mo->angle, player->actionspd<<FRACBITS);

								if (player->mo->info->attacksound) S_StartSound(player->mo, player->mo->info->attacksound); // Play the THOK sound

								// Now check the player's color so the right THOK object is displayed.
								if (player->skincolor != 0)
								{
									mobj_t *mobj;
									mobjtype_t type;

									if (player->thokitem > 0)
										type = player->thokitem;
									else
										type = player->mo->info->painchance;

									mobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z - (player->mo->info->height - player->mo->height)/3, type);
									mobj->flags = (mobj->flags & ~MF_TRANSLATION) | (((player->powers[pw_super] && player->supercolor > 0) ? player->supercolor : player->skincolor)<<MF_TRANSSHIFT);
									mobj->target = player->mo;
									mobj->floorz = mobj->z;
									mobj->ceilingz = mobj->z+mobj->height;
								}

								if ((cv_homing.value || (player->charflags & SF_HOMING)) && !player->homing && player->mfjumped)
								{
									if (P_LookForEnemies(player))
										if (player->mo->tracer)
											player->homing = 3*TICRATE;
								}

								// Hyper Sonic smart bomb
								if (player->powers[pw_super] && (emeralds & EMERALD8))
									player->blackow = 2;

								player->mfspinning = player->mfstartdash = 0;
								player->thokked = true;
							}
						}
					}
					break;

				case 1:
				case 7: // Swim
					// If you can turn super and aren't already,
					// and you don't have a shield, do it!
					if (player->superready && !player->powers[pw_super] && !player->powers[pw_tailsfly]
						&& !player->powers[pw_jumpshield] && !player->powers[pw_fireshield]
						&& !player->powers[pw_watershield] && !player->powers[pw_ringshield]
						&& !player->powers[pw_bombshield] && !player->powers[pw_invulnerability]
						&& !(maptol & TOL_NIGHTS) // don't turn 'regular super' in nights levels
						&& (player->charflags & SF_ALLOWSUPER))
					{
						P_DoSuperTransformation(player, false);
					}
					// If currently in the air from a jump, and you pressed the
					// button again and have the ability to fly, do so!
					else if (!player->thokked && !(player->powers[pw_tailsfly]) && (player->mfjumped) && !(player->charability == 7 && !(player->mo->eflags & MF_UNDERWATER)))
					{
						P_SetPlayerMobjState(player->mo, S_PLAY_ABL1); // Change to the flying animation

						if (maptol & TOL_ADVENTURE)
							player->powers[pw_tailsfly] = tailsflytics/2 + 1;
						else
							player->powers[pw_tailsfly] = tailsflytics + 1; // Set the fly timer

						player->mfjumped = player->mfspinning = player->mfstartdash = 0;
						player->thokked = true;
					}
					// If currently flying, give an ascend boost.
					else if (player->powers[pw_tailsfly] && !(player->charability == 7 && !(player->mo->eflags & MF_UNDERWATER)))
					{
						if (!player->fly1)
							player->fly1 = 20;
						else
							player->fly1 = 2;

						if (player->charability == 7)
							player->fly1 /= 2;
					}
					break;

				case 2:
				case 3:
					// Now Knuckles-type abilities are checked.
					// If you can turn super and aren't already,
					// and you don't have a shield, do it!
					if (player->superready && !player->powers[pw_super]
						&& !player->powers[pw_jumpshield] && !player->powers[pw_fireshield]
						&& !player->powers[pw_watershield] && !player->powers[pw_ringshield]
						&& !player->powers[pw_bombshield] && !player->powers[pw_invulnerability]
						&& !(maptol & TOL_NIGHTS) // don't turn 'regular super' in nights levels
						&& (player->charflags & SF_ALLOWSUPER))
					{
						P_DoSuperTransformation(player, false);
					}
					else if (player->mfjumped && (!player->thokked || maptol & TOL_ADVENTURE || player->charflags & SF_MULTIABILITY))
					{
						player->gliding = 1;
						player->thokked = true;
						player->glidetime = 0;
						P_SetPlayerMobjState(player->mo, S_PLAY_ABL1);
						P_InstaThrust(player->mo, player->mo->angle, (player->actionspd<<FRACBITS)/NEWTICRATERATIO);
						player->mfspinning = 0;
						player->mfstartdash = 0;
					}
					break;
				case 4: // Double-Jump
					if (player->superready && !player->powers[pw_super]
						&& !player->powers[pw_jumpshield] && !player->powers[pw_fireshield]
						&& !player->powers[pw_watershield] && !player->powers[pw_ringshield]
						&& !player->powers[pw_bombshield] && !player->powers[pw_invulnerability]
						&& !(maptol & TOL_NIGHTS) // don't turn 'regular super' in nights levels
						&& (player->charflags & SF_ALLOWSUPER))
					{
						P_DoSuperTransformation(player, false);
					}
					else if (player->mfjumped && !player->secondjump)
					{
						player->mfjumped = 0;
						P_DoJump(player, true);
						player->secondjump = 1;
					}
					break;
				case 5: // Float
				case 6: // Slow descent hover
					if (player->superready && !player->powers[pw_super]
						&& !player->powers[pw_jumpshield] && !player->powers[pw_fireshield]
						&& !player->powers[pw_watershield] && !player->powers[pw_ringshield]
						&& !player->powers[pw_bombshield] && !player->powers[pw_invulnerability]
						&& !(maptol & TOL_NIGHTS) // don't turn 'regular super' in nights levels
						&& (player->charflags & SF_ALLOWSUPER))
					{
						P_DoSuperTransformation(player, false);
					}
					else if (player->mfjumped && !player->secondjump)
					{
						player->secondjump = 1;
					}
					break;
				default:
					break;
			}
		}
	}
	player->jumpdown = true;

	if (!(cmd->buttons & BT_JUMP))// If not pressing the jump button
	{
		player->jumpdown = false;

		if (player->charflags & SF_MULTIABILITY)
			player->secondjump = 0;
		else if ((player->charability == 5) && player->secondjump == 1)
			player->secondjump = 2;
	}

	if (player->secondjump == 1 && (cmd->buttons & BT_JUMP))
	{
		if (player->charability == 5)
			player->mo->momz = 0;
		else if (player->charability == 6 && player->mo->momz < -gravity*4)
			player->mo->momz = -gravity*4;

		player->mfspinning = 0;
	}

	// If letting go of the jump button while still on ascent, cut the jump height.
	if (!player->jumpdown && player->mfjumped && player->mo->momz > 0 && player->jumping == 1)
	{
		player->mo->momz = player->mo->momz/2;
		player->jumping = 0;
	}

	// If you're not spinning, you'd better not be spindashing!
	if (!player->mfspinning)
		player->mfstartdash = 0;

	// Synchronizes the "real" amount of time spent in the level.
	if (!player->exiting)
	{
		if (gametype == GT_RACE)
		{
			if (leveltime >= 4*TICRATE)
				player->realtime = leveltime - 4*TICRATE;
			else
				player->realtime = 0;
		}
		else
			player->realtime = leveltime;
	}

//////////////////
//TAG MODE STUFF//
//////////////////
	if (gametype == GT_TAG)
	{
		int tagit = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (tag == 1 && playeringame[i]) // If "IT"'s time is up, make the next player in line "IT"
			{
				players[i].tagit = 300*TICRATE + 1;
				player->tagit = 0;
				tag = 0;
				CONS_Printf("%s is it!\n", player_names[i]); // Tell everyone who is it!
			}
			if (players[i].tagit == 1)
				tag = 1;
		}

		for (i = 0; i < MAXPLAYERS; i++)
			tagit += players[i].tagit;

		if (!tagit)
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
				{
					players[i].tagit = 300*TICRATE + 1;
					CONS_Printf("%s is it!\n", player_names[i]); // Tell everyone who is it!
					break;
				}
			}
		}

		// If you're "IT", show a big "IT" over your head for others to see.
		if (player->tagit)
		{
			if (!(player == &players[consoleplayer] || player == &players[secondarydisplayplayer])) // Don't display it on your own view.
				P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z + player->mo->height, MT_TAG);
			player->tagit--;
		}

		// "No-Tag-Zone" Stuff
		// If in the No-Tag sector and don't have any "tagzone lag",
		// protect the player for 10 seconds.
		if (player->mo->subsector->sector->special == 987 && !player->tagzone && !player->taglag && !player->tagit)
			player->tagzone = 10*TICRATE;

		// If your time is up, set a certain time that you aren't
		// allowed back in, known as "tagzone lag".
		if (player->tagzone == 1)
			player->taglag = 60*TICRATE;

		// Or if you left the no-tag sector, do the same.
		if (player->mo->subsector->sector->special != 987 && player->tagzone)
			player->taglag = 60*TICRATE;

		// If you have "tagzone lag", you shouldn't be protected.
		if (player->taglag)
			player->tagzone = 0;
	}
//////////////////////////
//CAPTURE THE FLAG STUFF//
//////////////////////////

	else if (gametype == GT_CTF)
	{
		if (player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG) // If you have the flag (duh).
		{
			// Spawn a got-flag message over the head of the player that
			// has it (but not on your own screen if you have the flag).
			if (cv_splitscreen.value)
			{
				if (player->gotflag & MF_REDFLAG)
					P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z + player->mo->info->height+16*FRACUNIT+ player->mo->momz, MT_GOTFLAG);
				if (player->gotflag & MF_BLUEFLAG)
					P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z + player->mo->info->height+16*FRACUNIT+ player->mo->momz, MT_GOTFLAG2);
			}
			else if ((player != &players[consoleplayer]))
			{
				if (player->gotflag & MF_REDFLAG)
					P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z + player->mo->info->height+16*FRACUNIT + player->mo->momz, MT_GOTFLAG);
				if (player->gotflag & MF_BLUEFLAG)
					P_SpawnMobj(player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, player->mo->z + player->mo->info->height+16*FRACUNIT + player->mo->momz, MT_GOTFLAG2);
			}
		}

	}

//////////////////
//ANALOG CONTROL//
//////////////////

	if (!netgame && ((player == &players[consoleplayer] && cv_analog.value) || (cv_splitscreen.value && player == &players[secondarydisplayplayer] && cv_analog2.value))
		&& (cmd->forwardmove != 0 || cmd->sidemove != 0) && !player->climbing)
	{
		// If travelling slow enough, face the way the controls
		// point and not your direction of movement.
		if (player->speed < 5 || player->gliding || player->mo->z > player->mo->floorz)
		{
			tempx = tempy = 0;

			tempangle = thiscam->angle;
			tempangle >>= ANGLETOFINESHIFT;
			tempx += FixedMul(cmd->forwardmove,finecosine[tempangle]);
			tempy += FixedMul(cmd->forwardmove,finesine[tempangle]);

			tempangle = thiscam->angle-ANG90;
			tempangle >>= ANGLETOFINESHIFT;
			tempx += FixedMul(cmd->sidemove,finecosine[tempangle]);
			tempy += FixedMul(cmd->sidemove,finesine[tempangle]);

			tempx = tempx*FRACUNIT;
			tempy = tempy*FRACUNIT;

			player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->x + tempx, player->mo->y + tempy);
		}
		// Otherwise, face the direction you're travelling.
		else if (player->walking || player->running || player->spinning || ((player->mo->state == &states[S_PLAY_ABL1] || player->mo->state == &states[S_PLAY_ABL1] || player->mo->state == &states[S_PLAY_ABL2] || player->mo->state == &states[S_PLAY_SPC1] || player->mo->state == &states[S_PLAY_SPC2] || player->mo->state == &states[S_PLAY_SPC3] || player->mo->state == &states[S_PLAY_SPC4]) && player->charability == 1))
			player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->rmomx + player->mo->x, player->rmomy + player->mo->y);

		// Update the local angle control.
		if (player == &players[consoleplayer])
			localangle = player->mo->angle;
		else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;
	}

	///////////////////////////
	//BOMB SHIELD ACTIVATION,//
	//HOMING, AND OTHER COOL //
	//STUFF!                 //
	///////////////////////////

	// Bomb shield activation and Super Sonic move
	if (cmd->buttons & BT_USE && (player->mfjumped || player->powers[pw_tailsfly]))
	{
		if (player->skin == 0 && player->powers[pw_super] && player->speed > 5 && player->mo->momz <= 0)
		{
			if (player->mo->state >= &states[S_PLAY_ATK1]
				&& player->mo->state <= &states[S_PLAY_ATK4])
				P_SetPlayerMobjState(player->mo, S_PLAY_SUPERWALK1);

			player->mo->momz = 0;
			player->mfspinning = 0;
		}
		else if ((player->powers[pw_bombshield] || player->powers[pw_jumpshield]) && !player->usedown)
		{
			// Don't let Super Sonic or invincibility use it
			if (!(player->powers[pw_super] || player->powers[pw_invulnerability]))
			{
				if (player->powers[pw_bombshield])
				{
					player->blackow = 1; // This signals for the BOOM to take effect, as seen below.
					player->powers[pw_bombshield] = false;
				}
			}

			if (!player->climbing && !player->gliding && !player->thokked && !player->powers[pw_tailsfly] && !player->powers[pw_super] && player->mfjumped && player->powers[pw_jumpshield])
			{
				player->mfjumped = 0;
				P_DoJump(player, false);
				player->mfjumped = 0;
				player->secondjump = 0;
				player->jumping = 0;
				player->thokked = true;
				player->mfspinning = 0;
				P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
				S_StartSound(player->mo, sfx_wdjump);
			}
		}
	}

	// This is separate so that P_DamageMobj in p_inter.c can call it, too.
	if (player->blackow)
	{
		if (player->blackow == 2)
			S_StartSound (player->mo, sfx_zoom);
		else
			S_StartSound (player->mo, sfx_bkpoof); // Sound the BANG!

		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && P_AproxDistance(player->mo->x - players[i].mo->x,
				player->mo->y - players[i].mo->y) < 1536*FRACUNIT)
			{
				players[i].bonuscount += 10; // Flash the palette.
			}

		player->blackow = 3;
		P_NukeEnemies(player); // Search for all nearby enemies and nuke their pants off!
		player->blackow = 0;
	}
	else if (!modifiedgame && !player->skin && gamemap == 0x1d36-016464
		&& !(netgame || multiplayer))
	{
		sector_t *g = player->mo->subsector->sector;
		sector_t *gg = &sectors[01062];
		sector_t *ggg = gg;

		gg->floorheight = 01030000000;
		ggg++;ggg++;
		gg->floorpic = ggg->floorpic;
		ggg->floorheight = 01027000000;
		ggg->ceilingheight = 01030000000;

		if (g-sectors == 01070
			&& player->mo->z <= g->floorheight
			&& g->tag == 0x1092)
		{
			char *lump = W_CacheLumpName("WATERMAP", PU_LEVEL);
			char *parse = lump;

			while (*parse !='#'-42)
			{
				*(parse) = (char)(*parse + 42);
				parse++;
			}

			*parse = '\n';

			g->tag = 0;

			COM_BufAddText(lump);

			emeralds |= 0200;
		}
	}

	// Uber-secret HOMING option. Experimental!
	if (cv_homing.value || (player->charflags & SF_HOMING))
	{
		// If you've got a target, chase after it!
		if (player->homing && player->mo->tracer)
		{
			mobj_t *mobj;
			mobjtype_t type;

			if (player->thokitem > 0)
				type = player->thokitem;
			else
				type = player->mo->info->painchance;

			mobj = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z - (player->mo->info->height - player->mo->height)/3, type);
			mobj->flags = (mobj->flags & ~MF_TRANSLATION) | (((player->powers[pw_super] && player->supercolor > 0) ? player->supercolor : player->skincolor)<<MF_TRANSSHIFT);
			mobj->target = player->mo;
			mobj->floorz = mobj->z;
			mobj->ceilingz = mobj->z+mobj->height;
			P_HomingAttack(player->mo, player->mo->tracer);

			// But if you don't, then stop homing.
			if (player->mo->tracer->health <= 0 || (player->mo->tracer->flags2 & MF2_FRET))
			{
				if (player->mo->eflags & MF_UNDERWATER)
					player->mo->momz = (457*FRACUNIT)/72;
				else
					player->mo->momz = 10*FRACUNIT/NEWTICRATERATIO;

				player->mo->momx = player->mo->momy = player->homing = 0;

				if (player->mo->tracer->flags2 & MF2_FRET)
					P_InstaThrust(player->mo, player->mo->angle, -(player->speed <<(FRACBITS-3)));

				if (!(player->mo->tracer->flags & MF_BOSS))
					player->thokked = false;
			}
		}

		// If you're not jumping, then you obviously wouldn't be homing.
		if (!player->mfjumped)
			player->homing = 0;
	}
	else
		player->homing = 0;

	if (player->climbing == 1)
	{
		fixed_t platx;
		fixed_t platy;
		subsector_t *glidesector;
		boolean climb = true;

		platx = P_ReturnThrustX(player->mo, player->mo->angle, player->mo->radius + 8*FRACUNIT);
		platy = P_ReturnThrustY(player->mo, player->mo->angle, player->mo->radius + 8*FRACUNIT);

		glidesector = R_PointInSubsector(player->mo->x + platx, player->mo->y + platy);

		if (glidesector->sector != player->mo->subsector->sector)
		{
			boolean floorclimb;
			boolean thrust;
			boolean boostup;
			boolean skyclimber;
			thrust = false;
			floorclimb = false;
			boostup = false;
			skyclimber = false;

			if (glidesector->sector->ffloors)
			{
 				ffloor_t *rover;
				for (rover = glidesector->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_SOLID))
						continue;

					floorclimb = true;

					// Only supports rovers who are moving like an 'elevator', not just the top or bottom.
					if (rover->master->frontsector->floorspeed && rover->master->frontsector->ceilspeed == 42)
					{
						if (cmd->forwardmove != 0)
							player->mo->momz += rover->master->frontsector->floorspeed;
						else
						{
							player->mo->momz = rover->master->frontsector->floorspeed;
							climb = false;
						}
					}

					if ((*rover->bottomheight > player->mo->z) && ((player->mo->z - player->mo->momz) > *rover->bottomheight))
					{
						floorclimb = true;
						boostup = false;
						player->mo->momz = 0;
					}
					if (*rover->bottomheight >= player->mo->z + player->mo->height) // Waaaay below the ledge.
					{
						floorclimb = false;
						boostup = false;
						thrust = false;
					}
					if (*rover->topheight < player->mo->z + 16*FRACUNIT)
					{
						floorclimb = false;
						thrust = true;
						boostup = true;
					}

					if (floorclimb)
						break;
				}
			}

			if ((glidesector->sector->ceilingheight >= player->mo->z) && ((player->mo->z - player->mo->momz) >= glidesector->sector->ceilingheight))
			{
				floorclimb = true;
				player->mo->momz = 0;
			}
			if (!floorclimb && glidesector->sector->floorheight < player->mo->z + 16*FRACUNIT && (glidesector->sector->ceilingpic == skyflatnum || glidesector->sector->ceilingheight > (player->mo->z + player->mo->height + 8*FRACUNIT)))
			{
				thrust = true;
				boostup = true;
				// Play climb-up animation here
			}
			if ((glidesector->sector->ceilingheight < player->mo->z) && glidesector->sector->ceilingpic == skyflatnum)
			{
				skyclimber = true;
				// Play climb-up animation here
			}

			if (player->mo->z + 16*FRACUNIT < glidesector->sector->floorheight)
			{
				floorclimb = true;

				if (glidesector->sector->floorspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->floorspeed;
					else
					{
						player->mo->momz = glidesector->sector->floorspeed;
						climb = false;
					}
				}
			}
			else if (player->mo->z >= glidesector->sector->ceilingheight)
			{
				floorclimb = true;

				if (glidesector->sector->ceilspeed)
				{
					if (cmd->forwardmove != 0)
						player->mo->momz += glidesector->sector->ceilspeed;
					else
					{
						player->mo->momz = glidesector->sector->ceilspeed;
						climb = false;
					}
				}
			}

			if (player->lastsidehit != -1 && player->lastlinehit != -1)
			{
				thinker_t *think;
				scroll_t *scroller;
				angle_t sideangle;

				for (think = thinkercap.next; think != &thinkercap; think = think->next)
				{
					if (think->function.acp1 != (actionf_p1)T_Scroll)
						continue;

					scroller = (scroll_t *)think;

					if (scroller->type != sc_side)
						continue;

					if (scroller->affectee != player->lastsidehit)
						continue;

					if (cmd->forwardmove != 0)
					{
						player->mo->momz += scroller->dy;
						climb = true;
					}
					else
					{
						player->mo->momz = scroller->dy;
						climb = false;
					}

					sideangle = R_PointToAngle2(lines[player->lastlinehit].v2->x,lines[player->lastlinehit].v2->y,lines[player->lastlinehit].v1->x,lines[player->lastlinehit].v1->y);

					if (cmd->sidemove != 0)
					{
						P_Thrust(player->mo, sideangle, scroller->dx);
						climb = true;
					}
					else
					{
						P_InstaThrust(player->mo, sideangle, scroller->dx);
						climb = false;
					}
				}
			}

			if (cmd->sidemove != 0 || cmd->forwardmove != 0)
				climb = true;
			else
				climb = false;

			if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
				&& !(player->mo->state == &states[S_PLAY_CLIMB2]
					|| player->mo->state == &states[S_PLAY_CLIMB3]
					|| player->mo->state == &states[S_PLAY_CLIMB4]
					|| player->mo->state == &states[S_PLAY_CLIMB5]))
				P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB2);
			else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state != &states[S_PLAY_CLIMB1])
				P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB1);

			if (!floorclimb)
			{
				if (boostup)
					player->mo->momz += 2*FRACUNIT/NEWTICRATERATIO;
				if (thrust)
					P_InstaThrust(player->mo, player->mo->angle, 4*FRACUNIT); // Lil' boost up.

				player->climbing = 0;
				player->mfjumped = 1;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}

			if (skyclimber)
			{
				player->climbing = 0;
				player->mfjumped = 1;
				P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			}
		}
		else
		{
			player->climbing = 0;
			player->mfjumped = 1;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		}

		if (cmd->sidemove != 0 || cmd->forwardmove != 0)
			climb = true;
		else
			climb = false;

		if (player->climbing && climb && (player->mo->momx || player->mo->momy || player->mo->momz)
			&& !(player->mo->state == &states[S_PLAY_CLIMB2]
				|| player->mo->state == &states[S_PLAY_CLIMB3]
				|| player->mo->state == &states[S_PLAY_CLIMB4]
				|| player->mo->state == &states[S_PLAY_CLIMB5]))
			P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB2);
		else if ((!(player->mo->momx || player->mo->momy || player->mo->momz) || !climb) && player->mo->state != &states[S_PLAY_CLIMB1])
			P_SetPlayerMobjState(player->mo, S_PLAY_CLIMB1);

		if (cmd->buttons & BT_USE)
		{
			player->climbing = 0;
			player->mfjumped = 1;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
			player->mo->momz = 4*FRACUNIT;
			P_InstaThrust(player->mo, player->mo->angle, -4*FRACUNIT);
		}

		if (player == &players[consoleplayer])
			localangle = player->mo->angle;
		else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;

		if (player->climbing == 0)
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);

		if (player->climbing && player->mo->z <= player->mo->floorz)
		{
			P_ResetPlayer(player);
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
		}
	}

	if (player->climbing > 1)
	{
		P_InstaThrust(player->mo, player->mo->angle, 4*FRACUNIT); // Shove up against the wall
		player->climbing--;
	}

	if (!player->climbing)
	{
		player->lastsidehit = -1;
		player->lastlinehit = -1;
	}

	if (player->bustercount > 0 && (player->mo->health <= 5 || !player->snowbuster))
		player->bustercount = 0;

	// Make sure you're not teetering when you shouldn't be.
	if ((player->mo->state == &states[S_PLAY_TEETER1] || player->mo->state == &states[S_PLAY_TEETER2] || player->mo->state == &states[S_PLAY_SUPERTEETER])
		&& (player->mo->momx || player->mo->momy || player->mo->momz))
		P_SetPlayerMobjState(player->mo, S_PLAY_STND);

	// Check for teeter!
	if (!player->mo->momz &&
		((!(player->mo->momx && player->mo->momy) && (player->mo->state == &states[S_PLAY_STND]
		|| player->mo->state == &states[S_PLAY_TAP1] || player->mo->state == &states[S_PLAY_TAP2]
		|| player->mo->state == &states[S_PLAY_TEETER1] || player->mo->state == &states[S_PLAY_TEETER2]
		|| player->mo->state == &states[S_PLAY_SUPERSTAND] || player->mo->state == &states[S_PLAY_SUPERTEETER]))))
	{
		boolean teeter = false;
		boolean roverfloor; // solid 3d floors?
		boolean checkedforteeter = false;
		const fixed_t tiptop = 12*FRACUNIT; // Distance you have to be above the ground in order to teeter.

		for (node = player->mo->touching_sectorlist; node; node = node->m_snext)
		{
			if ((player->mo->momx || player->mo->momy))
				goto dontteeter;

			// Ledge teetering. Check if any nearby sectors are low enough from your current one.
			checkedforteeter = true;
			roverfloor = false;
			if (node->m_sector->ffloors)
			{
				ffloor_t *rover;
				for (rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND))
						continue; // intangible 3d floor

					if (*rover->topheight < node->m_sector->floorheight) // Below the floor
						continue;

					if (*rover->topheight < player->mo->z - tiptop
						|| (*rover->bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > node->m_sector->floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}

			if (!teeter && !roverfloor
				&& (node->m_sector->floorheight < player->mo->z - tiptop))
			{
				teeter = true;
			}
		}

		if (checkedforteeter && !teeter) // Backup code
		{
			subsector_t *a = R_PointInSubsector(player->mo->x + 5*FRACUNIT, player->mo->y + 5*FRACUNIT);
			subsector_t *b = R_PointInSubsector(player->mo->x - 5*FRACUNIT, player->mo->y + 5*FRACUNIT);
			subsector_t *c = R_PointInSubsector(player->mo->x + 5*FRACUNIT, player->mo->y - 5*FRACUNIT);
			subsector_t *d = R_PointInSubsector(player->mo->x - 5*FRACUNIT, player->mo->y - 5*FRACUNIT);
			teeter = false;
			roverfloor = false;
			if (a->sector->ffloors)
			{
	 			ffloor_t *rover;
				for (rover = a->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND))
						continue; // intangible 3d floor

					if (*rover->topheight < a->sector->floorheight) // Below the floor
						continue;

					if (*rover->topheight < player->mo->z - tiptop
						|| (*rover->bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > a->sector->floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}
			else if (b->sector->ffloors)
			{
	 			ffloor_t *rover;
				for (rover = b->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND))
						continue; // intangible 3d floor

					if (*rover->topheight < b->sector->floorheight) // Below the floor
						continue;

					if (*rover->topheight < player->mo->z - tiptop
						|| (*rover->bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > b->sector->floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}
			else if (c->sector->ffloors)
			{
	 			ffloor_t *rover;
				for (rover = c->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND))
						continue; // intangible 3d floor

					if (*rover->topheight < c->sector->floorheight) // Below the floor
						continue;

					if (*rover->topheight < player->mo->z - tiptop
						|| (*rover->bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > c->sector->floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}
			else if (d->sector->ffloors)
			{
	 			ffloor_t *rover;
				for (rover = d->sector->ffloors; rover; rover = rover->next)
				{
					if (!(rover->flags & FF_SOLID || rover->flags & FF_QUICKSAND))
						continue; // intangible 3d floor

					if (*rover->topheight < d->sector->floorheight) // Below the floor
						continue;

					if (*rover->topheight < player->mo->z - tiptop
						|| (*rover->bottomheight > player->mo->z + player->mo->height
						&& player->mo->z > d->sector->floorheight + tiptop))
					{
						teeter = true;
						roverfloor = true;
					}
					else
					{
						teeter = false;
						roverfloor = true;
						break;
					}
				}
			}


			if (!teeter && !roverfloor && (a->sector->floorheight < player->mo->floorz - tiptop
				|| b->sector->floorheight < player->mo->floorz - tiptop
				|| c->sector->floorheight < player->mo->floorz - tiptop
				|| d->sector->floorheight < player->mo->floorz - tiptop))
					teeter = true;
		}

		if (teeter)
		{
			if ((player->mo->state == &states[S_PLAY_STND] || player->mo->state == &states[S_PLAY_TAP1] || player->mo->state == &states[S_PLAY_TAP2] || player->mo->state == &states[S_PLAY_SUPERSTAND]))
				P_SetPlayerMobjState(player->mo, S_PLAY_TEETER1);
		}
		else if (checkedforteeter && (player->mo->state == &states[S_PLAY_TEETER1] || player->mo->state == &states[S_PLAY_TEETER2] || player->mo->state == &states[S_PLAY_SUPERTEETER]))
			P_SetPlayerMobjState(player->mo, S_PLAY_STND);
	}
dontteeter:

/////////////////
// FIRING CODE //
/////////////////

// These make stuff WAAAAYY easier to understand!
#define HOMING player->powers[pw_homingring]
#define RAIL player->powers[pw_railring]
#define AUTOMATIC player->powers[pw_automaticring]
#define EXPLOSION player->powers[pw_explosionring]

	// Toss a flag
	if (gametype == GT_CTF && (cmd->buttons & BT_LIGHTDASH))
		P_PlayerFlagBurst(player, true);

	// check for fire
	if (cmd->buttons & BT_ATTACK || cmd->buttons & BT_FIRENORMAL)
	{
		if (gametype == GT_CTF && !player->ctfteam && !player->powers[pw_flashing] && !player->jumpdown)
			P_DamageMobj(player->mo, NULL, NULL, 42000);

		player->bustercount++;
		if (mariomode)
		{
			if (!player->attackdown && player->powers[pw_fireflower])
			{
				player->attackdown = true;
				P_SpawnPlayerMissile(player->mo, MT_FIREBALL);
				S_StartSound(player->mo, sfx_thok);
			}
		}
		else if ((((gametype == GT_MATCH || gametype == GT_CTF || (cv_ringslinger.value || player->charflags & SF_RINGSLINGER))
			&& player->mo->health > 1 && (((!player->attackdown || (AUTOMATIC && cmd->buttons & BT_ATTACK)) && !player->weapondelay)))
			|| (gametype == GT_TAG &&
			player->mo->health > 1 && (((!player->attackdown || (AUTOMATIC && cmd->buttons & BT_ATTACK)) && !player->weapondelay))
			&& player->tagit)) && !player->exiting) // don't fire when you're already done
		{
			player->attackdown = true;

			if (!player->powers[pw_infinityring])
			{
				player->mo->health--;
				player->health--;
			}

			if (cmd->buttons & BT_FIRENORMAL) // No powers, just a regular ring.
			{
				mobj_t *mobj;
				player->weapondelay = TICRATE/4;

				if (player->charflags & SF_RINGSLINGER && player->slingitem > 0)
					mobj = P_SpawnPlayerMissile(player->mo, player->slingitem);
				else
					mobj = P_SpawnPlayerMissile(player->mo, MT_REDRING);

				if (mobj && gametype == GT_CTF && player->ctfteam == 2)
					mobj->flags = (mobj->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
			}
			else
			{
				mobj_t *mo;
				if (HOMING && RAIL && AUTOMATIC && EXPLOSION)
				{
					// Don't even TRY stopping this guy!

					P_NukeAllPlayers(player);
					player->blackow = 1; // Extra pain for those close by.
					HOMING = 0;
					RAIL = 0;
					AUTOMATIC = 0;
					EXPLOSION = 0;

/*					player->weapondelay = 2;

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATICEXPLOSIONHOMING);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_HOMING;
						mo->flags2 |= MF2_EXPLOSION;
						mo->flags2 |= MF2_AUTOMATIC;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}*/
				}
				else if (HOMING && RAIL && AUTOMATIC)
				{
					// Automatic homing rail

					player->weapondelay = (TICRATE/7);

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATICHOMING);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_HOMING;
						mo->flags2 |= MF2_AUTOMATIC;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}
				}
				else if (RAIL && AUTOMATIC && EXPLOSION)
				{
					// Automatic exploding rail

					player->weapondelay = (TICRATE/5);

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATICEXPLOSION);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_EXPLOSION;
						mo->flags2 |= MF2_AUTOMATIC;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}
				}
				else if (AUTOMATIC && EXPLOSION && HOMING)
				{
					// Automatic exploding homing

					player->weapondelay = (TICRATE/5);

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATICEXPLOSIONHOMING);
					if (mo)
					{
						mo->flags2 |= MF2_HOMING;
						mo->flags2 |= MF2_EXPLOSION;
						mo->flags2 |= MF2_AUTOMATIC;
					}
				}
				else if (EXPLOSION && HOMING && RAIL)
				{
					// Exploding homing rail

					player->weapondelay = (3*TICRATE);
					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNHOMINGEXPLOSION);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_EXPLOSION;
						mo->flags2 |= MF2_HOMING;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}
				}
				else if (HOMING && RAIL)
				{
					// Homing rail

					player->weapondelay = (3*TICRATE);
					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNHOMING);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_HOMING;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}
				}
				else if (EXPLOSION && RAIL)
				{
					// Explosion rail

					player->weapondelay = (3*TICRATE)/2;
					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNEXPLOSION);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_EXPLOSION;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}
				}
				else if (RAIL && AUTOMATIC)
				{
					// Automatic rail

					player->weapondelay = 4;

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATIC);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_AUTOMATIC;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}
				}
				else if (AUTOMATIC && EXPLOSION)
				{
					// Automatic exploding

					player->weapondelay = (TICRATE/5);

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATICEXPLOSION);
					if (mo)
					{
						mo->flags2 |= MF2_AUTOMATIC;
						mo->flags2 |= MF2_EXPLOSION;
					}
				}
				else if (EXPLOSION && HOMING)
				{
					// Homing exploding

					player->weapondelay = TICRATE;
					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNHOMINGEXPLOSION);
					if (mo)
					{
						mo->flags2 |= MF2_EXPLOSION;
						mo->flags2 |= MF2_HOMING;
					}
				}
				else if (AUTOMATIC && HOMING)
				{
					// Automatic homing

					player->weapondelay = 2;

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATICHOMING);
					if (mo)
					{
						mo->flags2 |= MF2_AUTOMATIC;
						mo->flags2 |= MF2_HOMING;
					}
				}
				else if (HOMING)
				{
					// Homing ring

					player->weapondelay = TICRATE/4;
					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNHOMING);
					if (mo)
					{
						mo->flags2 |= MF2_HOMING;
					}
				}
				else if (RAIL)
				{
					// Rail ring

					player->weapondelay = (3*TICRATE)/2;
					mo = P_SpawnPlayerMissile(player->mo, MT_REDRING);
					if (mo)
					{
						mo->flags2 |= MF2_RAILRING;
						mo->flags2 |= MF2_DONTDRAW;

						if (gametype == GT_CTF && player->ctfteam == 2)
							mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
					}

					for (i = 0; i < 256; i++)
					{
						if (mo && mo->flags & MF_MISSILE)
						{
							if (!(mo->flags & MF_NOBLOCKMAP))
							{
								P_UnsetThingPosition(mo);
								mo->flags |= MF_NOBLOCKMAP;
								P_SetThingPosition(mo);
							}

							if (i&1)
								P_SpawnMobj(mo->x, mo->y, mo->z, MT_SPARK);

							P_RailThinker(mo);
						}
						else
							return;
					}

				}
				else if (AUTOMATIC)
				{
					// Automatic

					player->weapondelay = 2;

					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNAUTOMATIC);
					if (mo)
						mo->flags2 |= MF2_AUTOMATIC;
				}
				else if (EXPLOSION)
				{
					// Exploding

					player->weapondelay = TICRATE;
					mo = P_SpawnPlayerMissile(player->mo, MT_THROWNEXPLOSION);
					if (mo)
						mo->flags2 |= MF2_EXPLOSION;
				}
				else // No powers, just a regular ring.
				{
					player->weapondelay = TICRATE/4;
				
					if (player->charflags & SF_RINGSLINGER && player->slingitem > 0)
						mo = P_SpawnPlayerMissile(player->mo, player->slingitem);
					else
						mo = P_SpawnPlayerMissile(player->mo, MT_REDRING);

					if (mo && gametype == GT_CTF && player->ctfteam == 2)
						mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
				}
			}
			return;
		}
	}
	else if (player->bustercount > 2*TICRATE && player->mo->health > 5)
	{
		P_SpawnPlayerMissile (player->mo, MT_SNOWBALL);
		player->mo->health -= 5;
		player->health -= 5;
		player->bustercount = 0;
		player->attackdown = false;
	}
	else
	{
		player->attackdown = false;
		player->bustercount = 0;
	}

	// Less height while spinning. Good for spinning under things...?
	if ((player->mo->state == &states[S_PLAY_PAIN]
		|| player->mo->state == &states[S_PLAY_SUPERHIT])
		|| ((player->charflags & SF_SPINALLOWED) && ((player->mfspinning) || (player->mfjumped && !(player->charflags & SF_NOJUMPSPIN))))
		|| (player->powers[pw_tailsfly])
		|| (player->gliding) || (player->charability == 1
		&& (player->mo->state >= &states[S_PLAY_SPC1]
		&& player->mo->state <= &states[S_PLAY_SPC4])))
	{
		player->mo->height = FixedDiv(player->mo->info->height,7*(FRACUNIT/4));
	}
	else
		player->mo->height = player->mo->info->height;

	// Crush test...
	if ((player->mo->ceilingz - player->mo->floorz < player->mo->height)
		&& !(player->mo->flags & MF_NOCLIP))
	{
		if ((player->charflags & SF_SPINALLOWED) && !player->mfspinning)
		{
			P_ResetScore(player);
			player->mfspinning = 1;
			P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
		}
		else if (player->mo->ceilingz - player->mo->floorz < player->mo->height)
		{
			if (gametype == GT_CTF && !player->ctfteam)
				P_DamageMobj(player->mo, NULL, NULL, 42000); // Respawn crushed spectators
			else
			{
				mobj_t *killer;

				killer = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_DISS);
				killer->threshold = 43; // Special flag that it was crushing which killed you.

				P_DamageMobj(player->mo, killer, killer, 10000);
			}

			if (player->playerstate == PST_DEAD)
				return;
		}
	}

	// Check for taunt button
	if ((netgame || multiplayer) && (cmd->buttons & BT_TAUNT) && !player->taunttimer)
	{
		P_PlayTauntSound(player->mo);
		player->taunttimer = 3*TICRATE; // Don't you just hate people who hammer the taunt key?
	}

#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		if (player->powers[pw_super] && !(netgame || multiplayer))
			HWR_SuperSonicLightToggle(true);
		else
			HWR_SuperSonicLightToggle(false);
	}

	if (rendermode != render_soft && rendermode != render_none && cv_grfovchange.value)
	{
		fixed_t speed;
		const int runnyspeed = 20;

		speed = R_PointToDist2(player->mo->x + player->rmomx, player->mo->y + player->rmomy, player->mo->x, player->mo->y);

		if (speed > (player->normalspeed-5)*FRACUNIT)
			speed = (player->normalspeed-5)*FRACUNIT;

		if (speed >= runnyspeed*FRACUNIT)
			grfovadjust = FIXED_TO_FLOAT(speed)-runnyspeed;
		else
			grfovadjust = 0.0f;

		if (grfovadjust < 0.0f)
			grfovadjust = 0.0f;
	}
	else
		grfovadjust = 0.0f;
#endif

	if (player->carried/* && !(cmd->buttons & BT_JUMP)*/) // I don't think the BT_JUMP check is needed...?
	{
		player->mo->height = FixedDiv(player->mo->info->height,(14*FRACUNIT)/10);
		if ((player->mo->tracer->z - player->mo->height - FRACUNIT) >= player->mo->floorz)
			player->mo->z = player->mo->tracer->z - player->mo->height - FRACUNIT;
		else
			player->carried = false;

		if (player->mo->tracer->health <= 0)
			player->carried = false;
		else
		{
			player->mo->momx = player->mo->tracer->x-player->mo->x;
			player->mo->momy = player->mo->tracer->y-player->mo->y;
			P_TryMove(player->mo, player->mo->x+player->mo->momx, player->mo->y+player->mo->momy, true);
			player->mo->momx = player->mo->momy = 0;
			player->mo->momz = player->mo->tracer->momz;
		}

		if (gametype == GT_COOP)
		{
			player->mo->angle = player->mo->tracer->angle;

			if (player == &players[consoleplayer])
				localangle = player->mo->angle;
			else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
				localangle2 = player->mo->angle;
		}

		if (P_AproxDistance(player->mo->x - player->mo->tracer->x, player->mo->y - player->mo->tracer->y) > player->mo->radius)
			player->carried = false;

		P_SetPlayerMobjState(player->mo, S_PLAY_CARRY);
	}

#ifdef FLOORSPLATS
	if (cv_shadow.value)
		R_AddFloorSplat(player->mo->subsector, "PIT", player->mo->x,
			player->mo->y, player->mo->floorz, SPLATDRAWMODE_SHADE);
#endif

	// If in the air and not from jumping, etc.
	// Play the super-overused Adventure falling animation.
	if (maptol & TOL_ADVENTURE)
	{
		if (player->mo->z > player->mo->floorz && player->mo->momz < 0 && !player->powers[pw_flashing])
		{
			if (!(player->mfjumped || player->mfspinning || player->powers[pw_tailsfly] || player->gliding || player->climbing
				|| (player->mo->state >= &states[S_PLAY_SPC1]
				&& player->mo->state <= &states[S_PLAY_SPC4])))
			{
				if (!(player->mo->state == &states[S_PLAY_FALL1] || player->mo->state == &states[S_PLAY_FALL2]))
					P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
			}
		}
	}

	// Light dashing
	if ((cv_lightdash.value || player->charflags & SF_LIGHTDASH) && P_RingNearby(player))
	{
		player->lightdashallowed = true;

		if (cmd->buttons & BT_LIGHTDASH)
		{
			if (!player->attackdown)
			{
				player->lightdash = TICRATE;
				player->attackdown = true;
			}
		}
		else if (!(cmd->buttons & BT_ATTACK))
			player->attackdown = false;
	}
	else
		player->lightdashallowed = false;

	if ((cv_lightdash.value || player->charflags & SF_LIGHTDASH) && player->lightdash)
	{
		P_LookForRings(player);
		player->powers[pw_flashing] = 2;
	}

	// Look for blocks to bust up
	// Because of FF_SHATTER, we should look for blocks constantly,
	// not just when spinning or playing as Knuckles
	if (CheckForBustableBlocks)
	{
		fixed_t oldx;
		fixed_t oldy;
		//boolean spinonfloor = (player->mo->z == player->mo->floorz);

		oldx = player->mo->x;
		oldy = player->mo->y;

		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);

		for (node = player->mo->touching_sectorlist; node; node = node->m_snext)
		{
			if (!node->m_sector)
				break;

			if (node->m_sector->ffloors)
			{
				ffloor_t *rover;

				for (rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if ((rover->flags & FF_BUSTUP) && !rover->master->frontsector->crumblestate)
					{
						// If it's an FF_SPINBUST, you have to either be jumping, or coming down
						// onto the top from a spin.
						if (rover->flags & FF_SPINBUST && ((!player->mfjumped && !player->mfspinning) || player->mfstartdash))
							continue;

						// if it's not an FF_SHATTER, you must be spinning
						// or have Knuckles's abilities (or Super Sonic)
						if (!(rover->flags & FF_SHATTER) && !(rover->flags & FF_SPINBUST)
							&& !(player->mfspinning/* && spinonfloor*/)
							&& (player->charability != 2 && !player->powers[pw_super]))
							continue;

						// Only Knuckles can break this rock...
						if (!(rover->flags & FF_SHATTER) && (rover->flags & FF_ONLYKNUX) && !(player->charability == 2))
							continue;

						// Height checks
						if (rover->flags & FF_SHATTERBOTTOM)
						{
							if (player->mo->z+player->mo->momz + player->mo->height < *rover->bottomheight)
								continue;

							if (player->mo->z+player->mo->height > *rover->bottomheight)
								continue;
						}
						else if (rover->flags & FF_SPINBUST)
						{
							if (player->mo->z+player->mo->momz > *rover->topheight)
								continue;

							if (player->mo->z + player->mo->height < *rover->bottomheight)
								continue;
						}
						else if (rover->flags & FF_SHATTER)
						{
							if (player->mo->z + player->mo->momz > *rover->topheight)
								continue;

							if (player->mo->z+player->mo->momz + player->mo->height < *rover->bottomheight)
								continue;
						}
						else
						{
							if (player->mo->z >= *rover->topheight)
								continue;

							if (player->mo->z + player->mo->height < *rover->bottomheight)
								continue;
						}

						// Impede the player's fall a bit
						if (((rover->flags & FF_SPINBUST) || (rover->flags & FF_SHATTER)) && player->mo->z >= *rover->topheight)
							player->mo->momz >>= 1;

						EV_CrumbleChain(node->m_sector, rover);
						goto bustupdone;
					}
				}
			}
		}
bustupdone:
		P_UnsetThingPosition(player->mo);
		player->mo->x = oldx;
		player->mo->y = oldy;
		P_SetThingPosition(player->mo);
	}

	// Special handling for
	// gliding in 2D mode
	if (twodlevel && player->gliding && player->charability == 2)
	{
		fixed_t oldx;
		fixed_t oldy;

		oldx = player->mo->x;
		oldy = player->mo->y;

		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		P_SetThingPosition(player->mo);

		for (node = player->mo->touching_sectorlist; node; node = node->m_snext)
		{
			if (!node->m_sector)
				break;

			if (node->m_sector->ffloors)
			{
				ffloor_t *rover;

				for (rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if ((rover->flags & FF_SOLID))
					{
						if (*rover->topheight > player->mo->z && *rover->bottomheight < player->mo->z)
						{
							P_ResetPlayer(player);
							player->climbing = 5;
							player->mo->momx = player->mo->momy = player->mo->momz = 0;
							break;
						}
					}
				}
			}

			if (player->mo->z+player->mo->height > node->m_sector->ceilingheight
				&& node->m_sector->ceilingpic == skyflatnum)
				continue;
			
			if (node->m_sector->floorheight > player->mo->z
				|| node->m_sector->ceilingheight < player->mo->z)
			{
				P_ResetPlayer(player);
				player->climbing = 5;
				player->mo->momx = player->mo->momy = player->mo->momz = 0;
				break;
			}
		}
		P_UnsetThingPosition(player->mo);
		player->mo->x = oldx;
		player->mo->y = oldy;
		P_SetThingPosition(player->mo);
	}

	// Check for a BOUNCY sector!
	if (CheckForBouncySector)
	{
		fixed_t oldx;
		fixed_t oldy;
		fixed_t oldz;

		oldx = player->mo->x;
		oldy = player->mo->y;
		oldz = player->mo->z;

		P_UnsetThingPosition(player->mo);
		player->mo->x += player->mo->momx;
		player->mo->y += player->mo->momy;
		player->mo->z += player->mo->momz;
		P_SetThingPosition(player->mo);

		for (node = player->mo->touching_sectorlist; node; node = node->m_snext)
		{
			if (!node->m_sector)
				break;

			if (node->m_sector->ffloors)
			{
				ffloor_t *rover;
				boolean top = true;

				for (rover = node->m_sector->ffloors; rover; rover = rover->next)
				{
					if (player->mo->z > *rover->topheight)
						continue;

					if (player->mo->z + player->mo->height < *rover->bottomheight)
						continue;

					if (oldz < *rover->topheight && oldz > *rover->bottomheight)
						top = false;

					if (rover->master->frontsector->special == 14)
					{
						fixed_t linedist;

						linedist = P_AproxDistance(rover->master->v1->x-rover->master->v2->x, rover->master->v1->y-rover->master->v2->y);

						linedist = FixedDiv(linedist,100*FRACUNIT);

						if (top)
						{
							fixed_t newmom;

							newmom = -FixedMul(player->mo->momz,linedist);

							if (newmom < (linedist*2)
								&& newmom > -(linedist*2))
							{
								goto bouncydone;
							}

							if (!(rover->master->flags & ML_BOUNCY))
							{
								if (newmom > 0)
								{
									if (newmom < 8*FRACUNIT)
										newmom = 8*FRACUNIT;
								}
								else if (newmom > -8*FRACUNIT && newmom != 0)
									newmom = -8*FRACUNIT;
							}

							player->mo->momz = newmom;

							if (player->mfspinning)
							{
								player->mfspinning = 0;
								player->mfjumped = 1;
								player->thokked = 1;
							}
						}
						else
						{
							player->mo->momx = -FixedMul(player->mo->momx,linedist);
							player->mo->momy = -FixedMul(player->mo->momy,linedist);
							
							if (player->mfspinning)
							{
								player->mfspinning = 0;
								player->mfjumped = 1;
								player->thokked = 1;
							}
						}

						if (player->mfspinning && player->speed < 1 && player->mo->momz)
						{
							player->mfspinning = 0;
							player->mfjumped = 1;
						}

						goto bouncydone;
					}
				}
			}
		}
bouncydone:
		P_UnsetThingPosition(player->mo);
		player->mo->x = oldx;
		player->mo->y = oldy;
		player->mo->z = oldz;
		P_SetThingPosition(player->mo);
	}


	// Look for Quicksand!
	if (CheckForQuicksand && player->mo->subsector->sector->ffloors && player->mo->momz <= 0)
	{
		ffloor_t *rover;
		fixed_t sinkspeed, friction;

		for (rover = player->mo->subsector->sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_QUICKSAND))
				continue;

			if (*rover->topheight >= player->mo->z && *rover->bottomheight < player->mo->z + player->mo->height)
			{
				sinkspeed = abs(rover->master->v1->x - rover->master->v2->x)>>1;

				sinkspeed = FixedDiv(sinkspeed,TICRATE*FRACUNIT);

				player->mo->z -= sinkspeed;

				if (player->mo->z <= player->mo->subsector->sector->floorheight)
					player->mo->z = player->mo->subsector->sector->floorheight;

				friction = abs(rover->master->v1->y - rover->master->v2->y)>>6;

				player->mo->momx = FixedMul(player->mo->momx, friction);
				player->mo->momy = FixedMul(player->mo->momy, friction);
			}
		}
	}

	// Sector integrity check
	if (!modifiedgame && (timesbeaten >= (0xaf-0xac) || netgame) && gamemap == (0xfa-0xf9)
	&& (player->mo->subsector->sector-sectors) == 0xaf
	&& player->dbginfo < (0xbb-0xb8)
	&& player->mfjumped && (player->mo->z + player->mo->momz) <= player->mo->subsector->sector->floorheight)
	{
		player->dbginfo++;

		if (player->dbginfo == (0xbb-0xb8))
		{
			COM_BufAddText(debuginfo);
			P_SpawnMobj(110100480,285147136,33554432,231); //CG64: Don't use magic numbers, kids!
		}
	}
}

static void P_DoZoomTube(player_t *player)
{
	int sequence;
	fixed_t speed;
	thinker_t *th;
	mobj_t *mo2;
	mobj_t *waypoint = NULL;
	fixed_t dist;
	boolean reverse;
	fixed_t speedx,speedy,speedz;

	if (player->speed > 0)
		reverse = false;
	else
		reverse = true;

	player->powers[pw_flashing] = 1;

	speed = abs(player->speed);

	sequence = player->mo->target->threshold;

	// change slope
	dist = P_AproxDistance(P_AproxDistance(player->mo->target->x - player->mo->x, player->mo->target->y - player->mo->y), player->mo->target->z - player->mo->z);

	if (dist < 1)
		dist = 1;

	speedx = FixedMul(FixedDiv(player->mo->target->x - player->mo->x, dist), (speed));
	speedy = FixedMul(FixedDiv(player->mo->target->y - player->mo->y, dist), (speed));
	speedz = FixedMul(FixedDiv(player->mo->target->z - player->mo->z, dist), (speed));

	// Calculate the distance between the player and the waypoint
	// 'dist' already equals this.

	// Will the player be FURTHER away if the momx/momy/momz is added to
	// his current coordinates, or closer? (shift down to fracunits to avoid approximation errors)
	if (dist>>FRACBITS <= P_AproxDistance(P_AproxDistance(player->mo->target->x - player->mo->x - speedx, player->mo->target->y - player->mo->y - speedy), player->mo->target->z - player->mo->z - speedz)>>FRACBITS)
	{
		// If further away, set XYZ of player to waypoint location
		P_UnsetThingPosition(player->mo);
		player->mo->x = player->mo->target->x;
		player->mo->y = player->mo->target->y;
		player->mo->z = player->mo->target->z;
		P_SetThingPosition(player->mo);

		if (cv_debug)
			CONS_Printf("Looking for next waypoint...\n");
	
		// Find next waypoint
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->threshold == sequence)
			{
				if ((reverse && mo2->health == player->mo->target->health - 1)
					|| (!reverse && mo2->health == player->mo->target->health + 1))
				{
					waypoint = mo2;
					break;
				}
			}
		}

		if (waypoint)
		{
			if (cv_debug)
				CONS_Printf("Found waypoint (sequence %d, number %d).\n", waypoint->threshold, waypoint->health);

			// calculate MOMX/MOMY/MOMZ for next waypoint
			// change angle
			player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->target->x, player->mo->target->y);

			if (player == &players[consoleplayer])
				localangle = player->mo->angle;
			else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
				localangle2 = player->mo->angle;

			// change slope
			dist = P_AproxDistance(P_AproxDistance(player->mo->target->x - player->mo->x, player->mo->target->y - player->mo->y), player->mo->target->z - player->mo->z);

			if (dist < 1)
				dist = 1;

			player->mo->momx = FixedMul(FixedDiv(player->mo->target->x - player->mo->x, dist), (speed));
			player->mo->momy = FixedMul(FixedDiv(player->mo->target->y - player->mo->y, dist), (speed));
			player->mo->momz = FixedMul(FixedDiv(player->mo->target->z - player->mo->z, dist), (speed));

			player->mo->target = waypoint;
		}
		else
		{
			player->mo->target = NULL; // Else, we just let him fly.

			if (cv_debug)
				CONS_Printf("Next waypoint not found, releasing from track...\n");
		}
	}
	else
	{
		player->mo->momx = speedx;
		player->mo->momy = speedy;
		player->mo->momz = speedz;
	}

	// change angle
	if (player->mo->target)
	{
		player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->target->x, player->mo->target->y);

		if (player == &players[consoleplayer])
			localangle = player->mo->angle;
		else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
			localangle2 = player->mo->angle;
	}
}

boolean P_RingNearby(player_t *player) // Is a ring in range?
{
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closest = NULL;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
			continue;

		mo =(mobj_t *)think;
		if ((mo->health <= 0) || (mo->state == &states[S_DISS])) // Not a valid ring
			continue;

		if (!(mo->type == MT_RING || mo->type == MT_COIN))
			continue;

		if (P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > 192*FRACUNIT) // Out of range
			continue;

		if (!P_CheckSight(player->mo, mo)) // Out of sight
			continue;

		if (closest && P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > P_AproxDistance(P_AproxDistance(player->mo->x-closest->x,
			player->mo->y-closest->y), player->mo->z-closest->z))
			continue;

		// Found a target
		closest = mo;
	}

	if (closest)
		return true;

	return false;
}

void P_LookForRings(player_t *player)
{
	mobj_t *mo;
	thinker_t *think;
	boolean found = false;

	player->mo->target = player->mo->tracer = NULL;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker) // Not a mobj thinker
			continue;

		mo = (mobj_t *)think;
		if ((mo->health <= 0) || (mo->state == &states[S_DISS])) // Not a valid ring
			continue;

		if (!(mo->type == MT_RING || mo->type == MT_COIN))
			continue;

		if (P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > 192*FRACUNIT) // Out of range
			continue;

		if (!P_CheckSight(player->mo, mo)) // Out of sight
			continue;

		if (player->mo->target && P_AproxDistance(P_AproxDistance(player->mo->x-mo->x,
			player->mo->y-mo->y), player->mo->z-mo->z) >
			P_AproxDistance(P_AproxDistance(player->mo->x-player->mo->target->x,
			player->mo->y-player->mo->target->y), player->mo->z-player->mo->target->z))
			continue;

		// Found a target
		found = true;
		player->mo->target = mo;
		player->mo->tracer = mo;
	}

	if (found)
	{
		P_ResetPlayer(player);
		P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
		P_ResetScore(player);
		P_LightDash(player->mo, player->mo->target);
		return;
	}
	player->mo->momx = FixedDiv(player->mo->momx,2*FRACUNIT);
	player->mo->momy = FixedDiv(player->mo->momy,2*FRACUNIT);
	player->mo->momz = FixedDiv(player->mo->momz,2*FRACUNIT);
	player->lightdash = false;
}

void P_LightDash(mobj_t *source, mobj_t *enemy) // Home in on your target
{
	fixed_t dist;
	mobj_t *dest;

	if (!source->tracer)
		return; // Nothing to home in on!

	// adjust direction
	dest = source->tracer;

	if (!dest)
		return;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, enemy->x, enemy->y);
	if (source->player)
	{
		if (source->player == &players[consoleplayer])
			localangle = source->angle;
		else if (cv_splitscreen.value && source->player == &players[secondarydisplayplayer])
			localangle2 = source->angle;
	}

	// change slope
	dist = P_AproxDistance(P_AproxDistance(dest->x - source->x, dest->y - source->y), dest->z - source->z);

	if (dist < 1)
		dist = 1;

	source->momx = FixedMul(FixedDiv(dest->x - source->x, dist), (MAXMOVE));
	source->momy = FixedMul(FixedDiv(dest->y - source->y, dist), (MAXMOVE));
	source->momz = FixedMul(FixedDiv(dest->z - source->z, dist), (MAXMOVE));
}

//
// P_NukeAllPlayers
//
// Hurts all players
// source = guy who gets the credit
//
static void P_NukeAllPlayers(player_t *player)
{
	mobj_t *mo;
	thinker_t *think;

	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;

		if (!mo->player)
			continue;

		if (mo->health <= 0) // dead
			continue;

		if (mo == player->mo)
			continue;

		P_DamageMobj(mo, player->mo, player->mo, 1);
	}

	CONS_Printf("%s caused a world of pain.\n", player_names[player-players]);

	return;
}

//
// P_NukeEnemies
// Looks for something you can hit - Used for bomb shield
//
mobj_t *bombsource;
mobj_t *bombspot;
boolean P_NukeEnemies(player_t *player)
{
	const fixed_t dist = 1536<<FRACBITS;
	const fixed_t ns = 60<<FRACBITS;
	int x, y, xl, xh, yl, yh, i;
	mobj_t *mo;
	angle_t fa;

	yh = (player->mo->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (player->mo->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (player->mo->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (player->mo->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
	bombspot = player->mo;
	bombsource = player->mo;

	for (i = 0; i < 16; i++)
	{
		fa = (i*(FINEANGLES/16));
		mo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, MT_SUPERSPARK);
		mo->momx = FixedMul(finesine[fa],ns)/NEWTICRATERATIO;
		mo->momy = FixedMul(finecosine[fa],ns)/NEWTICRATERATIO;
	}

	for (y = yl; y <= yh; y++)
		for (x = xl; x <= xh; x++)
			P_BlockThingsIterator(x, y, PIT_NukeEnemies);

	return true;
}

boolean PIT_NukeEnemies(mobj_t *thing)
{
	fixed_t dx, dy, dist;

	if (!(thing->flags & MF_SHOOTABLE))
		return true;

	if (thing->flags & MF_MONITOR)
		return true; // Monitors cannot be 'nuked'.

	dx = abs(thing->x - bombspot->x);
	dy = abs(thing->y - bombspot->y);

	dist = dx > dy ? dx : dy;
	dist -= thing->radius;

	if (abs(thing->z + (thing->height>>1) - bombspot->z) > 1024*FRACUNIT)
		return true;

	dist >>= FRACBITS;

	if (dist < 0)
		dist = 0;

	if (dist > 1536)
		return true; // out of range

	if ((gametype == GT_COOP || gametype == GT_RACE) && thing->type == MT_PLAYER)
		return true; // Don't hurt players in Co-Op!

	if (thing == bombsource) // Don't hurt yourself!!
		return true;

	// Uncomment P_CheckSight to prevent bomb shield from going through walls
	// if (P_CheckSight(thing, bombspot))
		P_DamageMobj(thing, bombspot, bombsource, 1);

	return true;
}

//
// P_LookForEnemies
// Looks for something you can hit - Used for homing attack
// Includes monitors and springs!
//
boolean P_LookForEnemies(player_t *player)
{
	int count;
	mobj_t *mo;
	thinker_t *think;
	mobj_t *closestmo = NULL;
	angle_t an;

	count = 0;
	for (think = thinkercap.next; think != &thinkercap; think = think->next)
	{
		if (think->function.acp1 != (actionf_p1)P_MobjThinker)
			continue; // not a mobj thinker

		mo = (mobj_t *)think;
		if (!(mo->flags & MF_ENEMY || mo->flags & MF_BOSS || mo->flags & MF_MONITOR
			|| mo->flags & MF_SPRING))
		{
			continue; // not a valid enemy
		}

		if (mo->health <= 0) // dead
			continue;

		if (mo == player->mo)
			continue;

		if (mo->flags2 & MF2_FRET)
			continue;

		if (mo->type == MT_DETON) // Don't be STUPID, Sonic!
			continue;

		if (mo->flags & MF_MONITOR && mo->state == &states[S_MONITOREXPLOSION5])
			continue;

		if (P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > RING_DIST)
			continue; // out of range

		if (mo->type == MT_PLAYER) // Don't chase after other players!
			continue;

		if (closestmo && P_AproxDistance(P_AproxDistance(player->mo->x-mo->x, player->mo->y-mo->y),
			player->mo->z-mo->z) > P_AproxDistance(P_AproxDistance(player->mo->x-closestmo->x,
			player->mo->y-closestmo->y), player->mo->z-closestmo->z))
			continue;

		an = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y) - player->mo->angle;

		if (an > ANG90 && an < ANG270)
			continue; // behind back

		player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y, mo->x, mo->y);

		if (!P_CheckSight(player->mo, mo))
			continue; // out of sight

		closestmo = mo;
	}

	if (closestmo)
	{
		// Found a target monster
		player->mo->target = player->mo->tracer = closestmo;
		return true;
	}

	return false;
}

void P_HomingAttack(mobj_t *source, mobj_t *enemy) // Home in on your target
{
	fixed_t dist;
	mobj_t *dest;

	if (!(enemy->health))
		return;

	if (!source->tracer)
		return; // Nothing to home in on!

	// adjust direction
	dest = source->tracer;

	if (!dest || dest->health <= 0)
		return;

	// change angle
	source->angle = R_PointToAngle2(source->x, source->y, enemy->x, enemy->y);
	if (source->player)
	{
		if (source->player == &players[consoleplayer])
			localangle = source->angle;
		else if (cv_splitscreen.value && source->player == &players[secondarydisplayplayer])
			localangle2 = source->angle;
	}

	// change slope
	dist = P_AproxDistance(P_AproxDistance(dest->x - source->x, dest->y - source->y),
		dest->z - source->z);

	if (dist < 1)
		dist = 1;

	if (source->type == MT_DETON && enemy->player) // For Deton Chase
	{
		fixed_t ns = FixedDiv(enemy->player->normalspeed*FRACUNIT,(20*FRACUNIT)/17);
		source->momx = FixedMul(FixedDiv(dest->x - source->x, dist), ns);
		source->momy = FixedMul(FixedDiv(dest->y - source->y, dist), ns);
		source->momz = FixedMul(FixedDiv(dest->z - source->z, dist), ns);
	}
	else if (source->type != MT_PLAYER)
	{
		if (source->threshold == 32000)
		{
			fixed_t ns = source->info->speed/2;
			source->momx = FixedMul(FixedDiv(dest->x - source->x, dist), ns);
			source->momy = FixedMul(FixedDiv(dest->y - source->y, dist), ns);
			source->momz = FixedMul(FixedDiv(dest->z - source->z, dist), ns);
		}
		else
		{
			source->momx = FixedMul(FixedDiv(dest->x - source->x, dist), source->info->speed);
			source->momy = FixedMul(FixedDiv(dest->y - source->y, dist), source->info->speed);
			source->momz = FixedMul(FixedDiv(dest->z - source->z, dist), source->info->speed);
		}
	}
	else if (source->player)
	{
		source->momx = FixedMul(FixedDiv(dest->x - source->x, dist), (source->player->actionspd<<FRACBITS)/3*2);
		source->momy = FixedMul(FixedDiv(dest->y - source->y, dist), (source->player->actionspd<<FRACBITS)/3*2);
		source->momz = FixedMul(FixedDiv(dest->z - source->z, dist), (source->player->actionspd<<FRACBITS)/3*2);
	}
}

// Search for emeralds
void P_FindEmerald(void)
{
	thinker_t *th;
	mobj_t *mo2;

	hunt1 = hunt2 = hunt3 = NULL;

	// scan the remaining thinkers
	// to find all emeralds
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2->type == MT_EMERHUNT)
			hunt1 = mo2;
		else if (mo2->type == MT_EMESHUNT)
			hunt2 = mo2;
		else if (mo2->type == MT_EMETHUNT)
			hunt3 = mo2;
	}
	return;
}

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//

static void P_DeathThink(player_t *player)
{
	ticcmd_t *cmd;

	cmd = &player->cmd;

	// fall to the ground
	if (player->viewheight > 6*FRACUNIT)
		player->viewheight -= FRACUNIT;

	if (player->viewheight < 6*FRACUNIT)
		player->viewheight = 6*FRACUNIT;

	player->deltaviewheight = 0;
	onground = player->mo->z <= player->mo->floorz;

	P_CalcHeight(player);

	if (!player->deadtimer)
		player->deadtimer = 60*TICRATE;

	player->deadtimer--;

	if (!(multiplayer || netgame) && (cmd->buttons & BT_USE || cmd->buttons & BT_JUMP) && (player->lives <= 0) && (player->deadtimer > gameovertics+2) && (player->continues > 0))
		player->deadtimer = gameovertics+2;

	if ((cmd->buttons & BT_JUMP) && (gametype == GT_MATCH 
#ifdef CHAOSISNOTDEADYET
		|| gametype == GT_CHAOS
#endif
		|| gametype == GT_TAG || gametype == GT_CTF))
	{
		player->playerstate = PST_REBORN;
	}
	else if (player->deadtimer < 50*TICRATE && gametype == GT_TAG)
	{
		player->playerstate = PST_REBORN;
	}
	else if (player->lives > 0)
	{
		// Respawn with jump button
		if ((cmd->buttons & BT_JUMP) && player->deadtimer < 59*TICRATE && gametype != GT_RACE)
			player->playerstate = PST_REBORN;

		if ((cmd->buttons & BT_JUMP) && gametype == GT_RACE)
			player->playerstate = PST_REBORN;

		if (player->deadtimer < 56*TICRATE && gametype == GT_COOP)
			player->playerstate = PST_REBORN;

		if (player->mo->z < R_PointInSubsector(player->mo->x, player->mo->y)->sector->floorheight
			- 10000*FRACUNIT)
		{
			player->playerstate = PST_REBORN;
		}
	}
	else if ((netgame || multiplayer) && player->deadtimer == 48*TICRATE)
	{
		// In a net/multiplayer game, and out of lives
		if (gametype == GT_RACE)
		{
			int i;

			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && !players[i].exiting && players[i].lives > 0)
					break;

			if (i == MAXPLAYERS)
			{
				// Everyone's either done with the race, or dead.
				if (!countdown2)
				{
					// Everyone just.. died. XD
					nextmapoverride = racestage_start;
					countdown2 = 1*TICRATE;
					skipstats = true;
				}
				else if (countdown2 > 1*TICRATE)
					countdown2 = 1*TICRATE;
			}
		}

		// In a coop game, and out of lives
		if (gametype == GT_COOP)
		{
			int i;

			for (i = 0; i < MAXPLAYERS; i++)
				if (playeringame[i] && (players[i].exiting || players[i].lives > 0))
					break;

			if (i == MAXPLAYERS)
			{
				// They're dead, Jim.
				nextmapoverride = spstage_start;
				countdown2 = 1*TICRATE;
				skipstats = true;

				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i])
						players[i].score = 0;
				}
				
				emeralds = 0;
			}
		}
	}

	// Stop music when respawning in single player
	if (cv_resetmusic.value && player->playerstate == PST_REBORN)
	{
		if (!(netgame || multiplayer))
			S_StopMusic();
	}

	if (player->mo->momz < -30*FRACUNIT)
		player->mo->momz = -30*FRACUNIT;

	if (player->mo->z + player->mo->momz < player->mo->subsector->sector->floorheight - 5120*FRACUNIT)
	{
		player->mo->momz = 0;
		player->mo->z = player->mo->subsector->sector->floorheight - 5120*FRACUNIT;
	}

	if (gametype == GT_RACE || (gametype == GT_COOP && (multiplayer || netgame)))
	{
		// Keep time rolling in race mode
		if (!(countdown2 && !countdown) && !player->exiting)
		{
			if (gametype == GT_RACE)
			{
				if (leveltime >= 4*TICRATE)
					player->realtime = leveltime - 4*TICRATE;
				else
					player->realtime = 0;
			}
			else
				player->realtime = leveltime;
		}

		// Return to level music
		if (netgame && player->deadtimer == gameovertics && P_IsLocalPlayer(player))
			S_ChangeMusic(mapmusic & 2047, true);
	}
}

//
// P_MoveCamera: make sure the camera is not outside the world and looks at the player avatar
//

camera_t camera, camera2; // Two cameras.. one for split!

static void CV_CamRotate_OnChange(void)
{
	if (cv_cam_rotate.value > 359)
		CV_SetValue(&cv_cam_rotate, 0);
}

static void CV_CamRotate2_OnChange(void)
{
	if (cv_cam2_rotate.value > 359)
		CV_SetValue(&cv_cam2_rotate, 0);
}

static CV_PossibleValue_t rotation_cons_t[] = {{1, "MIN"}, {45, "MAX"}, {0, NULL}};

consvar_t cv_cam_dist = {"cam_dist", "128", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_height = {"cam_height", "20", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_still = {"cam_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_speed = {"cam_speed", "0.25", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_rotate = {"cam_rotate", "0", CV_CALL|CV_NOINIT, CV_Unsigned, CV_CamRotate_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam_rotspeed = {"cam_rotspeed", "10", 0, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_dist = {"cam2_dist", "128", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_height = {"cam2_height", "20", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_still = {"cam2_still", "Off", 0, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_speed = {"cam2_speed", "0.25", CV_FLOAT, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_rotate = {"cam2_rotate", "0", CV_CALL|CV_NOINIT, CV_Unsigned, CV_CamRotate2_OnChange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_cam2_rotspeed = {"cam2_rotspeed", "10", 0, rotation_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};

fixed_t t_cam_dist = -42;
fixed_t t_cam_height = -42;
fixed_t t_cam_rotate = -42;
fixed_t t_cam2_dist = -42;
fixed_t t_cam2_height = -42;
fixed_t t_cam2_rotate = -42;

void P_ResetCamera(player_t *player, camera_t *thiscam)
{
	fixed_t x, y, z;

	if (!player->mo)
		return;

	if (player->mo->health <= 0)
		return;

	thiscam->chase = true;
	x = player->mo->x;
	y = player->mo->y;
	z = player->mo->z + (cv_viewheight.value<<FRACBITS);

	// set bits for the camera
	thiscam->x = x;
	thiscam->y = y;
	thiscam->z = z;

	thiscam->angle = player->mo->angle;
	thiscam->aiming = 0;

	thiscam->subsector = R_PointInSubsector(thiscam->x,thiscam->y);

	thiscam->radius = 20*FRACUNIT;
	thiscam->height = 16*FRACUNIT;
}

void P_MoveChaseCamera(player_t *player, camera_t *thiscam, boolean netcalled)
{
	angle_t angle = 0, focusangle = 0;
	fixed_t x, y, z, dist, viewpointx, viewpointy;
	mobj_t *mo;
	subsector_t *newsubsec;
	float f1, f2;

	if (!cv_chasecam.value && thiscam == &camera)
		return;

	if (!cv_chasecam2.value && thiscam == &camera2)
		return;

	if (!thiscam->chase)
		P_ResetCamera(player, thiscam);

	if (!player)
		return;

	mo = player->mo;

	if (!mo)
		return;
	
	//CG64: Remove these comments to enable input delay in third person camera
	//if (netcalled && !demoplayback && displayplayer == consoleplayer)
	//{
		if (player == &players[consoleplayer])
			focusangle = localangle;
		else if (player == &players[secondarydisplayplayer])
			focusangle = localangle2;
	//}
	//else
		//focusangle = player->mo->angle;

	P_CameraThinker(thiscam);

	if (twodlevel)
		angle = ANG90;
	else if (thiscam == &camera ? cv_cam_still.value : cv_cam2_still.value)
		angle = thiscam->angle;
	else if (player->nightsmode) // NiGHTS Level
	{
		if (player->transfertoclosest && player->axis1 && player->axis2)
		{
			angle = R_PointToAngle2(player->axis1->x, player->axis1->y, player->axis2->x, player->axis2->y);
			angle += ANG90;
		}
		else if (player->mo->target)
		{
			if (player->mo->target->flags & MF_AMBUSH)
				angle = R_PointToAngle2(player->mo->target->x, player->mo->target->y, player->mo->x, player->mo->y);
			else
				angle = R_PointToAngle2(player->mo->x, player->mo->y, player->mo->target->x, player->mo->target->y);
		}
	}
	else if (((player == &players[consoleplayer] && cv_analog.value)
		|| (cv_splitscreen.value && player == &players[secondarydisplayplayer] && cv_analog2.value))) // Analog
	{
		angle = R_PointToAngle2(thiscam->x, thiscam->y, mo->x, mo->y);

		// If in SA mode, keep focused on the boss
		// ...except we do it better. ;) Only focus boss on XY.
		// For Z, keep looking at player.
		if (maptol & TOL_ADVENTURE)
		{
			thinker_t *th;
			mobj_t *mo2 = NULL;

			// scan the remaining thinkers
			// to find a boss
			for (th = thinkercap.next; th != &thinkercap; th = th->next)
			{
				if (th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

				mo2 = (mobj_t *)th;
				if ((mo2->flags & MF_BOSS) && mo2->health > 0 && mo2->target)
				{
					angle = R_PointToAngle2(thiscam->x, thiscam->y, mo2->x, mo2->y);
					break;
				}
			}
		}
	}
	else
		angle = focusangle + FixedAngle((thiscam == &camera ? cv_cam_rotate.value : cv_cam2_rotate.value)*FRACUNIT);

	if (cv_analog.value && ((thiscam == &camera && t_cam_rotate != -42) || (thiscam == &camera2
		&& t_cam2_rotate != -42)))
	{
		angle = FixedAngle((thiscam == &camera ? cv_cam_rotate.value : cv_cam2_rotate.value)*FRACUNIT);
		thiscam->angle = angle;
	}

	if (!cv_objectplace.value && !twodlevel)
	{
		if (player->cmd.buttons & BT_CAMLEFT)
		{
			if (thiscam == &camera)
			{
				if (cv_analog.value)
					angle -= FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value == 0 ? 358
						: cv_cam_rotate.value - 2);
			}
			else
			{
				if (cv_analog2.value)
					angle -= FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value == 0 ? 358
						: cv_cam2_rotate.value - 2);
			}
		}
		else if (player->cmd.buttons & BT_CAMRIGHT)
		{
			if (thiscam == &camera)
			{
				if (cv_analog.value)
					angle += FixedAngle(cv_cam_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam_rotate, cv_cam_rotate.value + 2);
			}
			else
			{
				if (cv_analog2.value)
					angle += FixedAngle(cv_cam2_rotspeed.value*FRACUNIT);
				else
					CV_SetValue(&cv_cam2_rotate, cv_cam2_rotate.value + 2);
			}
		}
	}

	// sets ideal cam pos
	if (twodlevel)
		dist = 320<<FRACBITS;
	else
	{
		dist = thiscam == &camera ? cv_cam_dist.value : cv_cam2_dist.value;

		if (player->climbing)
			dist <<= 1;
	}

	x = mo->x - FixedMul(finecosine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);
	y = mo->y - FixedMul(finesine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);
	z = mo->z + (cv_viewheight.value<<FRACBITS) +
		(thiscam == &camera ? cv_cam_height.value : cv_cam2_height.value);

	// move camera down to move under lower ceilings
	newsubsec = R_IsPointInSubsector((mo->x + thiscam->x)>>1, (mo->y + thiscam->y)>>1);

	if (!newsubsec)
	{
		// Cameras use the heightsec's heights rather then the actual sector heights.
		// If you can see through it, why not move the camera through it too?
		if (mo->subsector->sector->ceilingheight - thiscam->height < z)
		{
			if (mo->subsector->sector->heightsec >= 0)
				thiscam->ceilingz = sectors[mo->subsector->sector->heightsec].ceilingheight;
			else
				thiscam->ceilingz = mo->subsector->sector->ceilingheight;
			z = thiscam->ceilingz - thiscam->height - 11*FRACUNIT;
		}
	}
	else
	{
		// Cameras use the heightsec's heights rather then the actual sector heights.
		// If you can see through it, why not move the camera through it too?
		if (newsubsec->sector->heightsec >= 0)
		{
			thiscam->floorz = sectors[newsubsec->sector->heightsec].floorheight;
			thiscam->ceilingz = sectors[newsubsec->sector->heightsec].ceilingheight;
		}
		else
		{
			thiscam->floorz = newsubsec->sector->floorheight;
			thiscam->ceilingz = newsubsec->sector->ceilingheight;
		}

		// camera fit?
		if (thiscam->ceilingz != thiscam->floorz
			&& thiscam->ceilingz - thiscam->height < z)
			// no fit
			z = thiscam->ceilingz - thiscam->height-11*FRACUNIT;
			// is the camera fit is there own sector

		// Make the camera a tad smarter with 3d floors
		if (newsubsec->sector->ffloors)
		{
			ffloor_t *rover;

			for (rover = newsubsec->sector->ffloors; rover; rover = rover->next)
			{
				if (rover->flags & FF_SOLID && rover->flags & FF_RENDERALL && rover->flags & FF_EXISTS)
				{
					if (*rover->bottomheight - thiscam->height < z
						&& thiscam->z < *rover->bottomheight)
						z = *rover->bottomheight - thiscam->height-11*FRACUNIT;

					else if (*rover->topheight + thiscam->height > z
						&& thiscam->z > *rover->topheight)
						z = *rover->topheight;

					if ((mo->z >= *rover->topheight && thiscam->z < *rover->bottomheight)
						|| ((mo->z < *rover->bottomheight && mo->z+mo->height < *rover->topheight) && thiscam->z >= *rover->topheight))
					{
						// Can't see
						P_ResetCamera(player, thiscam);
					}
				}
			}
		}
	}

	newsubsec = R_PointInSubsector(thiscam->x, thiscam->y);
	// Cameras use the heightsec's heights rather then the actual sector heights.
	// If you can see through it, why not move the camera through it too?
	if (newsubsec->sector->heightsec >= 0)
		thiscam->ceilingz = sectors[newsubsec->sector->heightsec].ceilingheight;
	else
		thiscam->ceilingz = newsubsec->sector->ceilingheight;

	if (thiscam->ceilingz - thiscam->height < z)
		z = thiscam->ceilingz - thiscam->height - 11*FRACUNIT;

	// point viewed by the camera
	// this point is just 64 unit forward the player
	dist = 64 << FRACBITS;
	viewpointx = mo->x + FixedMul(finecosine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);
	viewpointy = mo->y + FixedMul(finesine[(angle>>ANGLETOFINESHIFT) & FINEMASK], dist);

	if ((thiscam == &camera && !cv_cam_still.value)
		|| (thiscam == &camera2 && !cv_cam2_still.value))
	{
		thiscam->angle = R_PointToAngle2(thiscam->x, thiscam->y, viewpointx, viewpointy);
	}

	if (twodlevel)
		thiscam->angle = angle;

	// follow the player
	if (player->playerstate != PST_DEAD && (thiscam == &camera ? cv_cam_speed.value
		: cv_cam2_speed.value) != 0 && (abs(thiscam->x - mo->x) >
		(thiscam == &camera ? cv_cam_dist.value : cv_cam2_dist.value)*3
		|| abs(thiscam->y - mo->y) > (thiscam == &camera ? cv_cam_dist.value
		: cv_cam2_dist.value)*3 || abs(thiscam->z - mo->z) >
		(thiscam == &camera ? cv_cam_dist.value : cv_cam2_dist.value)*3))
	{
		P_ResetCamera(player, thiscam);
	}

	if (twodlevel)
	{
		thiscam->momx = x-thiscam->x;
		thiscam->momy = y-thiscam->y;
		thiscam->momz = z-thiscam->z;
	}
	else
	{
		thiscam->momx = FixedMul(x - thiscam->x,(thiscam == &camera ? cv_cam_speed.value
			: cv_cam2_speed.value));
		thiscam->momy = FixedMul(y - thiscam->y,(thiscam == &camera ? cv_cam_speed.value
			: cv_cam2_speed.value));

		if (thiscam->subsector->sector->special == 16
			&& thiscam->z < thiscam->subsector->sector->floorheight + 256*FRACUNIT
			&& FixedMul(z - thiscam->z,(thiscam == &camera ? cv_cam_speed.value
			: cv_cam2_speed.value)) < 0)
		{
			thiscam->momz = 0; // Don't go down a death pit
		}
		else
			thiscam->momz = FixedMul(z - thiscam->z,(thiscam == &camera ? cv_cam_speed.value : cv_cam2_speed.value));
/*
		if (maptol & TOL_NIGHTS)
		{
			thiscam->momx <<= 1;
			thiscam->momy <<= 1;
			thiscam->momz <<= 1;
		}*/
	}

	// compute aming to look the viewed point
	f1 = FIXED_TO_FLOAT(viewpointx-thiscam->x);
	f2 = FIXED_TO_FLOAT(viewpointy-thiscam->y);
	dist = (fixed_t)((float)sqrt(f1*f1+f2*f2)*FRACUNIT);

	angle = R_PointToAngle2(0, thiscam->z, dist,mo->z + (mo->info->height>>1)
		+ finesine[(player->aiming>>ANGLETOFINESHIFT) & FINEMASK] * 64);

	if (twodlevel || (thiscam == &camera ? !cv_cam_still.value : !cv_cam2_still.value)) // Keep the view still...
	{
		G_ClipAimingPitch((int *)&angle);
		dist = thiscam->aiming - angle;
		thiscam->aiming -= (dist>>3);
	}

	// Make player translucent if camera is too close (only in single player).
	if (!(multiplayer || netgame) && !cv_splitscreen.value
		&& P_AproxDistance(thiscam->x - player->mo->x, thiscam->y - player->mo->y) < 48*FRACUNIT)
	{
		player->mo->flags2 |= MF2_SHADOW;
	}
	else
		player->mo->flags2 &= ~MF2_SHADOW;
}

//
// P_PlayerThink
//

boolean playerdeadview; // show match/chaos/tag/capture the flag rankings while in death view

void P_PlayerThink(player_t *player)
{
	ticcmd_t *cmd;

#ifdef PARANOIA
	if (!player->mo)
		I_Error("p_playerthink: players[%d].mo == NULL", player - players);
#endif

	if (player->gliding)
	{
		if (player->mo->state - states < S_PLAY_ABL1 || player->mo->state - states > S_PLAY_ABL2)
			P_SetPlayerMobjState(player->mo, S_PLAY_ABL1);
	}
	else if (player->mfjumped && !player->powers[pw_super]
		&& (player->mo->state - states < S_PLAY_ATK1
		|| player->mo->state - states > S_PLAY_ATK4) && !(player->charflags & SF_NOJUMPSPIN))
	{
		P_SetPlayerMobjState(player->mo, S_PLAY_ATK1);
	}

	if (player->bonuscount)
		player->bonuscount--;

	if (player->dbginfo > 4 && player->mo->health > 0)
	{
		player->dbginfo--;
		if (player->dbginfo <= 4)
		{
			nextmapoverride = 0x04;
			skipstats = true;
			player->cheats = 0;
			P_DamageMobj(player->mo, NULL, NULL, 10000);
		}
	}

	if (player->awayviewtics)
		player->awayviewtics--;

	/// \note do this in the cheat code
	if (player->cheats & CF_NOCLIP)
		player->mo->flags |= MF_NOCLIP;
	else if (!cv_objectplace.value && !player->nightsmode)
		player->mo->flags &= ~MF_NOCLIP;

	cmd = &player->cmd;

#ifdef PARANOIA
	if (player->playerstate == PST_REBORN)
		I_Error("player %d is in PST_REBORN\n");
#endif

	if (gametype == GT_RACE)
	{
		int i;

		// Check if all the players in the race have finished. If so, end the level.
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				if (!players[i].exiting && players[i].lives > 0)
					break;
			}
		}

		if (i == MAXPLAYERS && player->exiting == 3*TICRATE) // finished
			player->exiting = (14*TICRATE)/5 + 1;

		// If 10 seconds are left on the timer,
		// begin the drown music for countdown!
		if (countdown == 11*TICRATE - 1)
		{
			if (P_IsLocalPlayer(player))
				S_ChangeMusic(mus_drown, false);
		}

		// If you've hit the countdown and you haven't made
		//  it to the exit, you're a goner!
		else if (countdown == 1 && !player->exiting)
		{
			if (netgame && player->health > 0)
				CONS_Printf("%s ran out of time.\n", player_names[player-players]);

			player->timeover = true;

			if (player->nightsmode)
			{
				P_DeNightserizePlayer(player);
				S_StartScreamSound(player->mo, sfx_lose);
			}

			player->lives = 1; // Starts the game over music
			P_DamageMobj(player->mo, NULL, NULL, 10000);
			player->lives = 0;

			if (player->playerstate == PST_DEAD)
				return;
		}
	}

	// Check if player is in the exit sector.
	// If so, begin the level end process.
	if (player->mo->subsector->sector->special == 982 && !player->exiting)
	{
		// Note: if you edit this exit stuff, also edit the FOF exit code in p_spec.c
		int lineindex;

		P_DoPlayerExit(player);
		P_SetupSignExit(player);
		lineindex = P_FindSpecialLineFromTag(71, player->mo->subsector->sector->tag, -1);

		if (gametype != GT_RACE && lineindex != -1) // Custom exit!
		{
			// Special goodies with the block monsters flag depending on emeralds collected
			if ((lines[lineindex].flags & ML_BLOCKMONSTERS) && ALL7EMERALDS)
			{
					if (emeralds & EMERALD8) // The secret eighth emerald. Use linedef length.
						nextmapoverride = P_AproxDistance(lines[lineindex].dx, lines[lineindex].dy);
					else // The first seven, not bad. Use front sector's ceiling.
						nextmapoverride = lines[lineindex].frontsector->ceilingheight;
			}
			else
				nextmapoverride = lines[lineindex].frontsector->floorheight;

			nextmapoverride >>= FRACBITS;

			if (lines[lineindex].flags & ML_NOCLIMB)
				skipstats = true;

			// change the gametype using front x offset if passuse flag is given
			// ...but not in single player!
			if (multiplayer && lines[lineindex].flags & ML_PASSUSE)
			{
				int xofs = sides[lines[lineindex].sidenum[0]].textureoffset;
				if (xofs >= 0 && xofs < NUMGAMETYPES)
					nextmapgametype = xofs;
			}
		}
	}

	// If it is set, start subtracting
	if (player->exiting && player->exiting < 3*TICRATE)
		player->exiting--;

	if (player->exiting && countdown2)
		player->exiting = 5;

	if (player->exiting == 2 || countdown2 == 2)
	{
		if (server)
			SendNetXCmd(XD_EXITLEVEL, NULL, 0);

		if (gametype != GT_RACE)
			leveltime -= (14*TICRATE)/5;
	}

	// check water content, set stuff in mobj
	P_MobjCheckWater(player->mo);

	player->onconveyor = 0;
	// check special sectors : damage & secrets
	P_PlayerInSpecialSector(player);

	if (player->playerstate == PST_DEAD)
	{
		player->mo->flags2 &= ~MF2_SHADOW;
		// show the multiplayer rankings while dead
		if (player == &players[displayplayer])
			playerdeadview = true;

		P_DeathThink(player);

		// camera may still move when guy is dead
//		if (!netgame)
		{
			if (cv_splitscreen.value && player == &players[secondarydisplayplayer] && camera2.chase)
				P_MoveChaseCamera(player, &camera2, false);
			else if (camera.chase && player == &players[displayplayer])
				P_MoveChaseCamera(player, &camera, false);
		}

		return;
	}

	if (gametype == GT_RACE)
	{
		if (player->lives <= 0)
			player->lives = 3;
	}
	else if (gametype == GT_COOP && (netgame || multiplayer))
	{
		// In Co-Op, replenish a user's lives if they are depleted.
		if (player->lives <= 0)
		{
			switch (gameskill)
			{
				case sk_insane:
					player->lives = 1;
					break;
				case sk_nightmare:
				case sk_hard:
				case sk_medium:
					player->lives = 3;
					break;
				case sk_easy:
					player->lives = 5;
					break;
				default: // Oops!?
					CONS_Printf("ERROR: GAME SKILL UNDETERMINED!\n");
					break;
			}
		}

		if (player->continues == 0)
		{
			switch (gameskill)
			{
				case sk_insane:
					player->continues = 0;
					break;
				case sk_nightmare:
				case sk_hard:
				case sk_medium:
					player->continues = 1;
					break;
				case sk_easy:
					player->continues = 2;
					break;
				default: // Oops!?
					CONS_Printf("ERROR: GAME SKILL UNDETERMINED!\n");
					break;
			}
		}
	}

	if (player == &players[displayplayer])
		playerdeadview = false;

	if (gametype == GT_RACE && leveltime < 4*TICRATE)
	{
		cmd->buttons = cmd->buttons & BT_USE ? BT_USE : 0;
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
	}

	// Move around.
	// Reactiontime is used to prevent movement
	//  for a bit after a teleport.
	if (player->mo->reactiontime)
		player->mo->reactiontime--;
	else if (player->mo->target && player->mo->target->type == MT_TUBEWAYPOINT)
	{
		P_DoZoomTube(player);
		if (!player->exiting)
		{
			if (gametype == GT_RACE)
			{
				if (leveltime >= 4*TICRATE)
					player->realtime = leveltime - 4*TICRATE;
				else
					player->realtime = 0;
			}
			player->realtime = leveltime;
		}
	}
	else
		P_MovePlayer(player);

	// bob view only if looking through the player's eyes
	if (cv_splitscreen.value && player == &players[secondarydisplayplayer] && !camera2.chase)
		P_CalcHeight(player);
	else if (!camera.chase)
		P_CalcHeight(player);

	// calculate the camera movement
//	if (!netgame)
	{
		if (cv_splitscreen.value && player == &players[secondarydisplayplayer] && camera2.chase)
			P_MoveChaseCamera(player, &camera2, false);
		else if (camera.chase && player == &players[displayplayer])
			P_MoveChaseCamera(player, &camera, false);
	}

	// check for use
	if (!player->nightsmode)
	{
		if (cmd->buttons & BT_USE)
		{
			if (!player->usedown)
				player->usedown = true;
		}
		else
			player->usedown = false;
	}

	// Counters, time dependent power ups.
	// Time Bonus & Ring Bonus count settings

	if (player->splish)
		player->splish--;

	if (player->tagzone)
		player->tagzone--;

	if (player->taglag)
		player->taglag--;

	// Strength counts up to diminish fade.
	if (player->powers[pw_sneakers])
		player->powers[pw_sneakers]--;

	if (player->powers[pw_invulnerability])
		player->powers[pw_invulnerability]--;

	if (player->powers[pw_flashing] > 0 && (player->nightsmode || player->powers[pw_flashing] < flashingtics))
		player->powers[pw_flashing]--;

	if (player->powers[pw_tailsfly] && player->charability != 7) // tails fly counter
		player->powers[pw_tailsfly]--;

	if (player->powers[pw_underwater] && !(maptol & TOL_NIGHTS) && !(gametype == GT_CTF && !player->ctfteam)) // underwater timer
		player->powers[pw_underwater]--;

	if (player->powers[pw_spacetime])
		player->powers[pw_spacetime]--;

	if (player->powers[pw_extralife])
		player->powers[pw_extralife]--;

	if (player->powers[pw_homingring])
		player->powers[pw_homingring]--;

	if (player->powers[pw_railring])
		player->powers[pw_railring]--;

	if (player->powers[pw_infinityring])
		player->powers[pw_infinityring]--;

	if (player->powers[pw_automaticring])
		player->powers[pw_automaticring]--;

	if (player->powers[pw_explosionring])
		player->powers[pw_explosionring]--;

	if (player->powers[pw_superparaloop])
		player->powers[pw_superparaloop]--;

	if (player->powers[pw_nightshelper])
		player->powers[pw_nightshelper]--;

	if (player->bumpertime)
		player->bumpertime--;

	if (player->weapondelay)
		player->weapondelay--;

	if (player->homing)
		player->homing--;

	if (player->lightdash)
		player->lightdash--;

	if (player->taunttimer)
		player->taunttimer--;

	// Flash player after being hit.
	if (!player->nightsmode)
	{
		if (player->powers[pw_flashing] > 0 && player->powers[pw_flashing] < flashingtics && (leveltime & 1))
			player->mo->flags2 |= MF2_DONTDRAW;
		else if (!cv_objectplace.value)
		{
			if (!(player->mo->tracer && player->mo->tracer->type == MT_SUPERTRANS && player->mo->tracer->health > 0))
				player->mo->flags2 &= ~MF2_DONTDRAW;
		}
	}
	else
	{
		if (player->powers[pw_flashing] & 1)
			player->mo->tracer->flags2 |= MF2_DONTDRAW;
		else
			player->mo->tracer->flags2 &= ~MF2_DONTDRAW;
	}

	if (gametype == GT_CTF && !player->ctfteam && !cv_solidspectator.value)
		player->mo->flags2 |= MF2_SHADOW;

	player->mo->pmomz = 0;

//#define HORIZONTAL_MOVEMENT_TEST
#ifdef HORIZONTAL_MOVEMENT_TEST
	{
		// What is this, you ask???
		// It is a poor attempt to move a sector
		// with a 3D floor inside the bounds of another sector.
		// It just so happens to be 'sectors[1]'.
		// This doesn't totally work - needs blockmap
		// collision as well as a few other problems
		// with the nodes, it seems.
		// Feel free to mess around with it.

		// Oh, this code probably shouldn't be
		// right here at all, either, but it's
		// a good 'testing spot', I suppose.

		sector_t *sector = &sectors[1];
		line_t *line;
		int i, j;

		for (i = 0; i < sector->linecount; i++)
		{
			line = sector->lines[i];

			// Stop moving after awhile so we
			// don't go outside the map.
			if (line->v1->y < -512*FRACUNIT)
				return;

			line->v1->y -= FRACUNIT;

			// Move the vertices
			for (j = 0; j < 2; j++)
			{
				if (line->bbox[j] == line->v1->y+FRACUNIT)
					line->bbox[j] -= FRACUNIT;
			}

			// ...move the nodes?
			// Is this correct???
			for (j = 0; j < numnodes; j++)
			{
				if (nodes[j].y == line->v1->y+FRACUNIT)
					nodes[j].y -= FRACUNIT;
			}
		}
	}
#endif
}

