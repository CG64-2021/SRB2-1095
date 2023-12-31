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
/// \brief Enemy thinking, AI
/// 
///	Action Pointer Functions that are associated with states/frames

#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_state.h"
#include "s_sound.h"
#include "m_random.h"
#include "r_things.h"

#include "hardware/hw3sound.h"

player_t *stplyr;
int var1;
int var2;

typedef enum
{
	DI_NODIR = -1,
	DI_EAST = 0,
	DI_NORTHEAST = 1,
	DI_NORTH = 2,
	DI_NORTHWEST = 3,
	DI_WEST = 4,
	DI_SOUTHWEST = 5,
	DI_SOUTH = 6,
	DI_SOUTHEAST = 7,
	NUMDIRS = 8,
} dirtype_t;

//
// P_NewChaseDir related LUT.
//
static dirtype_t opposite[] =
{
	DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
	DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

static dirtype_t diags[] =
{
	DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};

//Real Prototypes to A_*
void A_Fall(mobj_t *actor);
void A_Look(mobj_t *actor);
void A_Chase(mobj_t *actor);
void A_SkimChase(mobj_t *actor);
void A_FaceTarget(mobj_t *actor);
void A_FireShot(mobj_t *actor);
void A_SuperFireShot(mobj_t *actor);
void A_SkullAttack(mobj_t *actor);
void A_BossZoom(mobj_t *actor);
void A_BossScream(mobj_t *actor);
void A_Scream(mobj_t *actor);
void A_Pain(mobj_t *actor);
void A_1upThinker(mobj_t *actor);
void A_MonitorPop(mobj_t *actor);
void A_Explode(mobj_t *actor);
void A_BossDeath(mobj_t *actor);
void A_CustomPower(mobj_t *actor);
void A_JumpShield(mobj_t *actor);
void A_RingShield(mobj_t *actor);
void A_RingBox(mobj_t *actor);
void A_Invincibility(mobj_t *actor);
void A_SuperSneakers(mobj_t *actor);
void A_ExtraLife(mobj_t *actor);
void A_BombShield(mobj_t *actor);
void A_WaterShield(mobj_t *actor);
void A_FireShield(mobj_t *actor);
void A_ScoreRise(mobj_t *actor);
void A_BunnyHop(mobj_t *actor);
void A_BubbleSpawn(mobj_t *actor);
void A_BubbleRise(mobj_t *actor);
void A_BubbleCheck(mobj_t *actor);
void A_AttractChase(mobj_t *actor);
void A_DropMine(mobj_t *actor);
void A_FishJump(mobj_t *actor);
void A_SignPlayer(mobj_t *actor);
void A_ThrownRing(mobj_t *actor);
void A_SetSolidSteam(mobj_t *actor);
void A_UnsetSolidSteam(mobj_t *actor);
void A_JetChase(mobj_t *actor);
void A_JetbThink(mobj_t *actor);
void A_JetgShoot(mobj_t *actor);
void A_JetgThink(mobj_t *actor);
void A_ShootBullet(mobj_t *actor);
void A_MouseThink(mobj_t *actor);
void A_DetonChase(mobj_t *actor);
void A_CapeChase(mobj_t *actor);
void A_RotateSpikeBall(mobj_t *actor);
void A_SnowBall(mobj_t *actor);
void A_CrawlaCommanderThink(mobj_t *actor);
void A_RingExplode(mobj_t *actor);
void A_MixUp(mobj_t *actor);
void A_PumaJump(mobj_t *actor);
void A_Invinciblerize(mobj_t *actor);
void A_DeInvinciblerize(mobj_t *actor);
void A_Boss2PogoSFX(mobj_t *actor);
void A_EggmanBox(mobj_t *actor);
void A_TurretFire(mobj_t *actor);
void A_SuperTurretFire(mobj_t *actor);
void A_TurretStop(mobj_t *actor);
void A_SparkFollow(mobj_t *actor);
void A_BuzzFly(mobj_t *actor);
void A_SetReactionTime(mobj_t *actor);
void A_LinedefExecute(mobj_t *actor);
void A_PlaySeeSound(mobj_t *actor);
void A_PlayAttackSound(mobj_t *actor);
void A_PlayActiveSound(mobj_t *actor);
void A_SmokeTrailer(mobj_t *actor);
void A_SpawnObjectAbsolute(mobj_t *actor);
void A_SpawnObjectRelative(mobj_t *actor);
void A_ChangeAngleRelative(mobj_t *actor);
void A_ChangeAngleAbsolute(mobj_t *actor);
void A_PlaySound(mobj_t *actor);
void A_FindTarget(mobj_t *actor);
void A_FindTracer(mobj_t *actor);
void A_SetTics(mobj_t *actor);
void A_ChangeColorRelative(mobj_t *actor);
void A_ChangeColorAbsolute(mobj_t *actor);
void A_MoveRelative(mobj_t *actor);
void A_MoveAbsolute(mobj_t *actor);
void A_SetTargetsTarget(mobj_t *actor);
void A_SetObjectFlags(mobj_t *actor);
void A_SetObjectFlags2(mobj_t *actor);
void A_RandomState(mobj_t *actor);
void A_RandomStateRange(mobj_t *actor);
void A_DualAction(mobj_t *actor);
//for p_enemy.c
void A_Boss1Chase(mobj_t *actor);
void A_Boss2Chase(mobj_t *actor);
void A_Boss2Pogo(mobj_t *actor);
void A_BossJetFume(mobj_t *actor);

//
// ENEMY THINKING
// Enemies are always spawned with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players, but some can be made preaware.
//

//
// P_CheckMeleeRange
//
static boolean P_CheckMeleeRange(mobj_t *actor)
{
	mobj_t *pl;
	fixed_t dist;

	if (!actor->target)
		return false;

	pl = actor->target;
	dist = P_AproxDistance(pl->x-actor->x, pl->y-actor->y);

	switch (actor->type)
	{
		case MT_JETTBOMBER:
			if (dist >= (actor->radius + pl->radius)*2)
				return false;
			break;
		default:
			if (dist >= MELEERANGE - 20*FRACUNIT + pl->radius)
				return false;
			break;
	}

	// check height now, so that damn crawlas cant attack
	// you if you stand on a higher ledge.
	if (actor->type == MT_JETTBOMBER)
	{
		if (pl->z + pl->height > actor->z - (40<<FRACBITS))
			return false;
	}
	else if (actor->type == MT_SKIM)
	{
		if (pl->z + pl->height > actor->z - (24<<FRACBITS))
			return false;
	}
	else
	{
		if ((pl->z > actor->z + actor->height) || (actor->z > pl->z + pl->height))
			return false;

		if (actor->type != MT_JETTBOMBER && actor->type != MT_SKIM
			&& !P_CheckSight(actor, actor->target))
		{
			return false;
		}
	}

	return true;
}

//
// P_CheckMissileRange
//
static boolean P_CheckMissileRange(mobj_t *actor)
{
	fixed_t dist;

	if (!P_CheckSight(actor, actor->target))
		return false;

	if (actor->reactiontime)
		return false; // do not attack yet

	// OPTIMIZE: get this from a global checksight
	dist = P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) - 64*FRACUNIT;

	if (!actor->info->meleestate)
		dist -= 128*FRACUNIT; // no melee attack, so fire more

	dist >>= 16;

	if (actor->type == MT_EGGMOBILE)
		dist >>= 1;

	if (dist > 200)
		dist = 200;

	if (actor->type == MT_EGGMOBILE && dist > 160)
		dist = 160;

	if (P_Random() < dist)
		return false;

	return true;
}

/** Checks for water in a sector.
  * Used by Skim movements.
  *
  * \param x X coordinate on the map.
  * \param y Y coordinate on the map.
  * \return True if there's water at this location, false if not.
  * \sa ::MT_SKIM
  */
static boolean P_WaterInSector(mobj_t *mobj, fixed_t x, fixed_t y)
{
	sector_t *sector;
	fixed_t height = -1;

	sector = R_PointInSubsector(x, y)->sector;

	if (sector->ffloors)
	{
		ffloor_t *rover;

		for (rover = sector->ffloors; rover; rover = rover->next)
		{
			if (!(rover->flags & FF_EXISTS))
				continue;

			if (rover->flags & FF_SWIMMABLE)
			{
				if (*rover->topheight >= mobj->floorz
					&& *rover->topheight <= mobj->z)
					height = *rover->topheight;
			}
		}
	}

	if (height != -1)
		return true;

	return false;
}

static const fixed_t xspeed[NUMDIRS] = {FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000, 0, 47000};
static const fixed_t yspeed[NUMDIRS] = {0, 47000, FRACUNIT, 47000, 0, -47000, -FRACUNIT, -47000};

/** Moves an actor in its current direction.
  *
  * \param actor Actor object to move.
  * \return False if the move is blocked, otherwise true.
  */
static boolean P_Move(mobj_t *actor)
{
	fixed_t tryx, tryy;
	dirtype_t movedir = actor->movedir;

	if (movedir == DI_NODIR)
		return false;

	I_Assert((unsigned)movedir < 8);

	tryx = actor->x + actor->info->speed*xspeed[movedir];
	tryy = actor->y + actor->info->speed*yspeed[movedir];

	if (actor->type == MT_SKIM && !P_WaterInSector(actor, tryx, tryy)) // bail out if sector lacks water
		return false;

	if (!P_TryMove(actor, tryx, tryy, false))
	{
		// open any specials
		if (actor->flags & MF_FLOAT && floatok)
		{
			// must adjust height
			if (actor->z < tmfloorz)
				actor->z += FLOATSPEED;
			else
				actor->z -= FLOATSPEED;

			actor->flags2 |= MF2_INFLOAT;
			return true;
		}

		if (!numspechit)
			return false;

		actor->movedir = (angle_t)DI_NODIR;
		return false;
	}
	else
		actor->flags2 &= ~MF2_INFLOAT;

	return true;
}

/** Attempts to move an actor on in its current direction.
  * If the move succeeds, the actor's move count is reset
  * randomly to a value from 0 to 15.
  *
  * \param actor Actor to move.
  * \return True if the move succeeds, false if the move is blocked.
  */
static boolean P_TryWalk(mobj_t *actor)
{
	if (!P_Move(actor))
		return false;
	actor->movecount = P_Random() & 15;
	return true;
}

static void P_NewChaseDir(mobj_t *actor)
{
	fixed_t deltax, deltay;
	dirtype_t d[3];
	dirtype_t tdir = DI_NODIR, olddir, turnaround;

#ifdef PARANOIA
	if (!actor->target)
		I_Error("P_NewChaseDir: called with no target");
#endif

	olddir = actor->movedir;

	if (olddir >= NUMDIRS)
		olddir = DI_NODIR;

	if (olddir != DI_NODIR)
		turnaround = opposite[olddir];
	else
		turnaround = olddir;

	deltax = actor->target->x - actor->x;
	deltay = actor->target->y - actor->y;

	if (deltax > 10*FRACUNIT)
		d[1] = DI_EAST;
	else if (deltax < -10*FRACUNIT)
		d[1] = DI_WEST;
	else
		d[1] = DI_NODIR;

	if (deltay < -10*FRACUNIT)
		d[2] = DI_SOUTH;
	else if (deltay > 10*FRACUNIT)
		d[2] = DI_NORTH;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		dirtype_t newdir = diags[((deltay < 0)<<1) + (deltax > 0)];

		actor->movedir = newdir;
		if ((newdir != turnaround) && P_TryWalk(actor))
			return;
	}

	// try other directions
	if (P_Random() > 200 || abs(deltay) > abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] == turnaround)
		d[1] = DI_NODIR;
	if (d[2] == turnaround)
		d[2] = DI_NODIR;

	if (d[1] != DI_NODIR)
	{
		actor->movedir = d[1];

		if (P_TryWalk(actor))
			return; // either moved forward or attacked
	}

	if (d[2] != DI_NODIR)
	{
		actor->movedir = d[2];

		if (P_TryWalk(actor))
			return;
	}

	// there is no direct path to the player, so pick another direction.
	if (olddir != DI_NODIR)
	{
		actor->movedir =olddir;

		if (P_TryWalk(actor))
			return;
	}

	// randomly determine direction of search
	if (P_Random() & 1)
	{
		for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
		{
			if (tdir != turnaround)
			{
				actor->movedir = tdir;

				if (P_TryWalk(actor))
					return;
			}
		}
	}
	else
	{
		for (tdir = DI_SOUTHEAST; tdir >= DI_EAST; tdir--)
		{
			if (tdir != turnaround)
			{
				actor->movedir = tdir;

				if (P_TryWalk(actor))
					return;
			}
		}
	}

	if (turnaround != DI_NODIR)
	{
		actor->movedir = turnaround;

		if (P_TryWalk(actor))
			return;
	}

	actor->movedir = (angle_t)DI_NODIR; // cannot move
}

/** Looks for players to chase after, aim at, or whatever.
  *
  * \param actor     The object looking for flesh.
  * \param allaround Look all around? If false, only players in a 180-degree
  *                  range in front will be spotted.
  * \return True if a player is found, otherwise false.
  * \sa P_SupermanLook4Players
  */
static boolean P_LookForPlayers(mobj_t *actor, boolean allaround, boolean tracer)
{
	int c = 0, stop;
	player_t *player;
	sector_t *sector;
	angle_t an;
	fixed_t dist;

	if (cv_objectplace.value)
		return false;

	sector = actor->subsector->sector;

	// BP: first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_Random() % MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for (; ; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
	{
		// done looking
		if (actor->lastlook == stop)
			return false;

		if (!playeringame[actor->lastlook])
			continue;

		if (c++ == 2)
			return false;

		player = &players[actor->lastlook];

		if (player->health <= 0)
			continue; // dead

		if (!player->mo)
			continue;

		if (!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if (!allaround)
		{
			an = R_PointToAngle2(actor->x, actor->y, player->mo->x, player->mo->y) - actor->angle;
			if (an > ANG90 && an < ANG270)
			{
				dist = P_AproxDistance(player->mo->x - actor->x, player->mo->y - actor->y);
				// if real close, react anyway
				if (dist > MELEERANGE)
					continue; // behind back
			}
		}

		if (tracer)
			actor->tracer = player->mo;
		else
			actor->target = player->mo;
		return true;
	}

	//return false;
}

/** Looks for a player with a ring shield.
  * Used by rings.
  *
  * \param actor Ring looking for a shield to be attracted to.
  * \return True if a player with ring shield is found, otherwise false.
  * \sa A_AttractChase
  */
static boolean P_LookForShield(mobj_t *actor)
{
	int c = 0, stop;
	player_t *player;
	sector_t *sector;

	sector = actor->subsector->sector;

	// BP: first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_Random() % MAXPLAYERS;

	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for (; ; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
	{
		// done looking
		if (actor->lastlook == stop)
			return false;

		if (!playeringame[actor->lastlook])
			continue;

		if (c++ == 2)
			return false;

		player = &players[actor->lastlook];

		if (player->health <= 0)
			continue; // dead

		if (player->powers[pw_ringshield]
			&& ((P_AproxDistance(actor->x-player->mo->x, actor->y-player->mo->y) < RING_DIST
			&& abs(player->mo->z-actor->z) < RING_DIST) || RING_DIST == 0))
		{
			actor->target = player->mo;
			return true;
		}
		else
			continue;
	}

	//return false;
}

//
// ACTION ROUTINES
//

// Function: A_Look
//
// Description: Look for a player and set your target to them.
//
// var1 = look all around?
// var2 = If 1, only change to seestate. If 2, only play seesound. If 0, do both.
//
void A_Look(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (!P_LookForPlayers(actor, locvar1, false))
		return;

	// go into chase state
	if (!locvar2)
	{
		P_SetMobjState(actor, actor->info->seestate);
		A_PlaySeeSound(actor);
	}
	else if (locvar2 == 1) // Only go into seestate
		P_SetMobjState(actor, actor->info->seestate);
	else if (locvar2 == 2) // Only play seesound
		A_PlaySeeSound(actor);
}

// Function: A_Chase
//
// Description: Chase after your target.
//
// var1 = unused
// var2 = unused
//
void A_Chase(mobj_t *actor)
{
	int delta;

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANG90/2;
		else if (delta < 0)
			actor->angle += ANG90/2;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if (actor->info->meleestate && P_CheckMeleeRange(actor))
	{
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if (actor->info->missilestate)
	{
		if (actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if (multiplayer && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor))
		P_NewChaseDir(actor);
}

void A_SkimChase(mobj_t *actor)
{
	int delta;

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANG90/2;
		else if (delta < 0)
			actor->angle += ANG90/2;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		P_LookForPlayers(actor, true, false);

		// the spawnstate for skims already calls this function so just return either way
		// without changing state
		return;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	// check for melee attack
	if (actor->info->meleestate && P_CheckMeleeRange(actor))
	{
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		P_SetMobjState(actor, actor->info->meleestate);
		return;
	}

	// check for missile attack
	if (actor->info->missilestate)
	{
		if (actor->movecount || !P_CheckMissileRange(actor))
			goto nomissile;

		P_SetMobjState(actor, actor->info->missilestate);
		actor->flags2 |= MF2_JUSTATTACKED;
		return;
	}

nomissile:
	// possibly choose another target
	if (multiplayer && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target))
		&& P_LookForPlayers(actor, true, false))
		return; // got a new target

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor))
		P_NewChaseDir(actor);
}

// Function: A_FaceTarget
//
// Description: Immediately turn to face towards your target.
//
// var1 = unused
// var2 = unused
//
void A_FaceTarget(mobj_t *actor)
{
	if (!actor->target)
		return;

	actor->flags &= ~MF_AMBUSH;

	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
}

// Function: A_FireShot
//
// Description: Shoot an object at your target.
//
// var1 = object # to shoot
// var2 = height offset
//
void A_FireShot(mobj_t *actor)
{
	fixed_t x, y, z;
	int locvar1 = var1;
	int locvar2 = var2;

	if (!actor->target)
		return;

	A_FaceTarget(actor);

	if (P_Random() & 1)
	{
		x = actor->x + P_ReturnThrustX(actor, actor->angle-ANG90, 43*FRACUNIT);
		y = actor->y + P_ReturnThrustY(actor, actor->angle-ANG90, 43*FRACUNIT);
	}
	else
	{
		x = actor->x + P_ReturnThrustX(actor, actor->angle+ANG90, 43*FRACUNIT);
		y = actor->y + P_ReturnThrustY(actor, actor->angle+ANG90, 43*FRACUNIT);
	}

	z = actor->z + 48*FRACUNIT + locvar2*FRACUNIT;

	P_SpawnXYZMissile(actor, actor->target, locvar1, x, y, z);

	if (!(actor->flags & MF_BOSS))
	{
		if (gameskill <= sk_medium)
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE;
	}
}

// Function: A_SuperFireShot
//
// Description: Shoot an object at your target that will even stall Super Sonic.
//
// var1 = object # to shoot
// var2 = height offset
//
void A_SuperFireShot(mobj_t *actor)
{
	fixed_t x, y, z;
	mobj_t *mo;
	int locvar1 = var1;
	int locvar2 = var2;

	if (!actor->target)
		return;

	A_FaceTarget(actor);

	if (P_Random() & 1)
	{
		x = actor->x + P_ReturnThrustX(actor, actor->angle-ANG90, 43*FRACUNIT);
		y = actor->y + P_ReturnThrustY(actor, actor->angle-ANG90, 43*FRACUNIT);
	}
	else
	{
		x = actor->x + P_ReturnThrustX(actor, actor->angle+ANG90, 43*FRACUNIT);
		y = actor->y + P_ReturnThrustY(actor, actor->angle+ANG90, 43*FRACUNIT);
	}

	z = actor->z + 48*FRACUNIT + locvar2*FRACUNIT;

	mo = P_SpawnXYZMissile(actor, actor->target, locvar1, x, y, z);

	if (mo)
		mo->flags2 |= MF2_SUPERFIRE;

	if (!(actor->flags & MF_BOSS))
	{
		if (gameskill <= sk_medium)
			actor->reactiontime = actor->info->reactiontime*TICRATE*2;
		else
			actor->reactiontime = actor->info->reactiontime*TICRATE;
	}
}

// Function: A_SkullAttack
//
// Description: Fly at the player like a missile.
//
// var1:
//		0 - Fly at the player
//		1 - Fly away from the player
//		2 - Strafe in relation to the player
// var2 = unused
//
#define SKULLSPEED (20*FRACUNIT)

void A_SkullAttack(mobj_t *actor)
{
	mobj_t *dest;
	angle_t an;
	int dist;
	int speed;
	int locvar1 = var1;
	//int locvar2 = var2;

	if (!actor->target)
		return;

	speed = SKULLSPEED;

	dest = actor->target;
	actor->flags2 |= MF2_SKULLFLY;
	if (actor->info->activesound)
		S_StartSound(actor, actor->info->activesound);
	A_FaceTarget(actor);

	if (locvar1 == 1)
		actor->angle += ANG180;
	else if (locvar1 == 2)
	{
		if (P_Random() & 1)
			actor->angle += ANG90;
		else
			actor->angle -= ANG90;
	}

	an = actor->angle >> ANGLETOFINESHIFT;
	
	actor->momx = FixedMul(speed, finecosine[an]);
	actor->momy = FixedMul(speed, finesine[an]);
	dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
	dist = dist / speed;

	if (dist < 1)
		dist = 1;

	actor->momz = (dest->z + (dest->height>>1) - actor->z) / dist;

	if (locvar1 == 1)
		actor->momz = -actor->momz;
}

// Function: A_BossZoom
//
// Description: Like A_SkullAttack, but used by Boss 1.
//
// var1 = unused
// var2 = unused
//
void A_BossZoom(mobj_t *actor)
{
	mobj_t *dest;
	angle_t an;
	int dist;

	if (!actor->target)
		return;

	dest = actor->target;
	actor->flags2 |= MF2_SKULLFLY;
	if (actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
	A_FaceTarget(actor);
	an = actor->angle >> ANGLETOFINESHIFT;
	actor->momx = FixedMul(actor->info->speed*5*FRACUNIT, finecosine[an]);
	actor->momy = FixedMul(actor->info->speed*5*FRACUNIT, finesine[an]);
	dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
	dist = dist / (actor->info->speed*5*FRACUNIT);

	if (dist < 1)
		dist = 1;
	actor->momz = (dest->z + (dest->height>>1) - actor->z) / dist;
}

// Function: A_BossScream
//
// Description: Spawns explosions and plays appropriate sounds around the defeated boss.
//
// var1 = unused
// var2 = unused
//
void A_BossScream(mobj_t *actor)
{
	fixed_t x, y, z;
	angle_t fa;

	actor->movecount += actor->info->speed*16;
	actor->movecount %= 360;
	fa = FINEANGLE_C(actor->movecount);
	x = actor->x + FixedMul(finecosine[fa],actor->radius);
	y = actor->y + FixedMul(finesine[fa],actor->radius);

	z = actor->z - 8*FRACUNIT + (P_Random()<<(FRACBITS-2));
	if (actor->info->deathsound) S_StartSound(P_SpawnMobj(x, y, z, MT_BOSSEXPLODE), actor->info->deathsound);
}

// Function: A_Scream
//
// Description: Starts the death sound of the object.
//
// var1 = unused
// var2 = unused
//
void A_Scream(mobj_t *actor)
{
	if (actor->tracer && (actor->tracer->type == MT_SHELL || actor->tracer->type == MT_FIREBALL))
		S_StartScreamSound(actor, sfx_lose);
	else if (actor->info->deathsound)
		S_StartScreamSound(actor, actor->info->deathsound);
}

// Function: A_Pain
//
// Description: Starts the pain sound of the object.
//
// var1 = unused
// var2 = unused
//
void A_Pain(mobj_t *actor)
{
	if (actor->info->painsound)
		S_StartSound(actor, actor->info->painsound);
}

// Function: A_Fall
//
// Description: Changes a dying object's flags to reflect its having fallen to the ground.
//
// var1 = unused
// var2 = unused
//
void A_Fall(mobj_t *actor)
{
	// actor is on ground, it can be walked over
	actor->flags &= ~MF_SOLID;

	actor->flags |= MF_NOCLIP;
	actor->flags |= MF_NOGRAVITY;
	actor->flags |= MF_FLOAT;

	// So change this if corpse objects
	// are meant to be obstacles.
}

#define LIVESBOXDISPLAYPLAYER // Use displayplayer instead of closest player

// Function: A_1upThinker
//
// Description: Used by the 1up box to show the player's face.
//
// var1 = unused
// var2 = unused
//
void A_1upThinker(mobj_t *actor)
{
	#ifdef LIVESBOXDISPLAYPLAYER
	if (!cv_splitscreen.value)
		actor->frame = states[S_PRUPAUX1+(players[displayplayer].boxindex*3)-3].frame;
	else
	{
		int i;
		fixed_t dist = MAXINT;
		fixed_t temp;
		int closestplayer = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			if (!players[i].mo)
				continue;

			temp = P_AproxDistance(players[i].mo->x-actor->x, players[i].mo->y-actor->y);

			if (temp < dist)
			{
				closestplayer = i;
				dist = temp;
			}
		}

		if (players[closestplayer].boxindex != 0)
			P_SetMobjState(actor, S_PRUPAUX1+(players[closestplayer].boxindex*3)-3);
	}
	#else
	int i;
	fixed_t dist = MAXINT;
	fixed_t temp;
	int closestplayer = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		if (!players[i].mo)
			continue;

		temp = P_AproxDistance(players[i].mo->x-actor->x, players[i].mo->y-actor->y);

		if (temp < dist)
		{
			closestplayer = i;
			dist = temp;
		}
	}

	if (players[closestplayer].boxindex != 0)
		P_SetMobjState(actor, S_PRUPAUX1+(players[closestplayer].boxindex*3)-3);
	#endif
}

// Function: A_MonitorPop
//
// Description: Used by monitors when they explode.
//
// var1 = unused
// var2 = unused
//
void A_MonitorPop(mobj_t *actor)
{
	mobj_t *remains;
	mobjtype_t item;
	int prandom;
	mobjtype_t newbox;

	// de-solidify
	P_UnsetThingPosition(actor);
	actor->flags &= ~MF_SOLID;
	actor->flags |= MF_NOCLIP;
	P_SetThingPosition(actor);

	remains = P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->speed);
	remains->type = actor->type; // Transfer type information
	remains->flags = actor->flags; // Transfer flags
	remains->fuse = actor->fuse; // Transfer respawn timer
	remains->threshold = 68;
	actor->flags2 |= MF2_BOSSNOTRAP; // Dummy flag to mark this as an exploded TV until it respawns
	actor->tracer = remains;

	if (actor->info->deathsound) S_StartSound(remains, actor->info->deathsound);

	switch (actor->type)
	{
		case MT_QUESTIONBOX: // Random!
		{
			mobjtype_t spawnchance[30];
			int i = 0;
			int oldi = 0;
			int numchoices = 0;

			prandom = P_Random(); // Gotta love those random numbers!

			if (cv_superring.value)
			{
				oldi = i;
				for (; i < oldi + cv_superring.value; i++)
				{
					spawnchance[i] = MT_SUPERRINGBOX;
					numchoices++;
				}
			}
			if (cv_silverring.value)
			{
				oldi = i;
				for (; i < oldi + cv_silverring.value; i++)
				{
					spawnchance[i] = MT_GREYRINGBOX;
					numchoices++;
				}
			}
			if (cv_supersneakers.value)
			{
				oldi = i;
				for (; i < oldi + cv_supersneakers.value; i++)
				{
					spawnchance[i] = MT_SNEAKERTV;
					numchoices++;
				}
			}
			if (cv_invincibility.value)
			{
				oldi = i;
				for (; i < oldi + cv_invincibility.value; i++)
				{
					spawnchance[i] = MT_INV;
					numchoices++;
				}
			}
			if (cv_jumpshield.value)
			{
				oldi = i;
				for (; i < oldi + cv_jumpshield.value; i++)
				{
					spawnchance[i] = MT_WHITETV;
					numchoices++;
				}
			}
			if (cv_watershield.value)
			{
				oldi = i;
				for (; i < oldi + cv_watershield.value; i++)
				{
					spawnchance[i] = MT_BLUETV;
					numchoices++;
				}
			}
			if (cv_ringshield.value)
			{
				oldi = i;
				for (; i < oldi + cv_ringshield.value; i++)
				{
					spawnchance[i] = MT_YELLOWTV;
					numchoices++;
				}
			}
			if (cv_fireshield.value)
			{
				oldi = i;
				for (; i < oldi + cv_fireshield.value; i++)
				{
					spawnchance[i] = MT_REDTV;
					numchoices++;
				}
			}
			if (cv_bombshield.value)
			{
				oldi = i;
				for (; i < oldi + cv_bombshield.value; i++)
				{
					spawnchance[i] = MT_BLACKTV;
					numchoices++;
				}
			}
			if (cv_1up.value && gametype == GT_RACE)
			{
				oldi = i;
				for (; i < oldi + cv_1up.value; i++)
				{
					spawnchance[i] = MT_PRUP;
					numchoices++;
				}
			}
			if (cv_eggmanbox.value)
			{
				oldi = i;
				for (; i < oldi + cv_eggmanbox.value; i++)
				{
					spawnchance[i] = MT_EGGMANBOX;
					numchoices++;
				}
			}
			if (cv_teleporters.value)
			{
				oldi = i;
				for (; i < oldi + cv_teleporters.value; i++)
				{
					spawnchance[i] = MT_MIXUPBOX;
					numchoices++;
				}
			}

			if (numchoices == 0)
			{
				CONS_Printf("Note: All monitors turned off.\n");
				return;
			}

			newbox = spawnchance[prandom%numchoices];
			item = mobjinfo[newbox].damage;

			remains->flags &= ~MF_AMBUSH;
			break;
		}
		default:
			item = actor->info->damage;
			break;
	}

	if (item != 0)
	{
		mobj_t *newmobj;
		newmobj = P_SpawnMobj(actor->x, actor->y, actor->z + 13*FRACUNIT, item);
		newmobj->target = actor->target; // Transfer target

		if (item == MT_1UPICO && newmobj->target->player)
		{
			if (newmobj->target->player->boxindex != 0)
			{
				P_SetMobjState(newmobj, S_PRUPAUX2+(newmobj->target->player->boxindex*3)-3);
			}
		}
	}
	else
		CONS_Printf("Powerup item not defined in 'damage' field for A_MonitorPop\n");

	P_SetMobjState(actor, S_DISS);
}

// Function: A_Explode
//
// Description: Explodes an object, doing damage to any objects nearby. The target is used as the cause of the explosion. Damage value is used as amount of damage to be dealt.
//
// var1 = unused
// var2 = unused
//
void A_Explode(mobj_t *actor)
{
	P_RadiusAttack(actor, actor->target, actor->info->damage);
}

// Function: A_BossDeath
//
// Description: Possibly trigger special effects when boss dies.
//
// var1 = unused
// var2 = unused
//
void A_BossDeath(mobj_t *mo)
{
	thinker_t *th;
	mobj_t *mo2;
	line_t junk;
	int i;

	if (mo->type == MT_EGGMOBILE || mo->type == MT_EGGMOBILE2)
	{
		if (mo->flags2 & MF2_CHAOSBOSS)
		{
			mo->health = 0;
			P_SetMobjState(mo, S_DISS);
			return;
		}
	}

	mo->health = 0;

	// make sure there is a player alive for victory
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && (players[i].health > 0
			|| ((netgame || multiplayer) && (players[i].lives > 0 || players[i].continues > 0))))
			break;

	if (i == MAXPLAYERS)
		return; // no one left alive, so do not end game

	// scan the remaining thinkers to see
	// if all bosses are dead
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;
		if (mo2 != mo && mo2->type == mo->type && mo2->health > 0)
			return; // other boss not dead
	}

	// victory!
	if (!mariomode)
	{
		if (mo->flags2 & MF2_BOSSNOTRAP)
		{
			for (i = 0; i < MAXPLAYERS; i++)
				P_DoPlayerExit(&players[i]);
		}
		else
		{
			// Bring the egg trap up to the surface
			junk.tag = 680;
			EV_DoElevator(&junk, elevateHighest, false);
			junk.tag = 681;
			EV_DoElevator(&junk, elevateUp, false);
			junk.tag = 682;
			EV_DoElevator(&junk, elevateHighest, false);
		}

		// Stop exploding and prepare to run.
		P_SetMobjState(mo, mo->info->xdeathstate);

		mo->target = NULL;

		// Flee! Flee! Find a point to escape to! If none, just shoot upward!
		// scan the thinkers to find the runaway point
		for (th = thinkercap.next; th != &thinkercap; th = th->next)
		{
			if (th->function.acp1 != (actionf_p1)P_MobjThinker)
				continue;

			mo2 = (mobj_t *)th;

			if (mo2->type == MT_BOSSFLYPOINT)
			{
				// If this one's closer then the last one, go for it.
				if (!mo->target ||
					P_AproxDistance(P_AproxDistance(mo->x - mo2->x, mo->y - mo2->y), mo->z - mo2->z) <
					P_AproxDistance(P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y), mo->z - mo->target->z))
						mo->target = mo2;
				// Otherwise... Don't!
			}
		}

		mo->flags |= MF_NOGRAVITY|MF_NOCLIP;

		if (mo->target)
		{
			if (mo->z < mo->floorz + 64*FRACUNIT)
				mo->momz = 2*FRACUNIT;
			mo->angle = R_PointToAngle2(mo->x, mo->y, mo->target->x, mo->target->y);
			mo->flags2 |= MF2_BOSSFLEE;
			mo->momz = FixedMul(FixedDiv(mo->target->z - mo->z, P_AproxDistance(mo->x-mo->target->x,mo->y-mo->target->y)), 2*FRACUNIT);
		}
		else
			mo->momz = 2*FRACUNIT;

		if (mo->type == MT_EGGMOBILE2)
		{
			mo2 = P_SpawnMobj(mo->x + P_ReturnThrustX(mo, mo->angle - ANG90, 32*FRACUNIT),
				mo->y + P_ReturnThrustY(mo, mo->angle-ANG90, 24*FRACUNIT),
				mo->z + mo->height/2 - 8*FRACUNIT, MT_BOSSTANK1); // Right tank
			mo2->angle = mo->angle;
			P_InstaThrust(mo2, mo2->angle - ANG90, 4*FRACUNIT);
			mo2->momz = 4*FRACUNIT;

			mo2 = P_SpawnMobj(mo->x + P_ReturnThrustX(mo, mo->angle + ANG90, 32*FRACUNIT),
				mo->y + P_ReturnThrustY(mo, mo->angle-ANG90, 24*FRACUNIT),
				mo->z + mo->height/2 - 8*FRACUNIT, MT_BOSSTANK2); // Left tank
			mo2->angle = mo->angle;
			P_InstaThrust(mo2, mo2->angle + ANG90, 4*FRACUNIT);
			mo2->momz = 4*FRACUNIT;

			P_SpawnMobj(mo->x, mo->y, mo->z + mo->height + 32*FRACUNIT, MT_BOSSSPIGOT)->momz = 4*FRACUNIT;
			return;
		}
	}
	else if (mariomode && mo->type == MT_KOOPA)
	{
		junk.tag = 650;
		EV_DoCeiling(&junk, raiseToHighest);
		return;
	}
}

// Function: A_CustomPower
//
// Description: Provides a custom powerup. Target (must be a player) is awarded the powerup. Reactiontime of the object is used as an index to the powers array.
//
// var1 = unused
// var2 = unused
//
void A_CustomPower(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	if (actor->info->reactiontime >= NUMPOWERS)
	{
		CONS_Printf("Power #%d out of range!\n", actor->info->reactiontime);
		return;
	}

	player = actor->target->player;

	player->powers[actor->info->reactiontime] = actor->info->painchance;
	if (actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_JumpShield
//
// Description: Awards the player a jump shield.
//
// var1 = unused
// var2 = unused
//
void A_JumpShield(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	player->powers[pw_fireshield] = player->powers[pw_bombshield] = false;
	player->powers[pw_watershield] = player->powers[pw_ringshield] = false;

	if (!(player->powers[pw_jumpshield]))
	{
		player->powers[pw_jumpshield] = true;
		P_SpawnShieldOrb(player);
	}

	S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_RingShield
//
// Description: Awards the player a ring shield.
//
// var1 = unused
// var2 = unused
//
void A_RingShield(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	player->powers[pw_bombshield] = player->powers[pw_watershield] = false;
	player->powers[pw_fireshield] = player->powers[pw_jumpshield] = false;

	if (!(player->powers[pw_ringshield]))
	{
		player->powers[pw_ringshield] = true;
		P_SpawnShieldOrb(player);
	}

	S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_RingBox
//
// Description: Awards the player 10 rings.
//
// var1 = unused
// var2 = unused
//
void A_RingBox(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	P_GivePlayerRings(player, actor->info->reactiontime, false);
	if (actor->info->seesound)
		S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_Invincibility
//
// Description: Awards the player invincibility.
//
// var1 = unused
// var2 = unused
//
void A_Invincibility(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;
	player->powers[pw_invulnerability] = invulntics + 1;

	if (P_IsLocalPlayer(player) && !player->powers[pw_super])
	{
		S_StopMusic();
		if (mariomode)
			S_ChangeMusic(mus_minvnc, false);
		else
			S_ChangeMusic(mus_invinc, false);
	}
}

// Function: A_SuperSneakers
//
// Description: Awards the player super sneakers.
//
// var1 = unused
// var2 = unused
//
void A_SuperSneakers(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	actor->target->player->powers[pw_sneakers] = sneakertics + 1;

	if (P_IsLocalPlayer(player) && (!player->powers[pw_super] && !mapheaderinfo[gamemap-1].nossmusic))
	{
		S_StopMusic();
		S_ChangeMusic(mus_shoes, false);
	}
}

// Function: A_ExtraLife
//
// Description: Awards the player an extra life.
//
// var1 = unused
// var2 = unused
//
void A_ExtraLife(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	P_GivePlayerLives(player, 1);

	if (mariomode)
		S_StartSound(player->mo, sfx_marioa);
	else
	{
		player->powers[pw_extralife] = extralifetics + 1;

		if (P_IsLocalPlayer(player))
		{
			S_StopMusic();
			S_ChangeMusic(mus_xtlife, false);
		}
	}
}

// Function: A_BombShield
//
// Description: Awards the player a bomb shield.
//
// var1 = unused
// var2 = unused
//
void A_BombShield(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	player->powers[pw_watershield] = player->powers[pw_fireshield] = false;
	player->powers[pw_ringshield] = player->powers[pw_jumpshield] = false;

	if (!(player->powers[pw_bombshield]))
	{
		player->powers[pw_bombshield] = true;
		P_SpawnShieldOrb(player);
	}

	S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_WaterShield
//
// Description: Awards the player a water shield.
//
// var1 = unused
// var2 = unused
//
void A_WaterShield(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	player->powers[pw_bombshield] = player->powers[pw_fireshield] = false;
	player->powers[pw_ringshield] = player->powers[pw_jumpshield] = false;

	if (!(player->powers[pw_watershield]))
	{
		player->powers[pw_watershield] = true;
		P_SpawnShieldOrb(player);
	}

	if (P_IsLocalPlayer(player) && player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1)
	{
		player->powers[pw_underwater] = 0;
		P_RestoreMusic(player);
	}
	else
		player->powers[pw_underwater] = 0;

	if (player->powers[pw_spacetime] > 1)
	{
		player->powers[pw_spacetime] = 0;

		if (P_IsLocalPlayer(player))
			P_RestoreMusic(player);
	}
	S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_FireShield
//
// Description: Awards the player a fire shield.
//
// var1 = unused
// var2 = unused
//
void A_FireShield(mobj_t *actor)
{
	player_t *player;

	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	player = actor->target->player;

	player->powers[pw_bombshield] = player->powers[pw_watershield] = false;
	player->powers[pw_ringshield] = player->powers[pw_jumpshield] = false;

	if (!(player->powers[pw_fireshield]))
	{
		player->powers[pw_fireshield] = true;
		P_SpawnShieldOrb(player);
	}

	S_StartSound(player->mo, actor->info->seesound);
}

// Function: A_ScoreRise
//
// Description: Makes the little score logos rise. Speed value sets speed.
//
// var1 = unused
// var2 = unused
//
void A_ScoreRise(mobj_t *actor)
{
	actor->momz = actor->info->speed; // make logo rise!
}

// Function: A_BunnyHop
//
// Description: Makes object hop like a bunny.
//
// var1 = jump strength
// var2 = horizontal movement
//
void A_BunnyHop(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (actor->z <= actor->floorz)
	{
		actor->momz = locvar1*FRACUNIT; // make it hop!
		actor->angle += P_Random()*FINEANGLES;
		P_InstaThrust(actor, actor->angle, locvar2*FRACUNIT); // Launch the hopping action! PHOOM!!
	}
}

// Function: A_BubbleSpawn
//
// Description: Spawns a randomly sized bubble from the object's location. Only works underwater.
//
// var1 = unused
// var2 = unused
//
void A_BubbleSpawn(mobj_t *actor)
{
	byte prandom;
	if (!(actor->eflags & MF_UNDERWATER))
	{
		// Don't draw or spawn bubbles above water
		actor->flags2 |= MF2_DONTDRAW;
		return;
	}

	actor->flags2 &= ~MF2_DONTDRAW;
	prandom = P_Random();

	if (leveltime % (3*TICRATE) < 8)
		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_EXTRALARGEBUBBLE);
	else if (prandom > 128)
		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_SMALLBUBBLE);
	else if (prandom < 128 && prandom > 96)
		P_SpawnMobj(actor->x, actor->y, actor->z + (actor->height / 2), MT_MEDIUMBUBBLE);
}

// Function: A_BubbleRise
//
// Description: Raises a bubble
//
// var1:
//		0 = Bend around the water abit, looking more realistic
//		1 = Rise straight up
// var2 = rising speed
//
void A_BubbleRise(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (actor->type == MT_EXTRALARGEBUBBLE)
		actor->momz = (6*FRACUNIT)/5; // make bubbles rise!
	else
	{
		actor->momz += locvar2; // make bubbles rise!

		// Move around slightly to make it look like it's bending around the water

		if (!locvar1)
		{
			if (P_Random() < 32)
			{
				P_InstaThrust(actor, P_Random() & 1 ? actor->angle + ANG90 : actor->angle,
					P_Random() & 1? FRACUNIT/2 : -FRACUNIT/2);
			}
			else if (P_Random() < 32)
			{
				P_InstaThrust(actor, P_Random() & 1 ? actor->angle - ANG90 : actor->angle - ANG180,
					P_Random() & 1? FRACUNIT/2 : -FRACUNIT/2);
			}
		}
	}
}

// Function: A_BubbleCheck
//
// Description: Checks if a bubble should be drawn or not. Bubbles are not drawn above water.
//
// var1 = unused
// var2 = unused
//
void A_BubbleCheck(mobj_t *actor)
{
	if (actor->eflags & MF_UNDERWATER)
		actor->flags2 &= ~MF2_DONTDRAW; // underwater so draw
	else
		actor->flags2 |= MF2_DONTDRAW; // above water so don't draw
}

// Function: A_AttractChase
//
// Description: Makes a ring chase after a player with a ring shield and also causes spilled rings to flicker.
//
// var1 = unused
// var2 = unused
//
void A_AttractChase(mobj_t *actor)
{
	if (actor->flags2 & MF2_NIGHTSPULL)
		return;

	// spilled rings flicker before disappearing
	if (leveltime & 1 && actor->type == (mobjtype_t)actor->info->reactiontime && actor->fuse && actor->fuse < 2*TICRATE)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if (actor->target && actor->target->player
		&& !actor->target->player->powers[pw_ringshield] && actor->type != (mobjtype_t)actor->info->reactiontime)
	{
		mobj_t *newring;
		newring = P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->reactiontime);
		newring->flags |= MF_COUNTITEM;
		newring->momx = actor->momx;
		newring->momy = actor->momy;
		newring->momz = actor->momz;
		P_SetMobjState(actor, S_DISS);
	}

	P_LookForShield(actor); // Go find 'em, boy!

	actor->tracer = actor->target;

	if (!actor->target)
	{
		actor->target = actor->tracer = NULL;
		return;
	}

	if (!actor->target->player)
		return;

	if (!actor->target->health)
		return;

	// If a FlingRing gets attracted by a shield, change it into a normal
	// ring, but don't count towards the total.
	if (actor->type == (mobjtype_t)actor->info->reactiontime)
	{
		P_SetMobjState(actor, S_DISS);
		if (actor->flags & MF_COUNTITEM)
			P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->painchance);
		else
			P_SpawnMobj(actor->x, actor->y, actor->z, actor->info->painchance)
				->flags &= ~MF_COUNTITEM;
	}

	// Keep stuff from going down inside floors and junk
	actor->flags2 &= ~MF2_NOCLIPHEIGHT;

	P_Attract(actor, actor->tracer, false);
}

// Function: A_DropMine
//
// Description: Drops a mine. Raisestate specifies the object # to use for the mine.
//
// var1 = height offset
// var2:
//		lower 16 bits = proximity check distance (0 disables)
//		upper 16 bits = 0 to check proximity with target, 1 for tracer
//
void A_DropMine(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar2 & 65535)
	{
		fixed_t dist;
		mobj_t* target;

		if (locvar2 >> 16)
			target = actor->tracer;
		else
			target = actor->target;

		if (!target)
			return;

		dist = P_AproxDistance(actor->x-target->x, actor->y-target->y)>>FRACBITS;

		if (dist > (locvar2 & 65535))
			return;
	}

	// Use raisestate instead of MT_MINE
	P_SpawnMobj(actor->x, actor->y, actor->z - 12*FRACUNIT + (locvar1*FRACUNIT), actor->info->raisestate)
		->momz = actor->momz + actor->pmomz;
}

// Function: A_FishJump
//
// Description: Makes the stupid harmless fish in Greenflower Zone jump.
//
// var1 = unused
// var2 = unused
//
void A_FishJump(mobj_t *actor)
{
	if ((actor->z <= actor->floorz) || (actor->z <= actor->watertop - (64 << FRACBITS)))
	{
		if (actor->threshold == 0) actor->threshold = 44;
		actor->momz = actor->threshold*(FRACUNIT/4);
		P_SetMobjState(actor, actor->info->seestate);
	}

	if (actor->momz < 0)
		P_SetMobjState(actor, actor->info->meleestate);
}

// Function: A_SignPlayer
//
// Description: Changes the state of a level end sign to reflect the player who hit it.
//
// var1 = unused
// var2 = unused
//
void A_SignPlayer(mobj_t *actor)
{
	if (!actor->target)
		return;

	if (!actor->target->player)
		return;

	actor->state->nextstate = actor->info->seestate+actor->target->player->skin;
}

// Function:A_ThrownRing
//
// Description: Thinker for thrown rings/sparkle trail
//
// var1 = unused
// var2 = unused
//
void A_ThrownRing(mobj_t *actor)
{
	int c;
	int stop;
	player_t *player;
	sector_t *sector;

	if (leveltime % (TICRATE/7) == 0)
	{
		if (actor->flags2 & MF2_EXPLOSION)
			P_SpawnMobj(actor->x, actor->y, actor->z, MT_SMOK);
		else if (!(actor->flags2 & MF2_RAILRING))
			P_SpawnMobj(actor->x, actor->y, actor->z, MT_SPARK);
	}

	// spilled rings flicker before disappearing
	if (leveltime & 1 && actor->fuse > 0 && actor->fuse < 2*TICRATE)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if (actor->tracer && actor->tracer->health <= 0)
		actor->tracer = NULL;

	// Updated homing ring special capability
	// If you have a ring shield, all rings thrown
	// at you become homing (except rail)!

	// A non-homing ring getting attracted by a
	// magnetic player. If he gets too far away, make
	// sure to stop the attraction!
	if (actor->tracer && actor->tracer->player && actor->tracer->player->powers[pw_ringshield]
		&& !(actor->flags2 & MF2_HOMING)
		&& P_AproxDistance(P_AproxDistance(actor->tracer->x-actor->x,
		actor->tracer->y-actor->y), actor->tracer->z-actor->z) > RING_DIST/4)
	{
		actor->tracer = NULL;
	}

	if ((actor->tracer)
		&& ((actor->flags2 & MF2_HOMING) || actor->tracer->player->powers[pw_ringshield]))// Already found someone to follow.
	{
		int temp;

		temp = actor->threshold;
		actor->threshold = 32000;
		P_HomingAttack(actor, actor->tracer);
		actor->threshold = temp;
		return;
	}

	sector = actor->subsector->sector;

	// first time init, this allow minimum lastlook changes
	if (actor->lastlook < 0)
		actor->lastlook = P_Random () % MAXPLAYERS;

	c = 0;
	stop = (actor->lastlook - 1) & PLAYERSMASK;

	for (; ; actor->lastlook = (actor->lastlook + 1) & PLAYERSMASK)
	{
		// done looking
		if (actor->lastlook == stop)
			return;

		if (!playeringame[actor->lastlook])
			continue;

		if (c++ == 2)
			return;

		player = &players[actor->lastlook];

		if (!player->mo)
			continue;

		if (player->mo->health <= 0)
			continue; // dead

		if (gametype == GT_CTF && !player->ctfteam)
			continue; // spectator

		if (actor->target && actor->target->player)
		{
			if (player->mo == actor->target)
				continue;

			// Don't home in on teammates.
			if (gametype == GT_CTF 
				&& actor->target->player->ctfteam == player->ctfteam)
				continue;
		}

		// check distance
		if (actor->flags2 & MF2_RAILRING)
		{
			if (P_AproxDistance(P_AproxDistance(player->mo->x-actor->x,
				player->mo->y-actor->y), player->mo->z-actor->z) > RING_DIST/2)
			{
				continue;
			}
		}
		else if (P_AproxDistance(P_AproxDistance(player->mo->x-actor->x,
			player->mo->y-actor->y), player->mo->z-actor->z) > RING_DIST)
		{
			continue;
		}

		// do this after distance check because it's more computationally expensive
		if (!P_CheckSight(actor, player->mo))
			continue; // out of sight

		if ((actor->flags2 & MF2_HOMING) || (player->powers[pw_ringshield] == true
		 && P_AproxDistance(P_AproxDistance(player->mo->x-actor->x,
		 player->mo->y-actor->y), player->mo->z-actor->z) < RING_DIST/4))
			actor->tracer = player->mo;
		return;
	}

	return;
}

// Function: A_SetSolidSteam
//
// Description: Makes steam solid so it collides with the player to boost them.
//
// var1 = unused
// var2 = unused
//
void A_SetSolidSteam(mobj_t *actor)
{
	actor->flags &= ~MF_NOCLIP;
	actor->flags |= MF_SOLID;
	if (!(P_Random() % 8))
	{
		if (actor->info->deathsound)
			S_StartSound(actor, actor->info->deathsound); // Hiss!
	}
	else
	{
		if (actor->info->painsound)
			S_StartSound(actor, actor->info->painsound);
	}
	actor->momz++;
}

// Function: A_UnsetSolidSteam
//
// Description: Makes an object non-solid and also noclip. Used by the steam.
//
// var1 = unused
// var2 = unused
//
void A_UnsetSolidSteam(mobj_t *actor)
{
	actor->flags &= ~MF_SOLID;
	actor->flags |= MF_NOCLIP;
}

// Function: A_JetChase
//
// Description: A_Chase for Jettysyns
//
// var1 = unused
// var2 = unused
//
void A_JetChase(mobj_t *actor)
{
	fixed_t thefloor;

	if (actor->flags & MF_AMBUSH)
		return;

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->reactiontime)
		actor->reactiontime--;

	if (P_Random() % 32 == 1)
	{
		actor->momx = actor->momx / 2;
		actor->momy = actor->momy / 2;
		actor->momz = actor->momz / 2;
	}

	// Bounce if too close to floor or ceiling -
	// ideal for Jetty-Syns above you on 3d floors
	if (actor->momz && ((actor->z - (32<<FRACBITS)) < thefloor) && !((thefloor + 32*FRACUNIT + actor->height) > actor->ceilingz))
		actor->momz = -actor->momz/2;

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if ((multiplayer || netgame) && !actor->threshold && (actor->target->health <= 0 || !P_CheckSight(actor, actor->target)))
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

	// If the player is over 3072 fracunits away, then look for another player
	if (P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y),
		actor->target->z - actor->z) > 3072*FRACUNIT && P_LookForPlayers(actor, true, false))
	{
		return; // got a new target
	}

	// chase towards player
	if (gameskill <= sk_easy)
		P_Thrust(actor, actor->angle, actor->info->speed/6);
	else if (gameskill < sk_hard)
		P_Thrust(actor, actor->angle, actor->info->speed/4);
	else
		P_Thrust(actor, actor->angle, actor->info->speed/2);

	// must adjust height
	if (gameskill <= sk_medium)
	{
		if (actor->z < (actor->target->z + actor->target->height + (32<<FRACBITS)))
			actor->momz += FRACUNIT/2;
		else
			actor->momz -= FRACUNIT/2;
	}
	else
	{
		if (actor->z < (actor->target->z + actor->target->height + (64<<FRACBITS)))
			actor->momz += FRACUNIT/2;
		else
			actor->momz -= FRACUNIT/2;
	}
}

// Function: A_JetbThink
//
// Description: Thinker for Jetty-Syn bombers
//
// var1 = unused
// var2 = unused
//
void A_JetbThink(mobj_t *actor)
{
	sector_t *nextsector;

	fixed_t thefloor;

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->target)
	{
		A_JetChase (actor);
		// check for melee attack
		if ((actor->z > (actor->floorz + (32<<FRACBITS)))
			&& P_CheckMeleeRange (actor) && !actor->reactiontime
			&& (actor->target->z >= actor->floorz))
		{
			if (actor->info->attacksound)
				S_StartAttackSound(actor, actor->info->attacksound);

			// use raisestate instead of MT_MINE
			P_SpawnMobj(actor->x, actor->y, actor->z - (32<<FRACBITS), actor->info->raisestate)->target = actor;
			actor->reactiontime = TICRATE; // one second
		}
	}
	else if (((actor->z - (32<<FRACBITS)) < thefloor) && !((thefloor + (32<<FRACBITS) + actor->height) > actor->ceilingz))
			actor->z = thefloor+(32<<FRACBITS);

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
    {
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if (nextsector->ceilingheight < actor->height)
		actor->momz -= 5*FRACUNIT;
	else if (nextsector->floorheight > actor->z)
		actor->momz += 5*FRACUNIT;
}

// Function: A_JetgShoot
//
// Description: Firing function for Jetty-Syn gunners.
//
// var1 = unused
// var2 = unused
//
void A_JetgShoot(mobj_t *actor)
{
	fixed_t dist;

	if (!actor->target)
		return;

	if (actor->reactiontime)
		return;

	dist = P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y);

	if (dist > actor->info->painchance*FRACUNIT)
		return;

	if (dist < 64*FRACUNIT)
		return;

	A_FaceTarget(actor);
	P_SpawnMissile(actor, actor->target, actor->info->raisestate);

	if (gameskill <= sk_medium)
		actor->reactiontime = actor->info->reactiontime*TICRATE*2;
	else
		actor->reactiontime = actor->info->reactiontime*TICRATE;
	
	if (actor->info->attacksound)
		S_StartSound(actor, actor->info->attacksound);
}

// Function: A_ShootBullet
//
// Description: Shoots a bullet. Raisestate defines object # to use as projectile.
//
// var1 = unused
// var2 = unused
//
void A_ShootBullet(mobj_t *actor)
{
	fixed_t dist;

	if (!actor->target)
		return;

	dist = P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y), actor->target->z - actor->z);

	if (dist > actor->info->painchance*FRACUNIT)
		return;

	A_FaceTarget(actor);
	P_SpawnMissile(actor, actor->target, actor->info->raisestate);

	if (actor->info->attacksound)
		S_StartSound(actor, actor->info->attacksound);
}

// Function: A_JetgThink
//
// Description: Thinker for Jetty-Syn Gunners
//
// var1 = unused
// var2 = unused
//
void A_JetgThink(mobj_t *actor)
{
	sector_t *nextsector;

	fixed_t thefloor;

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->target)
	{
		if (P_Random() <= 32 && !actor->reactiontime)
			P_SetMobjState(actor, actor->info->missilestate);
		else
			A_JetChase (actor);
	}
	else if (actor->z - (32<<FRACBITS) < thefloor && !(thefloor + (32<<FRACBITS)
		+ actor->height > actor->ceilingz))
	{
		actor->z = thefloor + (32<<FRACBITS);
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if (nextsector->ceilingheight < actor->height)
		actor->momz -= 5*FRACUNIT;
	else if (nextsector->floorheight > actor->z)
		actor->momz += 5*FRACUNIT;
}

// Function: A_MouseThink
//
// Description: Thinker for scurrying mice.
//
// var1 = unused
// var2 = unused
//
void A_MouseThink(mobj_t *actor)
{
	if (actor->reactiontime)
		actor->reactiontime--;

	if (actor->z == actor->floorz && !actor->reactiontime)
	{
		if (P_Random() & 1)
			actor->angle += ANG90;
		else
			actor->angle -= ANG90;

		P_InstaThrust(actor, actor->angle, actor->info->speed);
		actor->reactiontime = TICRATE/5;
	}
}

// Function: A_DetonChase
//
// Description: Chases a Deton after a player.
//
// var1 = unused
// var2 = unused
//
void A_DetonChase(mobj_t *actor)
{
	angle_t exact;
	fixed_t xydist, dist;
	mobj_t *oldtracer;

	oldtracer = actor->tracer;

	// modify tracer threshold
	if (!actor->tracer || actor->tracer->health <= 0)
		actor->threshold = 0;
	else
		actor->threshold = 1;

	if (!actor->tracer || !(actor->tracer->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, true))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	if (multiplayer && !actor->threshold && P_LookForPlayers(actor, true, true))
		return; // got a new target

	// Face movement direction if not doing so
	exact = R_PointToAngle2(actor->x, actor->y, actor->tracer->x, actor->tracer->y);
	actor->angle = exact;
	if (exact != actor->angle)
	{
		if (exact - actor->angle > ANG180)
		{
			actor->angle -= actor->info->raisestate;
			if (exact - actor->angle < ANG180)
				actor->angle = exact;
		}
		else
		{
			actor->angle += actor->info->raisestate;
			if (exact - actor->angle > ANG180)
				actor->angle = exact;
		}
	}
	// movedir is up/down angle: how much it has to go up as it goes over to the player
	xydist = P_AproxDistance(actor->tracer->x - actor->x, actor->tracer->y - actor->y);
	exact = R_PointToAngle2(actor->x, actor->z, actor->x + xydist, actor->tracer->z);
	actor->movedir = exact;
	if (exact != actor->movedir)
	{
		if (exact - actor->movedir > ANG180)
		{
			actor->movedir -= actor->info->raisestate;
			if (exact - actor->movedir < ANG180)
				actor->movedir = exact;
		}
		else
		{
			actor->movedir += actor->info->raisestate;
			if (exact - actor->movedir > ANG180)
				actor->movedir = exact;
		}
	}

	// check for melee attack
	if (actor->tracer)
	{
		if (P_AproxDistance(actor->tracer->x-actor->x, actor->tracer->y-actor->y) < actor->radius+actor->tracer->radius)
		{
			if (!((actor->tracer->z > actor->z + actor->height) || (actor->z > actor->tracer->z + actor->tracer->height)))
			{
				P_ExplodeMissile(actor);
				P_RadiusAttack(actor, actor, 96);
				return;
			}
		}
	}

	// chase towards player
	if ((dist = P_AproxDistance(xydist, actor->tracer->z-actor->z))
		> (actor->info->painchance << FRACBITS))
	{
		actor->tracer = NULL; // Too far away
		return;
	}

	if (actor->reactiontime == 0)
	{
		actor->reactiontime = actor->info->reactiontime;
		return;
	}

	if (actor->reactiontime > 1)
	{
		actor->reactiontime--;
		return;
	}

	if (actor->reactiontime > 0)
	{
		actor->reactiontime = -42;

		if (actor->info->seesound)
			S_StartScreamSound(actor, actor->info->seesound);
	}

	if (actor->reactiontime == -42)
	{
		fixed_t xyspeed;

		actor->reactiontime = -42;
		actor->tracer = actor->tracer;

		exact = actor->movedir>>ANGLETOFINESHIFT;
		xyspeed = FixedMul(actor->tracer->player->normalspeed*3*(FRACUNIT/4), finecosine[exact]);
		actor->momz = FixedMul(actor->tracer->player->normalspeed*3*(FRACUNIT/4), finesine[exact]);

		exact = actor->angle>>ANGLETOFINESHIFT;
		actor->momx = FixedMul(xyspeed, finecosine[exact]);
		actor->momy = FixedMul(xyspeed, finesine[exact]);

		// Variable re-use
		xyspeed = (P_AproxDistance(actor->tracer->x - actor->x, P_AproxDistance(actor->tracer->y - actor->y, actor->tracer->z - actor->z))>>(FRACBITS+6));

		if (xyspeed < 1)
			xyspeed = 1;

		if (leveltime % xyspeed == 0)
			S_StartSound(actor, sfx_deton);
	}
}

// Function: A_CapeChase
//
// Description: Set an object's location to its target or tracer.
//
// var1:
//		0 = Use target
//		1 = Use tracer
//		upper 16 bits = Z offset
// var2:
//		upper 16 bits = forward/backward offset
//		lower 16 bits = sideways offset
//
void A_CapeChase(mobj_t *actor)
{
	mobj_t *chaser;
	fixed_t offsetx, offsety;
	int locvar1 = var1;
	int locvar2 = var2;

	if(cv_debug)
		CONS_Printf("A_CapeChase called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	if (locvar1 & 65535)
		chaser = actor->tracer;
	else
		chaser = actor->target;

	if (actor->state == &states[S_DISS])
		return;

	if (!chaser || (chaser->health <= 0))
	{
		if(chaser && cv_debug)
			CONS_Printf("Hmm, the guy I'm chasing (object type %d) has no health.. so I'll die too!\n", chaser->type);

		P_SetMobjState(actor, S_DISS);
		return;
	}

	offsetx = P_ReturnThrustX(chaser, chaser->angle, (locvar2 >> 16)*FRACUNIT);
	offsety = P_ReturnThrustY(chaser, chaser->angle, (locvar2 & 65535)*FRACUNIT);

	P_UnsetThingPosition (actor);
	actor->x = chaser->x + offsetx;
	actor->y = chaser->y + offsety;
	actor->z = chaser->z + ((locvar1 >> 16)*FRACUNIT);
	actor->angle = chaser->angle;
	P_SetThingPosition (actor);
}

// Function: A_RotateSpikeBall
//
// Description: Rotates a spike ball around its target.
//
// var1 = unused
// var2 = unused
//
void A_RotateSpikeBall(mobj_t *actor)
{
	const fixed_t radius = 12*actor->info->speed;

	if (actor->type == MT_SPECIALSPIKEBALL)
		return;

	if (!actor->target) // This should NEVER happen.
	{
		if(cv_debug)
			CONS_Printf("Error: Spikeball has no target\n");
		P_SetMobjState(actor, S_DISS);
		return;
	}

	if (!actor->info->speed)
	{
		CONS_Printf("Error: A_RotateSpikeBall: Object has no speed.\n");
		return;
	}

	actor->angle += FixedAngle(actor->info->speed);
	P_UnsetThingPosition(actor);
	{
		const angle_t fa = actor->angle>>ANGLETOFINESHIFT;
		actor->x = actor->target->x + FixedMul(finecosine[fa],radius);
		actor->y = actor->target->y + FixedMul(finesine[fa],radius);
		actor->z = actor->target->z + actor->target->height/2;
		P_SetThingPosition(actor);
	}
}

// Function: A_SnowBall
//
// Description: Moves an object like A_MoveAbsolute in its current direction, using its Speed variable for the speed. Also sets a timer for the object to disappear.
//
// var1 = duration before disappearing (in seconds).
// var2 = unused
//
void A_SnowBall(mobj_t *actor)
{
	int locvar1 = var1;
	//int locvar2 = var2;

	P_InstaThrust(actor, actor->angle, actor->info->speed);
	if (!actor->fuse)
		actor->fuse = locvar1*TICRATE;
}

// Function: A_CrawlaCommanderThink
//
// Description: Thinker for Crawla Commander.
//
// var1 = shoot bullets?
// var2 = "pogo mode" speed
//
void A_CrawlaCommanderThink(mobj_t *actor)
{
	fixed_t dist;
	sector_t *nextsector;
	fixed_t thefloor;
	int locvar1 = var1;
	int locvar2 = var2;

	if (actor->z >= actor->waterbottom && actor->watertop > actor->floorz
		&& actor->z > actor->watertop - 256*FRACUNIT)
		thefloor = actor->watertop;
	else
		thefloor = actor->floorz;

	if (actor->fuse & 1)
		actor->flags2 |= MF2_DONTDRAW;
	else
		actor->flags2 &= ~MF2_DONTDRAW;

	if (actor->reactiontime > 0)
		actor->reactiontime--;

	if (actor->fuse < 2)
	{
		actor->fuse = 0;
		actor->flags2 &= ~MF2_FRET;
	}

	// Hover mode
	if (actor->health > 1 || actor->fuse)
	{
		if (actor->z < thefloor + (16*FRACUNIT))
			actor->momz += FRACUNIT;
		else if (actor->z < thefloor + (32*FRACUNIT))
			actor->momz += FRACUNIT/2;
		else
			actor->momz += 16;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		if (actor->state != &states[actor->info->spawnstate])
			P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	dist = P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y);

	if (actor->target->player && actor->health > 1)
	{
		if (dist < 128*FRACUNIT
			&& (actor->target->player->mfjumped || actor->target->player->mfspinning))
		{
			// Auugh! He's trying to kill you! Strafe! STRAAAAFFEEE!!
			if (actor->target->momx || actor->target->momy)
			{
				P_InstaThrust(actor, actor->angle-ANG180, 20*FRACUNIT);
			}
			return;
		}
	}

	if (locvar1)
	{
		if (actor->health < 2 && P_Random() < 2)
		{
			P_SpawnMissile (actor, actor->target, locvar1);
		}
	}

	// Face the player
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if (actor->threshold && dist > 256*FRACUNIT)
		actor->momx = actor->momy = 0;

	if (actor->reactiontime && actor->reactiontime <= 2*TICRATE && dist > actor->target->radius - FRACUNIT)
	{
		actor->threshold = 0;

		// Roam around, somewhat in the player's direction.
		actor->angle += (P_Random()<<10);
		actor->angle -= (P_Random()<<10);

		if (actor->health > 1)
			P_InstaThrust(actor, actor->angle, 10*FRACUNIT);
	}
	else if (!actor->reactiontime)
	{
		if (actor->health > 1) // Hover Mode
		{
			if (dist < 512*FRACUNIT)
			{
				actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
				P_InstaThrust(actor, actor->angle, 60*FRACUNIT);
				actor->threshold = 1;
			}
		}
		actor->reactiontime = 2*TICRATE + P_Random()/2;
	}

	if (actor->health == 1)
		P_Thrust(actor, actor->angle, 1);

	// Pogo Mode
	if (!actor->fuse && actor->health == 1 && actor->z <= actor->floorz)
	{
		if (dist < 256*FRACUNIT)
		{
			actor->momz = locvar2;
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
			P_InstaThrust(actor, actor->angle, locvar2/8);
			// pogo on player
		}
		else
		{
			byte prandom = P_Random();
			actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_Random() & 1 ? -prandom : +prandom);
			P_InstaThrust(actor, actor->angle, locvar2/3*2);
			actor->momz = locvar2; // Bounce up in air
		}
	}

	nextsector = R_PointInSubsector(actor->x + actor->momx, actor->y + actor->momy)->sector;

	// Move downwards or upwards to go through a passageway.
	if (nextsector->floorheight > actor->z && nextsector->floorheight - actor->z < 128*FRACUNIT)
		actor->momz += (nextsector->floorheight - actor->z) / 4;
}

// Function: A_RingExplode
//
// Description: Spurt out rings in many directions
//
// var1 = object # to explode as debris
// var2 = unused
//
void A_RingExplode(mobj_t *actor)
{
	int i;
	mobj_t *mo;
	const fixed_t ns = 20 * FRACUNIT;
	int locvar1 = var1;
	//int locvar2 = var2;
	boolean changecolor = (gametype == GT_CTF && actor->target && actor->target->player
			&& actor->target->player->ctfteam == 2);

	for (i = 0; i < 32; i++)
	{
		const angle_t fa = (i*FINEANGLES/16) & FINEMASK;

		mo = P_SpawnMobj(actor->x, actor->y, actor->z, locvar1);
		mo->target = actor->target; // Transfer target so player gets the points

		mo->momx = FixedMul(finesine[fa],ns);
		mo->momy = FixedMul(finecosine[fa],ns);

		if (i > 15)
		{
			if (i & 1)
				mo->momz = ns;
			else
				mo->momz = -ns;
		}

		mo->flags2 |= MF2_DEBRIS;
		mo->fuse = TICRATE/(OLDTICRATE/5);

		if (changecolor)
			mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);
	}

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, locvar1);

	mo->target = actor->target;
	mo->momz = ns;
	mo->flags2 |= MF2_DEBRIS;
	mo->fuse = TICRATE/(OLDTICRATE/5);

	if (changecolor)
		mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);

	mo = P_SpawnMobj(actor->x, actor->y, actor->z, locvar1);

	mo->target = actor->target;
	mo->momz = -ns;
	mo->flags2 |= MF2_DEBRIS;
	mo->fuse = TICRATE/(OLDTICRATE/5);

	if (changecolor)
		mo->flags = (mo->flags & ~MF_TRANSLATION) | (8<<MF_TRANSSHIFT);

	return;
}

// Function: A_MixUp
//
// Description: Mix up all of the player positions.
//
// var1 = unused
// var2 = unused
//
void A_MixUp(mobj_t *actor)
{
	boolean teleported[MAXPLAYERS];
	int i, numplayers = 0, prandom = 0;

	actor = NULL;
	if (!multiplayer)
		return;

	numplayers = 0;
	memset(teleported, 0, sizeof (teleported));

	// Count the number of players in the game
	// and grab their xyz coords
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] && players[i].mo && players[i].mo->health > 0 && players[i].playerstate == PST_LIVE
			&& !players[i].exiting)
		{
			if (gametype == GT_CTF && !players[i].ctfteam) // Ignore spectators
				continue;

			numplayers++;
		}

	if (numplayers <= 1) // Not enough players to mix up.
		return;
	else if (numplayers == 2) // Special case -- simple swap
	{
		fixed_t x, y, z;
		angle_t angle;
		int one = -1, two = 0; // default value 0 to make the compiler shut up
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && players[i].mo && players[i].mo->health > 0 && players[i].playerstate == PST_LIVE
				&& !players[i].exiting)
			{
				if (gametype == GT_CTF && !players[i].ctfteam) // Ignore spectators
					continue;

				if (one == -1)
					one = i;
				else
				{
					two = i;
					break;
				}
			}

		x = players[one].mo->x;
		y = players[one].mo->y;
		z = players[one].mo->z;
		angle = players[one].mo->angle;

		P_MixUp(players[one].mo, players[two].mo->x, players[two].mo->y,
			players[two].mo->z, players[two].mo->angle);

		P_MixUp(players[two].mo, x, y, z, angle);

		teleported[one] = true;
		teleported[two] = true;
	}
	else
	{
		fixed_t position[MAXPLAYERS][3];
		angle_t anglepos[MAXPLAYERS];
		boolean picked[MAXPLAYERS];
		int pindex[MAXPLAYERS], counter = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			position[i][0] = position[i][1] = position[i][2] = anglepos[i] = pindex[i] = -1;
			picked[i] = false;
			teleported[i] = false;
		}

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && players[i].mo->health > 0 && !players[i].exiting)
			{
				if (gametype == GT_CTF && !players[i].ctfteam) // Ignore spectators
					continue;

				position[counter][0] = players[i].mo->x;
				position[counter][1] = players[i].mo->y;
				position[counter][2] = players[i].mo->z;
				pindex[counter] = i;
				anglepos[counter] = players[i].mo->angle;
				players[i].mo->momx = players[i].mo->momy = players[i].mo->momz =
					players[i].rmomx = players[i].rmomy = 1;
				players[i].cmomx = players[i].cmomy = 0;
				counter++;
			}
		}

		counter = 0;

		// Mix them up!
		for (;;)
		{
			if (counter > 255) // fail-safe to avoid endless loop
				break;
			prandom = P_Random();
			if (!(prandom % numplayers)) // Make sure it's not a useless mix
				break;
			counter++;
		}

		// Scramble!
		prandom %= numplayers; // I love modular arithmetic, don't you?
		counter = prandom;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && players[i].mo->health > 0 && !players[i].exiting)
			{
				if (gametype == GT_CTF && !players[i].ctfteam) // Ignore spectators
					continue;

				prandom = 0;
				while ((pindex[counter] == i || picked[counter] == true)
					&& prandom < 255) // Failsafe
				{
					counter = P_Random() % numplayers;
					prandom++;
				}

				P_MixUp(players[i].mo, position[counter][0], position[counter][1],
					position[counter][2], anglepos[counter]);

				teleported[i] = true;

				picked[counter] = true;
			}
		}
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (teleported[i])
		{
			if (playeringame[i] && players[i].playerstate == PST_LIVE
				&& players[i].mo && players[i].mo->health > 0 && !players[i].exiting)
			{
				if (gametype == GT_CTF && !players[i].ctfteam) // Ignore spectators
					continue;

				P_SetThingPosition(players[i].mo);

				players[i].mo->floorz = players[i].mo->subsector->sector->floorheight;
				players[i].mo->ceilingz = players[i].mo->subsector->sector->ceilingheight;

				P_CheckPosition(players[i].mo, players[i].mo->x, players[i].mo->y);
			}
		}
	}

	// Play the 'bowrwoosh!' sound
	S_StartSound(NULL, sfx_mixup);
}

// Function A_PumaJump
//
// Description: Like A_FishJump, but for the pumas.
//
// var1 = unused
// var2 = unused
//
void A_PumaJump(mobj_t *actor)
{
	if ((actor->z <= actor->floorz) || (actor->z <= actor->watertop-(64<<FRACBITS)))
	{
		if (actor->threshold == 0) actor->threshold = 44;

		actor->momz = actor->threshold*(FRACUNIT/4);
	}

	if (actor->momz < 0
		&& (actor->state != &states[S_PUMA4] || actor->state != &states[S_PUMA5] || actor->state != &states[S_PUMA6]))
		P_SetMobjStateNF(actor, S_PUMA4);
	else if (actor->state != &states[S_PUMA1] || actor->state != &states[S_PUMA2] || actor->state != &states[S_PUMA3])
		P_SetMobjStateNF(actor, S_PUMA1);
}

// Function: A_Boss1Chase
//
// Description: Like A_Chase, but for Boss 1.
//
// var1 = unused
// var2 = unused
//
void A_Boss1Chase(mobj_t *actor)
{
	int delta;

	if (actor->reactiontime)
		actor->reactiontime--;

	if (actor->z < actor->floorz+33*FRACUNIT)
		actor->z = actor->floorz+33*FRACUNIT;

	// turn towards movement direction if not there yet
	if (actor->movedir < NUMDIRS)
	{
		actor->angle &= (7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANG90/2;
		else if (delta < 0)
			actor->angle += ANG90/2;
	}

	// do not attack twice in a row
	if (actor->flags2 & MF2_JUSTATTACKED)
	{
		actor->flags2 &= ~MF2_JUSTATTACKED;
		P_NewChaseDir(actor);
		return;
	}

	if (actor->movecount)
		goto nomissile;

	if (!P_CheckMissileRange(actor))
		goto nomissile;

	if (actor->reactiontime <= 0)
	{
		if (actor->health > actor->info->damage)
		{
			if (P_Random() & 1)
				P_SetMobjState(actor, actor->info->missilestate);
			else
				P_SetMobjState(actor, actor->info->meleestate);
		}
		else
			P_SetMobjState(actor, actor->info->raisestate);

		actor->flags2 |= MF2_JUSTATTACKED;
		actor->reactiontime = 2*TICRATE;
		return;
	}

	// ?
nomissile:
	// possibly choose another target
	if (multiplayer && P_Random() < 2)
	{
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target
	}

	// chase towards player
	if (--actor->movecount < 0 || !P_Move(actor))
		P_NewChaseDir(actor);
}

// Function: A_Boss2Chase
//
// Description: Really doesn't 'chase', but rather goes in a circle.
//
// var1 = unused
// var2 = unused
//
void A_Boss2Chase(mobj_t *actor)
{
	const fixed_t radius = 384*FRACUNIT;
	boolean reverse = false;
	int speedvar;

	if (actor->health <= 0)
		return;

	// When reactiontime hits zero, he will go the other way
	if (actor->reactiontime)
		actor->reactiontime--;

	if (actor->reactiontime <= 0)
	{
		reverse = true;
		actor->reactiontime = 2*TICRATE + P_Random();
	}

	actor->target = P_GetClosestAxis(actor);

	if (!actor->target) // This should NEVER happen.
	{
		CONS_Printf("Error: Boss2 has no target!\n");
		A_BossDeath(actor);
		return;
	}

	if (reverse)
		actor->watertop = -actor->watertop;

	// Only speed up if you have the 'Deaf' flag.
	if (actor->flags & MF_AMBUSH)
		speedvar = actor->health;
	else
		speedvar = actor->info->spawnhealth;

	actor->target->angle += FixedAngle(FixedMul(actor->watertop,(actor->info->spawnhealth*(FRACUNIT/4)*3)/speedvar)); // Don't use FixedAngleC?
	
	P_UnsetThingPosition(actor);
	{
		const angle_t fa = actor->target->angle>>ANGLETOFINESHIFT;
		const fixed_t fc = FixedMul(finecosine[fa],radius);
		const fixed_t fs = FixedMul(finesine[fa],radius);
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x + fc, actor->target->y + fs);
		actor->x = actor->target->x + fc;
		actor->y = actor->target->y + fs;
	}
	P_SetThingPosition(actor);

	// Spray goo once every second
	if (leveltime % (speedvar*15/10)-1 == 0)
	{
		const fixed_t ns = 3 * FRACUNIT;
		mobj_t *goop;
		fixed_t fz = actor->z+actor->height+56*FRACUNIT;
		angle_t fa;
		// actor->movedir is used to determine the last
		// direction goo was sprayed in. There are 8 possible
		// directions to spray. (45-degree increments)

		actor->movedir++;
		actor->movedir %= NUMDIRS;
		fa = (actor->movedir*FINEANGLES/8) & FINEMASK;

		goop = P_SpawnMobj(actor->x, actor->y, fz, actor->info->painchance);
		goop->momx = FixedMul(finesine[fa],ns);
		goop->momy = FixedMul(finecosine[fa],ns);
		goop->momz = 4*FRACUNIT;
		goop->fuse = 30*TICRATE+P_Random();

		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);

		if (P_Random() & 1)
		{
			goop->momx *= 2;
			goop->momy *= 2;
		}
		else if (P_Random() > 128)
		{
			goop->momx *= 3;
			goop->momy *= 3;
		}

		actor->flags2 |= MF2_JUSTATTACKED;
	}
}

// Function: A_Boss2Pogo
//
// Description: Pogo part of Boss 2 AI.
//
// var1 = unused
// var2 = unused
//
void A_Boss2Pogo(mobj_t *actor)
{
	if (actor->z <= actor->floorz + 8*FRACUNIT && actor->momz <= 0)
	{
		P_SetMobjState(actor, actor->info->raisestate);
		// Pogo Mode
	}
	else if (actor->momz < 0 && actor->reactiontime)
	{
		const fixed_t ns = 3 * FRACUNIT;
		mobj_t *goop;
		fixed_t fz = actor->z+actor->height+56*FRACUNIT;
		angle_t fa;
		int i;
		// spray in all 8 directions!
		for (i = 0; i < 8; i++)
		{
			actor->movedir++;
			actor->movedir %= NUMDIRS;
			fa = (actor->movedir*FINEANGLES/8) & FINEMASK;

			goop = P_SpawnMobj(actor->x, actor->y, fz, actor->info->painchance);
			goop->momx = FixedMul(finesine[fa],ns);
			goop->momy = FixedMul(finecosine[fa],ns);
			goop->momz = 4*FRACUNIT;


#ifdef CHAOSISNOTDEADYET
			if (gametype == GT_CHAOS)
				goop->fuse = 15*TICRATE;
			else
#endif
				goop->fuse = 30*TICRATE+P_Random();
		}
		actor->reactiontime = 0;
		if (actor->info->attacksound)
			S_StartAttackSound(actor, actor->info->attacksound);
		actor->flags2 |= MF2_JUSTATTACKED;
	}
}

// Function: A_Invinciblerize
//
// Description: Special function for Boss 2 so you can't just sit and destroy him.
//
// var1 = unused
// var2 = unused
//
void A_Invinciblerize(mobj_t *actor)
{
	A_Pain(actor);
	actor->reactiontime = 1;
	actor->movecount = TICRATE;
}

// Function: A_DeInvinciblerize
//
// Description: Does the opposite of A_Invinciblerize.
//
// var1 = unused
// var2 = unused
//
void A_DeInvinciblerize(mobj_t *actor)
{
	actor->movecount = actor->state->tics+TICRATE;
}

// Function: A_Boss2PogoSFX
//
// Description: Pogoing for Boss 2
//
// var1 = pogo jump strength
// var2 = horizontal pogoing speed multiple
//
void A_Boss2PogoSFX(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		return;
	}

	// Boing!
	if (P_AproxDistance(actor->x-actor->target->x, actor->y-actor->target->y) < 256*FRACUNIT)
	{
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);
		P_InstaThrust(actor, actor->angle, actor->info->speed);
		// pogo on player
	}
	else
	{
		byte prandom = P_Random();
		actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y) + (P_Random() & 1 ? -prandom : +prandom);
		P_InstaThrust(actor, actor->angle, FixedMul(actor->info->speed,(locvar2)));
	}
	if (actor->info->activesound) S_StartSound(actor, actor->info->activesound);
	actor->momz = locvar1; // Bounce up in air
	actor->reactiontime = 1;
}

// Function: A_EggmanBox
//
// Description: Harms the player
//
// var1 = unused
// var2 = unused
//
void A_EggmanBox(mobj_t *actor)
{
	if (!actor->target || !actor->target->player)
	{
		if(cv_debug)
			CONS_Printf("ERROR: Powerup has no target!\n");
		return;
	}

	P_DamageMobj(actor->target, actor, actor, 1); // Ow!
}

// Function: A_TurretFire
//
// Description: Initiates turret fire.
//
// var1 = object # to repeatedly fire
// var2 = distance threshold
//
void A_TurretFire(mobj_t *actor)
{
	int count = 0;
	fixed_t dist;
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar2)
		dist = locvar2*FRACUNIT;
	else
		dist = 2048*FRACUNIT;

	if (!locvar1)
		locvar1 = MT_TURRETLASER;

	while (P_SupermanLook4Players(actor) && count < MAXPLAYERS)
	{
		if (P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) < dist)
		{
			actor->flags2 |= MF2_FIRING;
			actor->eflags &= 65535;
			actor->eflags += (locvar1 << 16); // Upper 16 bits contains mobj #
			break;
		}

		count++;
	}
}

// Function: A_SuperTurretFire
//
// Description: Initiates turret fire that even stops Super Sonic.
//
// var1 = object # to repeatedly fire
// var2 = distance threshold
//
void A_SuperTurretFire(mobj_t *actor)
{
	int count = 0;
	fixed_t dist;
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar2)
		dist = locvar2*FRACUNIT;
	else
		dist = 2048*FRACUNIT;

	if (!locvar1)
		locvar1 = MT_TURRETLASER;

	while (P_SupermanLook4Players(actor) && count < MAXPLAYERS)
	{
		if (P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) < dist)
		{
			actor->flags2 |= MF2_FIRING;
			actor->flags2 |= MF2_SUPERFIRE;
			actor->eflags &= 65535;
			actor->eflags += (locvar1 << 16); // Upper 16 bits contains mobj #
			break;
		}

		count++;
	}
}

// Function: A_TurretStop
//
// Description: Stops the turret fire.
//
// var1 = Don't play activesound?
// var2 = unused
//
void A_TurretStop(mobj_t *actor)
{
	int locvar1 = var1;

	actor->flags2 &= ~MF2_FIRING;
	actor->flags2 &= ~MF2_SUPERFIRE;

	if (actor->target && actor->info->activesound && !locvar1)
		S_StartSound(actor, actor->info->activesound);
}

// Function: A_SparkFollow
//
// Description: Used by the hyper sparks to rotate around their target.
//
// var1 = unused
// var2 = unused
//
void A_SparkFollow(mobj_t *actor)
{
	if (actor->state == &states[S_DISS])
		return;

	if ((!actor->target || (actor->target->health <= 0))
		|| (actor->target->player && !actor->target->player->powers[pw_super]))
	{
		P_SetMobjState(actor, S_DISS);
		return;
	}

	actor->angle += FixedAngle(actor->info->damage*FRACUNIT);
	P_UnsetThingPosition(actor);
	{
		const angle_t fa = actor->angle>>ANGLETOFINESHIFT;
		actor->x = actor->target->x + FixedMul(finecosine[fa],actor->info->speed);
		actor->y = actor->target->y + FixedMul(finesine[fa],actor->info->speed);
		actor->z = actor->target->z + FixedDiv(actor->target->height,3*FRACUNIT) - actor->height;
	}
	P_SetThingPosition(actor);
}

// Function: A_BuzzFly
//
// Description: Makes an object slowly fly after a player, in the manner of a Buzz.
//
// var1 = unused
// var2 = unused
//
void A_BuzzFly(mobj_t *actor)
{
	if (actor->flags & MF_AMBUSH)
		return;

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
			actor->threshold = 0;
		else
			actor->threshold--;
	}

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers(actor, true, false))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate);
		return;
	}

	// turn towards movement direction if not there yet
	actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

	if (actor->target->health <= 0 || (!actor->threshold && !P_CheckSight(actor, actor->target)))
	{
		if ((multiplayer || netgame) && P_LookForPlayers(actor, true, false))
			return; // got a new target

		actor->momx = actor->momy = actor->momz = 0;
		P_SetMobjState(actor, actor->info->spawnstate); // Go back to looking around
		return;
	}

	// If the player is over 3072 fracunits away, then look for another player
	if (P_AproxDistance(P_AproxDistance(actor->target->x - actor->x, actor->target->y - actor->y),
		actor->target->z - actor->z) > 3072*FRACUNIT)
	{
		if (multiplayer || netgame)
			P_LookForPlayers(actor, true, false); // maybe get a new target

		return;
	}

	// chase towards player
	{
		int dist, realspeed;
		const fixed_t mf = 5*(FRACUNIT/4);

		if (gameskill <= sk_easy)
			realspeed = FixedDiv(actor->info->speed,mf);
		else if (gameskill < sk_hard)
			realspeed = actor->info->speed;
		else
			realspeed = FixedMul(actor->info->speed,mf);

		dist = P_AproxDistance(P_AproxDistance(actor->target->x - actor->x,
			actor->target->y - actor->y), actor->target->z - actor->z);

		if (dist < 1)
			dist = 1;

		actor->momx = FixedMul(FixedDiv(actor->target->x - actor->x, dist), realspeed);
		actor->momy = FixedMul(FixedDiv(actor->target->y - actor->y, dist), realspeed);
		actor->momz = FixedMul(FixedDiv(actor->target->z - actor->z, dist), realspeed);

		if (actor->z+actor->momz >= actor->waterbottom && actor->watertop > actor->floorz
			&& actor->z+actor->momz > actor->watertop - 256*FRACUNIT)
		{
			actor->momz = 0;
			actor->z = actor->watertop;
		}
	}
}

// Function: A_SetReactionTime
//
// Description: Sets the object's reaction time.
//
// var1 = unused
// var2 = unused
//
void A_SetReactionTime(mobj_t *actor)
{
	actor->reactiontime = actor->info->reactiontime;
}

// Function: A_LinedefExecute
//
// Description: Object's location is used to set the calling sector. The tag used is var1, or, if that is zero, the mobj's state number (beginning from 0) + 1000.
//
// var1 = tag (optional)
// var2 = unused
//
void A_LinedefExecute(mobj_t *actor)
{
	int tagnum;
	int locvar1 = var1;

	if (locvar1 > 0)
		tagnum = locvar1;
	else
		tagnum = (int)(1000 + (size_t)(actor->state - states));

	if (cv_debug)
		CONS_Printf("A_LinedefExecute: Running mobjtype %d's sector with tag %d\n", actor->type, tagnum);
	
	P_LinedefExecute(tagnum, actor, actor->subsector->sector);
}

// Function: A_PlaySeeSound
//
// Description: Plays the object's seesound.
//
// var1 = unused
// var2 = unused
//
void A_PlaySeeSound(mobj_t *actor)
{
	if (actor->info->seesound)
		S_StartScreamSound(actor, actor->info->seesound);
}

// Function: A_PlayAttackSound
//
// Description: Plays the object's attacksound.
//
// var1 = unused
// var2 = unused
//
void A_PlayAttackSound(mobj_t *actor)
{
	if (actor->info->attacksound)
		S_StartAttackSound(actor, actor->info->attacksound);
}

// Function: A_PlayActiveSound
//
// Description: Plays the object's activesound.
//
// var1 = unused
// var2 = unused
//
void A_PlayActiveSound(mobj_t *actor)
{
	if (actor->info->activesound)
		S_StartSound(actor, actor->info->activesound);
}

// Function: A_SmokeTrailer
//
// Description: Adds smoke trails to an object.
//
// var1 = object # to spawn as smoke
// var2 = unused
//
void A_SmokeTrailer(mobj_t *actor)
{
	mobj_t *th;
	int locvar1 = var1;

	if (gametic % (4*NEWTICRATERATIO))
		return;

	// add the smoke behind the rocket
	th = P_SpawnMobj(actor->x-actor->momx, actor->y-actor->momy, actor->z, locvar1);

	th->momz = FRACUNIT;
	th->tics -= P_Random() & 3;
	if (th->tics < 1)
		th->tics = 1;
}

// Function: A_SpawnObjectAbsolute
//
// Description: Spawns an object at an absolute position
//
// var1:
//		var1 >> 16 = x
//		var1 & 65535 = y
// var2:
//		var2 >> 16 = z
//		var2 & 65535 = type
//
void A_SpawnObjectAbsolute(mobj_t *actor)
{
	signed short x, y, z; // Want to be sure we can use negative values
	mobjtype_t type;
	int locvar1 = var1;
	int locvar2 = var2;

	x = (signed short)(locvar1>>16);
	y = (signed short)(locvar1&65535);
	z = (signed short)(locvar2>>16);
	type = (mobjtype_t)(locvar2&65535);

	P_SpawnMobj(x<<FRACBITS, y<<FRACBITS, z<<FRACBITS, type);
	actor = NULL;
}

// Function: A_SpawnObjectRelative
//
// Description: Spawns an object relative to the location of the actor
//
// var1:
//		var1 >> 16 = x
//		var1 & 65535 = y
// var2:
//		var2 >> 16 = z
//		var2 & 65535 = type
//
void A_SpawnObjectRelative(mobj_t *actor)
{
	signed short x, y, z; // Want to be sure we can use negative values
	mobjtype_t type;
	int locvar1 = var1;
	int locvar2 = var2;

	if(cv_debug)
		CONS_Printf("A_SpawnObjectRelative called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);
	
	x = (signed short)(locvar1>>16);
	y = (signed short)(locvar1&65535);
	z = (signed short)(locvar2>>16);
	type = (mobjtype_t)(locvar2&65535);

	P_SpawnMobj(actor->x + (x<<FRACBITS), actor->y + (y<<FRACBITS), actor->z + (z<<FRACBITS), type);
}

// Function: A_ChangeAngleRelative
//
// Description: Changes the angle to a random relative value between the min and max. Set min and max to the same value to eliminate randomness
//
// var1 = min
// var2 = max
//
void A_ChangeAngleRelative(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;
	angle_t angle = P_Random()+1;
	const angle_t amin = FixedAngle(locvar1*FRACUNIT);
	const angle_t amax = FixedAngle(locvar2*FRACUNIT);
	angle *= (P_Random()+1);
	angle *= (P_Random()+1);
	angle *= (P_Random()+1);

#ifdef PARANOIA
	if (amin > amax)
		I_Error("A_ChangeAngleRelative: var1 is greater then var2");	
#endif

	if (angle < amin)
		angle = amin;
	if (angle > amax)
		angle = amax;

	actor->angle += angle;
}

// Function: A_ChangeAngleAbsolute
//
// Description: Changes the angle to a random absolute value between the min and max. Set min and max to the same value to eliminate randomness
//
// var1 = min
// var2 = max
//
void A_ChangeAngleAbsolute(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;
	angle_t angle = P_Random()+1;
	const angle_t amin = FixedAngle(locvar1*FRACUNIT);
	const angle_t amax = FixedAngle(locvar2*FRACUNIT);
	angle *= (P_Random()+1);
	angle *= (P_Random()+1);
	angle *= (P_Random()+1);

#ifdef PARANOIA
	if (amin > amax)
		I_Error("A_ChangeAngleAbsolute: var1 is greater then var2");	
#endif

	if (angle < amin)
		angle = amin;
	if (angle > amax)
		angle = amax;

	actor->angle = angle;
}

// Function: A_PlaySound
//
// Description: Plays a sound
//
// var1 = sound # to play
// var2:
//		0 = Play sound without an origin
//		1 = Play sound using calling object as origin
//
void A_PlaySound(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	S_StartSound(locvar2 ? NULL : actor, locvar1);
}

// Function: A_FindTarget
//
// Description: Finds the nearest/furthest mobj of the specified type and sets actor->target to it.
//
// var1 = mobj type
// var2 = if (0) nearest; else furthest;
// 
void A_FindTarget(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;
	mobj_t *targetedmobj = NULL;
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist1 = 0, dist2 = 0;

	if(cv_debug)
		CONS_Printf("A_FindTarget called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == (mobjtype_t)locvar1)
		{
			if (targetedmobj == NULL)
			{
				targetedmobj = mo2;
				dist2 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);
			}
			else
			{
				dist1 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);

				if ((!locvar2 && dist1 < dist2) || (locvar2 && dist1 > dist2))
				{
					targetedmobj = mo2;
					dist2 = dist1;
				}
			}
		}
	}

	if (!targetedmobj)
	{
		if(cv_debug)
			CONS_Printf("A_FindTarget: Unable to find the specified object to target.\n");
		return; // Oops, nothing found..
	}

	if(cv_debug)
		CONS_Printf("A_FindTarget: Found a target.\n");

	actor->target = targetedmobj;
}

// Function: A_FindTracer
//
// Description: Finds the nearest/furthest mobj of the specified type and sets actor->tracer to it.
//
// var1 = mobj type
// var2 = if (0) nearest; else furthest;
// 
void A_FindTracer(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;
	mobj_t *targetedmobj = NULL;
	thinker_t *th;
	mobj_t *mo2;
	fixed_t dist1 = 0, dist2 = 0;

	if(cv_debug)
		CONS_Printf("A_FindTracer called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);
	
	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == (mobjtype_t)locvar1)
		{
			if (targetedmobj == NULL)
			{
				targetedmobj = mo2;
				dist2 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);
			}
			else
			{
				dist1 = R_PointToDist2(actor->x, actor->y, mo2->x, mo2->y);

				if ((!locvar2 && dist1 < dist2) || (locvar2 && dist1 > dist2))
				{
					targetedmobj = mo2;
					dist2 = dist1;
				}
			}
		}
	}

	if (!targetedmobj)
	{
		if(cv_debug)
			CONS_Printf("A_FindTracer: Unable to find the specified object to target.\n");
		return; // Oops, nothing found..
	}

	if(cv_debug)
		CONS_Printf("A_FindTracer: Found a target.\n");

	actor->tracer = targetedmobj;
}

// Function: A_SetTics
//
// Description: Sets the animation tics of an object
//
// var1 = tics to set to
// var2 = if this is set, and no var1 is supplied, the mobj's threshold value will be used.
//
void A_SetTics(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar1)
		actor->tics = locvar1;
	else if (locvar2)
		actor->tics = actor->threshold;
}

// Function: A_ChangeColorRelative
//
// Description: Changes the color of an object
//
// var1 = if (var1 > 0), find target and add its color value to yours
// var2 = if (var1 = 0), color value to add
//
void A_ChangeColorRelative(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar1)
	{
		// Have you ever seen anything so hideous?
		if (actor->target)
			actor->flags = (actor->flags & ~MF_TRANSLATION) | ((((((actor->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8))/256)+(((actor->target->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8))/256))<<MF_TRANSSHIFT));
	}
	else
		actor->flags = (actor->flags & ~MF_TRANSLATION) | (((((actor->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8))/256)+locvar2)<<MF_TRANSSHIFT);
}

// Function: A_ChangeColorAbsolute
//
// Description: Changes the color of an object by an absolute value.
//
// var1 = if (var1 > 0), set your color to your target's color
// var2 = if (var1 = 0), color value to set to
//
void A_ChangeColorAbsolute(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar1)
	{
		if (actor->target)
			actor->flags = (actor->flags & ~MF_TRANSLATION) | (((((actor->target->flags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8))/256)<<MF_TRANSSHIFT));
	}
	else
		actor->flags = (actor->flags & ~MF_TRANSLATION) | (locvar2<<MF_TRANSSHIFT);
}

// Function: A_MoveRelative
//
// Description: Moves an object (wrapper for P_Thrust)
//
// var1 = angle
// var2 = force
//
void A_MoveRelative(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	P_Thrust(actor, actor->angle+(locvar1*(ANG45/45)), locvar2*FRACUNIT);
}

// Function: A_MoveAbsolute
//
// Description: Moves an object (wrapper for P_InstaThrust)
//
// var1 = angle
// var2 = force
//
void A_MoveAbsolute(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	P_InstaThrust(actor, (locvar1*(ANG45/45)), locvar2*FRACUNIT);
}

// Function: A_SetTargetsTarget
//
// Description: Sets your target to the object who has your target targeted. Yikes! If it happens to be NULL, you're just out of luck.
//
// var1 = unused
// var2 = unused
//
void A_SetTargetsTarget(mobj_t *actor)
{
	mobj_t *targetedmobj = NULL;
	thinker_t *th;
	mobj_t *mo2;

	if (!actor->target)
		return;

	if (!actor->target->target)
		return; // Don't search for nothing.

	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2 == actor->target->target)
		{
			targetedmobj = mo2;
			break;
		}
	}

	if (!targetedmobj)
		return; // Oops, nothing found..

	actor->target = targetedmobj;
}

// Function: A_SetObjectFlags
//
// Description: Sets the flags of an object
//
// var1 = flag value to set
// var2:
//		if var2 == 2, add the flag to the current flags
//		else if var2 == 1, remove the flag from the current flags
//		else if var2 == 0, set the flags to the exact value
//
void A_SetObjectFlags(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar2 == 2)
		actor->flags |= locvar1;
	else if (locvar2 == 1)
		actor->flags &= ~locvar1;
	else
		actor->flags = locvar1;
}

// Function: A_SetObjectFlags2
//
// Description: Sets the flags2 of an object
//
// var1 = flag value to set
// var2:
//		if var2 == 2, add the flag to the current flags
//		else if var2 == 1, remove the flag from the current flags
//		else if var2 == 0, set the flags to the exact value
//
void A_SetObjectFlags2(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if (locvar2 == 2)
		actor->flags2 |= locvar1;
	else if (locvar2 == 1)
		actor->flags2 &= ~locvar1;
	else
		actor->flags2 = locvar1;
}

// Function: A_BossJetFume
//
// Description: Spawns jet fumes for the boss. To only be used when he is spawned.
//
// var1:
//		0 = Boss1 jet fume pattern
//		1 = Boss2 jet fume pattern
// var2 = unused
//
void A_BossJetFume(mobj_t *actor)
{
	mobj_t *filler;
	int locvar1 = var1;

	if (locvar1 == 0) // Boss1 jet fumes
	{
		fixed_t jetx, jety;

		jetx = actor->x + P_ReturnThrustX(actor, actor->angle, -56*FRACUNIT);
		jety = actor->y + P_ReturnThrustY(actor, actor->angle, -56*FRACUNIT);

		filler = P_SpawnMobj(jetx, jety, actor->z + 8*FRACUNIT, MT_JETFUME1);
		filler->target = actor;
		filler->fuse = 56;

		filler = P_SpawnMobj(jetx + P_ReturnThrustX(actor, actor->angle-ANG90, 24*FRACUNIT), jety + P_ReturnThrustY(actor, actor->angle-ANG90, 24*FRACUNIT), actor->z + 32*FRACUNIT, MT_JETFUME1);
		filler->target = actor;
		filler->fuse = 57;

		filler = P_SpawnMobj(jetx + P_ReturnThrustX(actor, actor->angle+ANG90, 24*FRACUNIT), jety + P_ReturnThrustY(actor, actor->angle+ANG90, 24*FRACUNIT), actor->z + 32*FRACUNIT, MT_JETFUME1);
		filler->target = actor;
		filler->fuse = 58;

		actor->tracer = filler;
	}
	else if (locvar1 == 1) // Boss2 jet fumes
	{
		filler = P_SpawnMobj(actor->x, actor->y, actor->z - 15*FRACUNIT, MT_JETFUME2);
		filler->target = actor;

		actor->tracer = filler;
	}
}

// Function: A_RandomState
//
// Description: Chooses one of the two state numbers supplied randomly.
//
// var1 = state number 1
// var2 = state number 2
//
void A_RandomState(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	P_SetMobjState(actor, P_Random()&1 ? locvar1 : locvar2);
}

// Function: A_RandomStateRange
//
// Description: Chooses a random state within the range supplied.
//
// var1 = Minimum state number to choose. 
// var2 = Maximum state number to use. 
//
void A_RandomStateRange(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;
	int statenum;
	int difference = locvar2 - locvar1;

	// Scale P_Random() to the difference.
	statenum = P_Random();
	statenum /= statenum/difference;

	statenum += locvar1;

	P_SetMobjState(actor, statenum);
}

// Function: A_DualAction
//
// Description: Calls two actions. Be careful, if you reference the same state this action is called from, you can create an infinite loop.
//
// var1 = state # to use 1st action from
// var2 = state # to use 2nd action from
//
void A_DualAction(mobj_t *actor)
{
	int locvar1 = var1;
	int locvar2 = var2;

	if(cv_debug)
		CONS_Printf("A_DualAction called from object type %d, var1: %d, var2: %d\n", actor->type, locvar1, locvar2);

	var1 = states[locvar1].var1;
	var2 = states[locvar1].var2;

	if(cv_debug)
		CONS_Printf("A_DualAction: Calling First Action (state %d)...\n", locvar1);
	states[locvar1].action.acp1(actor);

	var1 = states[locvar2].var1;
	var2 = states[locvar2].var2;

	if(cv_debug)
		CONS_Printf("A_DualAction: Calling Second Action (state %d)...\n", locvar2);
	states[locvar2].action.acp1(actor);
}

