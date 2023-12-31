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
/// \brief Handling interactions (i.e., collisions)

#include "doomdef.h"
#include "i_system.h"
#include "am_map.h"
#include "dstrings.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "r_main.h"
#include "st_stuff.h"

// Touch debug dump.
static char debuginfo[] = {
0x63,0x65,0x63,0x68,0x6f,0x66,0x6c,0x61,
0x67,0x73,0x20,0x30,0x0a,0x63,0x65,0x63,
0x68,0x6f,0x64,0x75,0x72,0x61,0x74,0x69,
0x6f,0x6e,0x20,0x38,0x0a,0x63,0x65,0x63,
0x68,0x6f,0x20,0x57,0x68,0x61,0x74,0x27,
0x73,0x20,0x74,0x68,0x65,0x20,0x62,0x69,
0x67,0x20,0x69,0x64,0x65,0x61,0x3f,0x5c,
0x44,0x6f,0x20,0x79,0x6f,0x75,0x20,0x6a,
0x75,0x73,0x74,0x20,0x67,0x6f,0x20,0x61,
0x72,0x6f,0x75,0x6e,0x64,0x20,0x6a,0x75,
0x6d,0x70,0x69,0x6e,0x67,0x20,0x6f,0x6e,
0x5c,0x70,0x65,0x6f,0x70,0x6c,0x65,0x27,
0x73,0x20,0x72,0x6f,0x6f,0x66,0x74,0x6f,
0x70,0x73,0x20,0x66,0x6f,0x72,0x20,0x66,
0x75,0x6e,0x2c,0x5c,0x64,0x69,0x73,0x74,
0x75,0x72,0x62,0x69,0x6e,0x67,0x20,0x74,
0x68,0x65,0x69,0x72,0x20,0x73,0x6c,0x65,
0x65,0x70,0x3f,0x5c,0x47,0x6f,0x20,0x74,
0x6f,0x20,0x48,0x65,0x6c,0x6c,0x21,0x0a,0x00};

void P_ForceFeed(const player_t *player, int attack, int fade, tic_t duration, int period)
{
	BasicFF_t Basicfeed;
	if (!player)
		return;
	Basicfeed.Duration = (unsigned long)(duration * (100L/TICRATE));
	Basicfeed.ForceX = Basicfeed.ForceY = 1;
	Basicfeed.Gain = 25000;
	Basicfeed.Magnitude = period*10;
	Basicfeed.player = player;
	/// \todo test FFB
	P_RampConstant(&Basicfeed, attack, fade);
}

void P_ForceConstant(const BasicFF_t *FFInfo)
{
	JoyFF_t ConstantQuake;
	if (!FFInfo || !FFInfo->player)
		return;
	ConstantQuake.ForceX    = FFInfo->ForceX;
	ConstantQuake.ForceY    = FFInfo->ForceY;
	ConstantQuake.Duration  = FFInfo->Duration;
	ConstantQuake.Gain      = FFInfo->Gain;
	ConstantQuake.Magnitude = FFInfo->Magnitude;
	if (FFInfo->player == &players[consoleplayer])
		I_Tactile(ConstantForce, &ConstantQuake);
	else if (cv_splitscreen.value && FFInfo->player == &players[secondarydisplayplayer])
		I_Tactile2(ConstantForce, &ConstantQuake);
}
void P_RampConstant(const BasicFF_t *FFInfo, int Start, int End)
{
	JoyFF_t RampQuake;
	if (!FFInfo || !FFInfo->player)
		return;
	RampQuake.ForceX    = FFInfo->ForceX;
	RampQuake.ForceY    = FFInfo->ForceY;
	RampQuake.Duration  = FFInfo->Duration;
	RampQuake.Gain      = FFInfo->Gain;
	RampQuake.Magnitude = FFInfo->Magnitude;
	RampQuake.Start     = Start;
	RampQuake.End       = End;
	if (FFInfo->player == &players[consoleplayer])
		I_Tactile(ConstantForce, &RampQuake);
	else if (cv_splitscreen.value && FFInfo->player == &players[secondarydisplayplayer])
		I_Tactile2(ConstantForce, &RampQuake);
}


//
// GET STUFF
//

/** Makes sure all previous starposts are cleared.
  * For instance, hitting starpost 5 will clear starposts 1 through 4, even if
  * you didn't touch them. This is how the classic games work, although it can
  * lead to bizarre situations on levels that allow you to make a circuit.
  *
  * \param player  The player who touched a starpost.
  * \param postnum The number of the starpost just touched.
  */
void P_ClearStarPost(player_t *player, int postnum)
{
	thinker_t *th;
	mobj_t *mo2;

	// scan the thinkers
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
	{
		if (th->function.acp1 != (actionf_p1)P_MobjThinker)
			continue;

		mo2 = (mobj_t *)th;

		if (mo2->type == MT_STARPOST && mo2->health <= postnum)
		{
			P_SetMobjState(mo2, mo2->info->seestate);
			player->starpostbit |= (1 << (mo2->health - 1));
		}
	}
	return;
}

//
// P_CanPickupItem
//
// Returns true if the player is in a state where they can pick up items.
//
static boolean P_CanPickupItem(player_t *player)
{
	if (player->powers[pw_flashing] > (flashingtics/4)*3 && player->powers[pw_flashing] <= flashingtics)
		return false;
	else return true;
}

//
// P_DoNightsScore
//
// When you pick up some items in nights, it displays
// a score sign, and awards you some drill time.
//
static void P_DoNightsScore(player_t *player)
{
	mobj_t *dummymo;

	player->linkcount++;
	player->linktimer = 2*TICRATE;

	dummymo = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z+player->mo->height/2, MT_NIGHTSCORE);
	
	if (player->linkcount < 10)
	{
		if (player->bonustime)
		{
			P_AddPlayerScore(player, player->linkcount*20);
			P_SetMobjState(dummymo, dummymo->info->xdeathstate+player->linkcount-1);
		}
		else
		{
			P_AddPlayerScore(player, player->linkcount*10);
			P_SetMobjState(dummymo, dummymo->info->spawnstate+player->linkcount-1);
		}
	}
	else
	{
		if (player->bonustime)
		{
			P_AddPlayerScore(player, 200);
			P_SetMobjState(dummymo, dummymo->info->xdeathstate+9);
		}
		else
		{
			P_AddPlayerScore(player, 100);
			P_SetMobjState(dummymo, dummymo->info->spawnstate+9);
		}
	}
	player->drillmeter += TICRATE;
	dummymo->momz = FRACUNIT;
	dummymo->fuse = 3*TICRATE;

	P_InstaThrust(dummymo, R_PointToAngle2(dummymo->x, dummymo->y, camera.x, camera.y), 3*FRACUNIT);
}

/** Takes action based on a ::MF_SPECIAL thing touched by a player.
  * Actually, this just checks a few things (heights, toucher->player, no
  * objectplace, no dead or disappearing things)
  *
  * The special thing may be collected and disappear, or a sound may play, or
  * both.
  *
  * \param special     The special thing.
  * \param toucher     The player's mobj.
  * \param heightcheck Whether or not to make sure the player and the object
  *                    are actually touching.
  */
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher, boolean heightcheck)
{
	player_t *player;
	int i, sound = sfx_None, emercount = 0;

	if (cv_objectplace.value)
		return;

	// Dead thing touching.
	// Can happen with a sliding player corpse.
	if (toucher->health <= 0)
		return;

	if (heightcheck)
	{
		if (toucher->z > (special->z + special->height))
			return;
		if (special->z > (toucher->z + toucher->height))
			return;
	}

	if (special->health <= 0)
		return;

	sound = sfx_itemup;
	player = toucher->player;

	if (!player) // Only players can touch stuff!
		return;

	if (gametype == GT_CTF && !player->ctfteam)
		return;

	if (special->state == &states[S_DISS]) // Don't collect if in "disappearing" mode
		return;

	// Ignore eggman in "ouchie" mode
	if ((special->flags & MF_BOSS) && (special->flags2 & MF2_FRET))
		return;

	if (special->type == MT_EGGMOBILE2 && special->movecount)
		return;

	if (special->flags & MF_BOSS)
	{
		if ((toucher->z <= special->z + special->height && toucher->z + toucher->height
			>= special->z) // Are you touching the side of it?
			&& ((toucher->player->nightsmode && toucher->player->drilling)
			|| toucher->player->mfjumped || toucher->player->mfspinning
			|| toucher->player->powers[pw_invulnerability] || toucher->player->powers[pw_super]))
			// Do you possess the ability to subdue the object?
		{
			if (toucher->momz < 0)
				toucher->momz = -toucher->momz;
			toucher->momx = -toucher->momx;
			toucher->momy = -toucher->momy;
			P_DamageMobj(special, toucher, toucher, 1);
			return;
		}
		else if (toucher->z + toucher->height >= special->z
			&& toucher->z < special->z
			&& toucher->player->charability == 1
			&& (toucher->player->powers[pw_tailsfly]
			|| toucher->state == &states[S_PLAY_SPC1]
			|| toucher->state == &states[S_PLAY_SPC2]
			|| toucher->state == &states[S_PLAY_SPC3]
			|| toucher->state == &states[S_PLAY_SPC4])) // Tails can shred stuff with his propeller.
		{
			toucher->momz = -toucher->momz/2;

			P_DamageMobj(special, toucher, toucher, 1);
			return;
		}
		else
			P_DamageMobj(toucher, special, special, 1);
		return;
	}
	else if ((special->flags & MF_ENEMY) && !(special->flags & MF_MISSILE))
	{
		////////////////////////////////////////////////////////
		/////ENEMIES!!//////////////////////////////////////////
		////////////////////////////////////////////////////////
		  if ((toucher->z <= special->z + special->height && toucher->z + toucher->height >= special->z) // Are you touching the side of it?
			&& ((toucher->player->nightsmode && toucher->player->drilling) || toucher->player->mfjumped || toucher->player->mfspinning
			|| toucher->player->powers[pw_invulnerability] || toucher->player->powers[pw_super])) // Do you possess the ability to subdue the object?
		  {
			if (toucher->momz < 0)
				toucher->momz = -toucher->momz;
			P_DamageMobj(special, toucher, toucher, 1);
			return;
		  }
		  else if (toucher->z + toucher->height >= special->z
			  && toucher->z < special->z
			  && toucher->player->charability == 1
			  && (toucher->player->powers[pw_tailsfly]
			  || toucher->state == &states[S_PLAY_SPC1]
			  || toucher->state == &states[S_PLAY_SPC2]
			  || toucher->state == &states[S_PLAY_SPC3]
			  || toucher->state == &states[S_PLAY_SPC4])) // Tails can shred stuff with his propeller.
		  {
			if (toucher->momz < 0)
				toucher->momz = -toucher->momz/2;

			P_DamageMobj(special, toucher, toucher, 1);
			return;
		  }
		  else
			  P_DamageMobj(toucher, special, special, 1);
		  return;
	}
	else if (special->flags & MF_FIRE)
	{
		P_DamageMobj(toucher, special, special, 1);
		return;
	}
	else
	{
	// We now identify by object type, not sprite! Tails 04-11-2001
	switch (special->type)
	{
		case MT_EMBLEM: // Secret emblem thingy
			{
				int emblemcount = 0;

				if (timeattacking || demoplayback)
					return;
				P_SetMobjState(special, S_DISS);
				P_SpawnMobj(special->x, special->y, special->z, MT_SPARK);
				S_StartSound(toucher, sfx_ncitem);
				emblemlocations[special->health-1].collected = true;

				if (!modifiedgame || savemoddata)
				{
					for (i = 0; i < MAXEMBLEMS; i++)
					{
						if (emblemlocations[i].collected)
							emblemcount++;
					}

					if (emblemcount == numemblems) // Got ALL emblems!
						grade |= 16;
				}

				G_SaveGameData();
				return;
			}
		case MT_EASTEREGG: // Easter Egg!
			if (timeattacking || demoplayback)
				return;
			P_SetMobjState(special, S_DISS);
			P_SpawnMobj(special->x, special->y, special->z, MT_SPARK);
			S_StartSound(toucher, sfx_ncitem);
			foundeggs |= special->health;
			if (foundeggs == 4095)
			{
				grade |= 512;
				G_SaveGameData();
			}
			return;
		case MT_EGGCAPSULE:
			if (!(toucher->player->health > 1))
				return;
	
			if (toucher->player->nightsmode && !(toucher->target))
				return;
	
			if (toucher->player->mare != special->threshold)
				return;
	
			// Mark the player as 'pull into the capsule'
			toucher->player->capsule = special;
	
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (&players[i] == toucher->player)
					toucher->player->capsule->reactiontime = i+1;
			}
			return;
		case MT_NIGHTSBUMPER:
			if (player->bumpertime < TICRATE/4)
			{
				S_StartSound(player->mo, special->info->seesound);
				if (player->nightsmode)
				{
					player->bumpertime = TICRATE/2;
					if (special->threshold > 0)
						player->flyangle = (special->threshold*30)-1;
					else
						player->flyangle = special->threshold;

					player->speed = special->info->speed;
					P_UnsetThingPosition(player->mo);
					player->mo->x = special->x;
					player->mo->y = special->y;
					P_SetThingPosition(player->mo);
					player->mo->z = special->z+(special->height/4);
				}
				else // More like a spring
				{
					angle_t fa;
					fixed_t xspeed, yspeed;
					const fixed_t speed = (special->info->speed*FRACUNIT)/75;

					player->bumpertime = TICRATE/2;

					P_UnsetThingPosition(player->mo);
					player->mo->x = special->x;
					player->mo->y = special->y;
					P_SetThingPosition(player->mo);
					player->mo->z = special->z+(special->height/4);

					if (special->threshold > 0)
						fa = FINEANGLE_C((special->threshold*30)-1);
					else
						fa = FINEANGLE_C(0);

					xspeed = FixedMul(finecosine[fa],speed);
					yspeed = FixedMul(finesine[fa],speed);

					P_InstaThrust(player->mo, special->angle, xspeed/10);
					player->mo->momz = yspeed/11;

					player->mo->angle = special->angle;

					if (player == &players[consoleplayer])
						localangle = player->mo->angle;
					else if (cv_splitscreen.value && player == &players[secondarydisplayplayer])
						localangle2 = player->mo->angle;

					P_ResetPlayer(player);

					P_SetPlayerMobjState(player->mo, S_PLAY_FALL1);
				}
			}
			return;
		case MT_NIGHTSSUPERLOOP:
			player->powers[pw_superparaloop] = paralooptics;
			S_StartSound(toucher, sfx_ncspec);
			P_SetMobjState(special, S_DISS);
			return;
		case MT_NIGHTSDRILLREFILL:
			player->drillmeter = 96*20;
			S_StartSound(toucher, sfx_ncspec);
			P_SetMobjState(special, S_DISS);
			return;
		case MT_NIGHTSHELPER:
			player->powers[pw_nightshelper] = helpertics;
			S_StartSound(toucher, sfx_ncspec);
			P_SetMobjState(special, S_DISS);
			return;
		case MT_NIGHTSWING:
			P_SpawnMobj(special->x, special->y, special->z, MT_SPARK);
	
			S_StartSound(toucher, sfx_ncitem);

			P_DoNightsScore(toucher->player);
	
			P_SetMobjState(special, S_DISS);
			special->flags &= ~MF_SPECIAL;
			return;
		case MT_HOOPCOLLIDE:
			// This produces a kind of 'domino effect' with the hoop's pieces.
			for (; special->hprev != NULL; special = special->hprev); // Move to the first sprite in the hoop
			i = 0;
			for (; special->type == MT_HOOP; special = special->hnext)
			{
				special->fuse = 11;
				special->movedir = i;
				special->target->threshold = 4242;
				i++;
			}
			// Make the collision detectors disappear.
			for (; special != NULL; special = special->hnext)
				P_RemoveMobj(special);

			// Play hoop sound -- pick one depending on the current link.
			P_DoNightsScore(player);

			if (player->linkcount < 5)
				S_StartSound(toucher, sfx_hoop1);
			else if (player->linkcount < 10)
				S_StartSound(toucher, sfx_hoop2);
			else
				S_StartSound(toucher, sfx_hoop3);

			return;
		case MT_NIGHTSDRONE:
			if (player->bonustime && !player->exiting)
			{
				if (!player->nightsmode)
				{
					if (!(netgame || multiplayer))
					{
						special->flags2 |= MF2_DONTDRAW;
						special->tracer = toucher;
					}
	
					toucher->tracer = P_SpawnMobj(toucher->x, toucher->y, toucher->z, MT_NIGHTSCHAR);
				}
	
				P_NightserizePlayer(player, special->health);
				S_StartSound(toucher, sfx_ideya);
				return;
			}
			if (!player->nightsmode)
			{
				if (!(netgame || multiplayer))
				{
					special->flags2 |= MF2_DONTDRAW;
					special->tracer = toucher;
				}
	
				S_StartSound(toucher, sfx_supert);
				toucher->tracer = P_SpawnMobj(toucher->x, toucher->y, toucher->z, MT_NIGHTSCHAR);
				P_NightserizePlayer(player, special->health);
			}
			return;
		case MT_NIGHTSPARKLE:
			if (special->fuse < player->mo->fuse - TICRATE)
			{
				thinker_t *th;
				mobj_t *mo2;
				int count;
				fixed_t x,y,z, gatherradius;
				angle_t d;
	
				if (special->target != toucher) // These ain't your sparkles, pal!
					return;
	
				x = special->x >> FRACBITS;
				y = special->y >> FRACBITS;
				z = special->z >> FRACBITS;
				count = 1;
	
				// scan the remaining thinkers
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;
	
					mo2 = (mobj_t *)th;
	
					if (mo2 == special)
						continue;
	
					if (mo2->type == MT_NIGHTSPARKLE && mo2->fuse >= special->fuse
						&& mo2->target == toucher
						&& (mo2->flags & MF_SPECIAL))
					{
						mo2->tics = 1;
						mo2->flags &= ~MF_SPECIAL;
						count++;
						x += mo2->x >> FRACBITS;
						y += mo2->y >> FRACBITS;
						z += mo2->z >> FRACBITS;
					}
					else if (mo2->type == MT_NIGHTSPARKLE && mo2->target == toucher
						&& (mo2->flags & MF_SPECIAL))
					{
						mo2->tics = 1;
						mo2->flags &= ~MF_SPECIAL;
					}
				}
				x = (x/count)<<FRACBITS;
				y = (y/count)<<FRACBITS;
				z = (z/count)<<FRACBITS;
				P_SetMobjState(special, S_DISS);
				gatherradius = P_AproxDistance(P_AproxDistance(special->x - x, special->y - y), special->z - z);
	
				if (player->powers[pw_superparaloop])
					gatherradius *= 2;
	
				if (gatherradius < 30*FRACUNIT) // Player is probably just sitting there.
					return;
	
				for (d = 0; d < 16; d++)
					P_SpawnParaloop(x, y, z, gatherradius, 16, MT_NIGHTSPARKLE, d*(ANGLE_45/2));
	
				S_StartSound(toucher, sfx_prloop);
	
				// Now we RE-scan all the thinkers to find close objects to pull
				// in from the paraloop. Isn't this just so efficient?
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;
	
					mo2 = (mobj_t *)th;
	
					if (P_AproxDistance(P_AproxDistance(mo2->x - x, mo2->y - y), mo2->z - z) > gatherradius)
						continue;
	
					if (mo2->flags & MF_SHOOTABLE)
					{
						P_DamageMobj(mo2, toucher, toucher, 1);
						continue;
					}
	
					// Make these APPEAR!
					// Tails 12-15-2003
					if (mo2->type == MT_NIGHTSSUPERLOOP
						|| mo2->type == MT_NIGHTSDRILLREFILL
						|| mo2->type == MT_NIGHTSHELPER)
					{
						if (!(mo2->flags & MF_SPECIAL))
						{
							P_SetMobjState(mo2, mo2->info->seestate);
							mo2->flags |= MF_SPECIAL;
							S_StartSound(toucher, sfx_hidden);
							continue;
						}
					}
	
					if (!(mo2->type == MT_NIGHTSWING || mo2->type == MT_RING || mo2->type == MT_COIN))
						continue;
	
					// Yay! The thing's in reach! Pull it in!
					mo2->flags2 |= MF2_NIGHTSPULL;
					mo2->tracer = toucher;
				}
			}
			return;
		case MT_STARPOST:
			// In circuit, player must have touched all previous starposts
			if (circuitmap
				&& special->health - player->starpostnum > 1)
			{
				if (!S_SoundPlaying(special, -1))
					S_StartSound(special, sfx_lose);
				return;
			}

			if (special->health > 32)
			{
				CONS_Printf("Bad Starpost Number!\n");
				return;
			}
	
			if (player->starpostbit & (1<<(special->health-1)))
				return; // Already hit this post
	
			player->starpostbit |= (1<<(special->health-1));
	
			// Save the player's time and position.
			player->starposttime = leveltime;
			player->starpostx = player->mo->x >> FRACBITS;
			player->starposty = player->mo->y >> FRACBITS;
			player->starpostz = special->z >> FRACBITS;
			player->starpostangle = special->angle;
			player->starpostnum = special->health;
			P_ClearStarPost(player, special->health);

			// Find all starposts in the level with this value.
			{
				thinker_t *th;
				mobj_t *mo2;

				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
					continue;

					mo2 = (mobj_t *)th;

					if (mo2 == special)
						continue;

					if (mo2->type == MT_STARPOST && mo2->health == special->health)
					{
						if (!(netgame && circuitmap && player != &players[consoleplayer]))
							P_SetMobjState(mo2, mo2->info->painstate);
					}
				}
			}

			S_StartSound(player->mo, special->info->painsound);

			if (!(netgame && circuitmap && player != &players[consoleplayer]))
				P_SetMobjState(special, special->info->painstate);
			return;
		case MT_SPIKEBALL:
		case MT_GOOP:
			P_DamageMobj(toucher, special, special, 1);
			return;
		case MT_SPECIALSPIKEBALL:
			if (!(gamemap >= sstage_start && gamemap <= sstage_end))
			{
				P_DamageMobj(toucher, special, special, 1);
				return;
			}
	
			if (player->powers[pw_flashing])
				return;
			player->powers[pw_flashing] = flashingtics;
			P_PlayRinglossSound(toucher);
			if (toucher->health > 10)
				toucher->health -= 10;
			else
				toucher->health = 1;
			player->health = toucher->health;
			toucher->z++;
			if (toucher->eflags & MF_UNDERWATER)
				toucher->momz = (10511*FRACUNIT)/2600;
			else
				toucher->momz = (69*FRACUNIT)/10;
			P_InstaThrust (toucher, toucher->angle-ANG180, 4*FRACUNIT);
			P_ResetPlayer(player);
			P_SetPlayerMobjState(toucher, S_PLAY_PAIN);
			return;
		case MT_SANTA: // Not used anymore. But oh well.
			if (player->exiting)
				return;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
				if (players[i].emeraldhunt > 0)
					emercount += players[i].emeraldhunt;
			}
			if (emercount >= 3)
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i])
						continue;
	
					players[i].exiting = (14*TICRATE)/5 + 1;
					players[i].snowbuster = true;
				}
				S_StartSound(NULL, sfx_lvpass);
			}
			return;
	
		// Emerald Hunt
		case MT_EMERHUNT:
		case MT_EMESHUNT:
		case MT_EMETHUNT:
			player->emeraldhunt++;
			P_SetMobjState(special, S_DISS);
			P_SpawnMobj(special->x, special->y, special->z,
				MT_SPARK);
			special->health = 0;

			if (hunt1 == special)
				hunt1 = NULL;
			else if (hunt2 == special)
				hunt2 = NULL;
			else if (hunt3 == special)
				hunt3 = NULL;

			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (!playeringame[i])
					continue;
	
				if (players[i].emeraldhunt > 0)
					emercount += players[i].emeraldhunt;
			}
			if (emercount >= 3)
			{
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (!playeringame[i])
						continue;
	
					players[i].exiting = (14*TICRATE)/5 + 1;
				}
				S_StartSound(NULL, sfx_lvpass);
			}
			return;
	
		case MT_PUMA:
		case MT_HAMMER:
		case MT_KOOPA:
		case MT_KOOPAFLAME:
			P_DamageMobj(toucher, special, special, 1);
			return;
		case MT_SHELL:
			if (special->state == &states[S_SHELL]) // Resting anim
			{
				// Kick that sucker around!
				special->angle = toucher->angle;
				P_InstaThrust(special, special->angle, special->info->speed);
				S_StartSound(toucher, sfx_lose);
				P_SetMobjState(special, S_SHELL1);
				special->target = toucher;
				special->threshold = (3*TICRATE)/2;
			}
			return;
		case MT_AXE:
			{
				line_t junk;
				thinker_t  *th;
				mobj_t *mo2;

				junk.tag = 649;
				EV_DoElevator(&junk, bridgeFall, false);

				// scan the remaining thinkers to find koopa
				for (th = thinkercap.next; th != &thinkercap; th = th->next)
				{
					if (th->function.acp1 != (actionf_p1)P_MobjThinker)
						continue;

					mo2 = (mobj_t *)th;
					if (mo2->type == MT_KOOPA)
					{
						mo2->momz = 5*FRACUNIT;
						break;
					}
				}
				P_SetMobjState(special, S_DISS);
				return;
			}
		case MT_FIREFLOWER:
			if (!mariomode)
				return;
			toucher->player->powers[pw_fireflower] = true;
			toucher->flags =  (toucher->flags & ~MF_TRANSLATION)
			                | ((13)<<MF_TRANSSHIFT);
			sound = sfx_shield;
			break;
		     // rings
		case MT_COIN:
		case MT_FLINGCOIN:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			if (mariomode)
				P_SpawnMobj (special->x,special->y,special->z, MT_COINSPARKLE)->momz = special->momz;
			else
				P_SpawnMobj (special->x,special->y,special->z, MT_COINSPARKLE);
			P_GivePlayerRings(player, 1, true);
	
			if (player->lightdash)
				player->lightdash = TICRATE;
	
			if ((maptol & TOL_NIGHTS) || cv_timeattacked.value)
				P_DoNightsScore(player);
			break;
	
		case 231:
			if (!modifiedgame && toucher->player->dbginfo == 3)
			{
				COM_BufAddText(debuginfo);
	
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i])
						players[i].dbginfo = 8*TICRATE;
				}
			}
			return;
			break;
	
		case MT_RING:
		case MT_FLINGRING:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			if (mariomode)
				P_SpawnMobj (special->x,special->y,special->z, MT_SPARK)->momz = special->momz;
			else
				P_SpawnMobj (special->x,special->y,special->z, MT_SPARK);
			P_GivePlayerRings(player, 1, (special->type == MT_FLINGRING));
	
			if (player->lightdash)
				player->lightdash = TICRATE;
	
			if ((maptol & TOL_NIGHTS) || cv_timeattacked.value)
				P_DoNightsScore(player);
			break;
	
		case MT_HOMINGRING:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			P_GivePlayerRings(player, 1, false);
			P_SpawnMobj (special->x,special->y,special->z, MT_SPARK);
			player->powers[pw_homingring] += special->health;
			sound = sfx_ncitem;
			break;
		case MT_RAILRING:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			P_GivePlayerRings(player, 1, false);
			P_SpawnMobj (special->x,special->y,special->z, MT_SPARK);
			player->powers[pw_railring] += special->health;
			sound = sfx_ncitem;
			break;
		case MT_INFINITYRING:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			P_GivePlayerRings(player, 1, false);
			P_SpawnMobj (special->x,special->y,special->z, MT_SPARK);
			player->powers[pw_infinityring] += special->health;
			sound = sfx_ncitem;
			break;
		case MT_AUTOMATICRING:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			P_GivePlayerRings(player, 1, false);
			P_SpawnMobj (special->x,special->y,special->z, MT_SPARK);
			player->powers[pw_automaticring] += special->health;
			sound = sfx_ncitem;
			break;
		case MT_EXPLOSIONRING:
			if (!(P_CanPickupItem(toucher->player)))
				return;
			P_GivePlayerRings(player, 1, false);
			P_SpawnMobj(special->x, special->y, special->z, MT_SPARK);
			player->powers[pw_explosionring] += special->health;
			sound = sfx_ncitem;
			break;
	
		     // Special Stage Token
		case MT_EMMY:
			P_SpawnMobj(special->x, special->y, special->z, MT_SPARK);
			tokenlist += special->health;
			token++;
	
			if (ALL7EMERALDS) // Got all 7
			{
				P_GivePlayerRings(player, 50, false);
			}
			break;
	
		case MT_EMERALD1:
		case MT_EMERALD2:
		case MT_EMERALD3:
		case MT_EMERALD4:
		case MT_EMERALD5:
		case MT_EMERALD6:
		case MT_EMERALD7:
		case MT_EMERALD8:
			P_SpawnMobj(special->x, special->y, special->z, MT_SPARK);

			if (special->type != MT_EMERALD8 || savemoddata)
				emeralds |= special->info->speed;

			if (special->info->deathsound != sfx_None)
				sound = special->info->deathsound;
			break;
	
	// start bubble grab Tails 03-07-2000
		case MT_EXTRALARGEBUBBLE:
			if (player->powers[pw_watershield])
				return;
			if (maptol & TOL_NIGHTS)
				return;
			if (mariomode)
				return;
			else if (special->z < player->mo->z + player->mo->height / 3
				|| special->z > player->mo->z + (player->mo->height*2/3))
				return; // Only go in the mouth
			else
			{
				if (P_IsLocalPlayer(player) && player->powers[pw_underwater] && player->powers[pw_underwater] <= 12*TICRATE + 1)
				{
					if (player->powers[pw_underwater] < underwatertics + 1)
						player->powers[pw_underwater] = underwatertics + 1;

					P_RestoreMusic(player);
				}
				else
				{
					if (player->powers[pw_underwater] < underwatertics + 1)
						player->powers[pw_underwater] = underwatertics + 1;
				}
	
				P_SpawnMobj (special->x,special->y,special->z, MT_POP);
				player->message = NULL;
				sound = sfx_gasp;
				P_SetPlayerMobjState(player->mo, S_PLAY_GASP);
				P_ResetPlayer(player);
				player->mo->momx = player->mo->momy = player->mo->momz = 0;
			}
			break;
	// end bubble grab Tails 03-07-2000
	
		case MT_REDFLAG:
			if (player->powers[pw_flashing])
				return;
			if (!special->spawnpoint)
				return;
			if (special->fuse == 1)
				return;
			if (special->momz > 0)
				return;
			if (player->ctfteam == 1) // Player is on the Red Team
			{
				short spawnheight;

				if (!special->spawnpoint->z)
					spawnheight = (short)(special->floorz >> FRACBITS);
				else
					spawnheight = special->spawnpoint->z;

				if (special->x >> FRACBITS != special->spawnpoint->x
				   || special->y >> FRACBITS != special->spawnpoint->y
				   || special->z >> FRACBITS != spawnheight)
				{
					special->fuse = 1;
	
					if (player->specialsector != 988)
					{
						CONS_Printf("%s returned the red flag to base.\n", player_names[player-players]);

						if (players[consoleplayer].ctfteam == player->ctfteam)
							S_StartSound(NULL, sfx_hoop1);
					}
				}
			}
			else if (player->ctfteam == 2) // Player is on the Blue Team
			{
				player->gotflag |= MF_REDFLAG;
				rflagpoint = special->spawnpoint;
				S_StartSound (player->mo, sfx_lvpass);
				P_SetMobjState(special, S_DISS);
				CONS_Printf("%s picked up the red flag!\n", player_names[player-players]);
				player->gliding = 0;
				player->climbing = 0;
				if (player->powers[pw_tailsfly])
					player->powers[pw_tailsfly] = 1;
			}
			return;
	
		case MT_BLUEFLAG:
			if (player->powers[pw_flashing])
				return;
			if (!special->spawnpoint)
				return;
			if (special->fuse == 1)
				return;
			if (special->momz > 0)
				return;
			if (player->ctfteam == 2) // Player is on the Blue Team
			{
				short spawnheight;

				if (!special->spawnpoint->z)
					spawnheight = (short)(special->floorz >> FRACBITS);
				else
					spawnheight = special->spawnpoint->z;

				if (special->x >> FRACBITS != special->spawnpoint->x
				   || special->y >> FRACBITS != special->spawnpoint->y
				   || special->z >> FRACBITS != spawnheight)
					{
						special->fuse = 1;

						if (player->specialsector != 989)
						{
							CONS_Printf("%s returned the blue flag to base.\n", player_names[player-players]);

							if (players[consoleplayer].ctfteam == player->ctfteam)
								S_StartSound(NULL, sfx_hoop1);
						}
					}
			}
			else if (player->ctfteam == 1) // Player is on the Red Team
			{
				player->gotflag |= MF_BLUEFLAG;
				bflagpoint = special->spawnpoint;
				S_StartSound (player->mo, sfx_lvpass);
				P_SetMobjState(special, S_DISS);
				CONS_Printf("%s picked up the blue flag!\n", player_names[player-players]);
				player->gliding = 0;
				player->climbing = 0;
				if (player->powers[pw_tailsfly])
					player->powers[pw_tailsfly] = 1;
			}
			return;
	
		case MT_DISS:
			break;
	
		case MT_TOKEN: // Tails 08-18-2001
			P_SetMobjState(special, S_DISS);
			return; // Tails 08-18-2001
	
		default:
			break;
		}
	}

	P_SetMobjState(special, S_DISS);
	special->flags &= ~MF_NOBLOCKMAP;

	if (sound != sfx_None)
		S_StartSound(player->mo, sound); // was NULL, but changed to player so you could hear others pick up rings
}

#define CTFTEAMCODE(pl) pl->ctfteam ? (pl->ctfteam == 1 ? "\x85" : "\x84") : ""
#define CTFTEAMENDCODE(pl) pl->ctfteam ? "\x80" : ""

/** Prints death messages relating to a dying player.
  *
  * \param target    Dying mobj (if it is not a player, nothing will happen).
  * \param inflictor The attack weapon used, can be NULL.
  * \param source    The attacker, can be NULL.
  * \todo Clean up, refactor.
  * \sa P_KillMobj
  */
static void P_DeathMessages(mobj_t *target, mobj_t *inflictor, mobj_t *source)
{
	int w;
	const char *str;

	if (gametype == GT_COOP || gametype == GT_RACE)
		return;

	if (!target || !target->player)
		return;

	str = "%s%s%s died.\n";

	// death message decision structure redone by Orospakr
	if (source)
	{
		// inflictor shouldn't be NULL if source isn't
		I_Assert(inflictor != NULL);
		if ((inflictor->flags & MF_PUSHABLE) && source->player)
		{
			CONS_Printf("%s%s%s crushed %s%s%s with a heavy object!\n",
				CTFTEAMCODE(source->player),
				player_names[source->player - players],
				CTFTEAMENDCODE(source->player),
				CTFTEAMCODE(target->player),
				player_names[target->player - players],
				CTFTEAMENDCODE(target->player));
			return;
		}
		else if (source->player)
		{
			if (source->player == target->player)
			{
				CONS_Printf("%s%s%s suicides\n",
					CTFTEAMCODE(target->player),
					player_names[target->player - players],
					CTFTEAMENDCODE(target->player));
				return;
			}
			else if (inflictor)
			{
				switch (inflictor->type)
				{
					case MT_SPINFIRE:
						str = "%s%s%s was fried by %s%s%s's fire trail.\n";
						break;
					case MT_THROWNAUTOMATICEXPLOSIONHOMING:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's automatic-explosion-homing-rail ring.\n";
						else
							str = "%s%s%s was killed by %s%s%s's automatic-explosion-homing ring.\n";
						break;
					case MT_THROWNAUTOMATICEXPLOSION:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's automatic-explosion-rail ring.\n";
						else						
							str = "%s%s%s was killed by %s%s%s's automatic-explosion ring.\n";
						break;
					case MT_THROWNAUTOMATICHOMING:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's automatic-homing-rail ring.\n";
						else
							str = "%s%s%s was killed by %s%s%s's automatic-homing ring.\n";
						break;
					case MT_THROWNHOMINGEXPLOSION:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's explosion-homing-rail ring.\n";
						else
							str = "%s%s%s was killed by %s%s%s's homing-explosion ring.\n";
						break;
					case MT_THROWNHOMING:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's homing-rail ring.\n";
						else
							str = "%s%s%s was killed by %s%s%s's homing ring.\n";
						break;
					case MT_THROWNAUTOMATIC:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's automatic-rail ring.\n";
						else
							str = "%s%s%s was killed by %s%s%s's automatic ring.\n";
						break;
					case MT_THROWNEXPLOSION:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's rail-explosion ring.\n";
						else
							str = "%s%s%s was killed by %s%s%s's explosion ring.\n";
						break;
					case MT_REDRING:
						if (inflictor->flags2 & MF2_RAILRING)
							str = "%s%s%s was killed by %s%s%s's rail ring.\n";
						else if (inflictor->flags2 & MF2_DEBRIS)
							str = "%s%s%s was killed by %s%s%s's explosion ring shrapnel.\n";
						else
							str = "%s%s%s was killed by %s%s%s's ring.\n";
						break;
					
					default:
						str = "%s%s%s was killed by %s%s%s\n";
						break;
				}
				CONS_Printf(str,
					CTFTEAMCODE(target->player),
					player_names[target->player - players],
					CTFTEAMENDCODE(target->player),
					CTFTEAMCODE(source->player),
					player_names[source->player - players],
					CTFTEAMENDCODE(source->player));
				return;
			}
			else
			{
				str = "%s%s%s was killed by %s%s%s\n";

				CONS_Printf(str,
					CTFTEAMCODE(target->player),
					player_names[target->player - players],
					CTFTEAMENDCODE(target->player),
					CTFTEAMCODE(source->player),
					player_names[source->player - players],
					CTFTEAMENDCODE(source->player));
			}
			return;
		}
		else
		{
			switch (source->type)
			{
				case MT_DISS:
					if (source->threshold == 42)
						str = "%s%s%s drowned.\n";
					else if (source->threshold == 43)
						str = "%s%s%s was crushed.\n";
					break;
				case MT_BLUECRAWLA:
					if (netgame)
						str = "%s%s%s was killed by a blue crawla!\n";
					break;
				case MT_REDCRAWLA:
					if (netgame)
						str = "%s%s%s was killed by a red crawla!\n";
					break;
				case MT_JETTGUNNER:
					if (netgame)
						str = "%s%s%s was killed by a jetty-syn gunner!\n";
					break;
				case MT_JETTBOMBER:
					if (netgame)
						str = "%s%s%s was killed by a jetty-syn bomber!\n";
					break;
				case MT_CRAWLACOMMANDER:
					if (netgame)
						str = "%s%s%s was killed by a crawla commander!\n";
					break;
				case MT_EGGMOBILE:
					if (netgame)
						str = "%s%s%s was killed by the Egg Mobile!\n";
					break;
				case MT_EGGMOBILE2:
					if (netgame)
						str = "%s%s%s was killed by the Egg Slimer!\n";
					break;
				default:
					break;
			}
		}
	}
	else
	{ // source is NULL
		// environment kills
		w = target->player->specialsector; //see p_spec.c

		if (w == 5 || w == 16)
			str = "%s%s%s fell into a bottomless pit.\n";
		else if (w == 7)
			str = "%s%s%s fell in some nasty goop!\n";
	}

	CONS_Printf(str,
		CTFTEAMCODE(target->player),
		player_names[target->player - players],
		CTFTEAMENDCODE(target->player));

	if ((gametype == GT_COOP || gametype == GT_RACE))
	{
		if (target->player->lives - 1 <= 0)
			CONS_Printf("%s got a game over.\n",player_names[target->player-players]);
		else
			CONS_Printf("%s has %d lives remaining.\n", player_names[target->player-players], target->player->lives);
	}
}

//
// P_HitMessages
//
// Like P_DeathMessages, but for when you don't die.
//
static void P_HitMessages(mobj_t *target, mobj_t *inflictor, mobj_t *source)
{
	const char *str;

	if (!target || !target->player)
		return;

	if (!source || !source->player)
		return;

	str = "%s%s%s was hit by %s%s%s\n";

	if (source->player->blackow == 3)
		str = "%s%s%s got nuked by %s%s%s!\n";
	else if (inflictor)
	{
		switch (inflictor->type)
		{
			case MT_SPINFIRE:
				str = "%s%s%s was burnt by %s%s%s's fire trail.\n";
				break;
			case MT_THROWNAUTOMATICEXPLOSIONHOMING:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's automatic-explosion-homing-rail ring.\n";
				else
					str = "%s%s%s was hit by %s%s%s's automatic-explosion-homing ring.\n";
				break;
			case MT_THROWNAUTOMATICEXPLOSION:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's automatic-explosion-rail ring.\n";
				else						
					str = "%s%s%s was hit by %s%s%s's automatic-explosion ring.\n";
				break;
			case MT_THROWNAUTOMATICHOMING:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's automatic-homing-rail ring.\n";
				else
					str = "%s%s%s was hit by %s%s%s's automatic-homing ring.\n";
				break;
			case MT_THROWNHOMINGEXPLOSION:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's explosion-homing-rail ring.\n";
				else
					str = "%s%s%s was hit by %s%s%s's homing-explosion ring.\n";
				break;
			case MT_THROWNHOMING:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's homing-rail ring.\n";
				else
					str = "%s%s%s was hit by %s%s%s's homing ring.\n";
				break;
			case MT_THROWNAUTOMATIC:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's automatic-rail ring.\n";
				else
					str = "%s%s%s was hit by %s%s%s's automatic ring.\n";
				break;
			case MT_THROWNEXPLOSION:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's rail-explosion ring.\n";
				else
					str = "%s%s%s was hit by %s%s%s's explosion ring.\n";
				break;
			case MT_REDRING:
				if (inflictor->flags2 & MF2_RAILRING)
					str = "%s%s%s was hit by %s%s%s's rail ring.\n";
				else if (inflictor->flags2 & MF2_DEBRIS)
					str = "%s%s%s was hit by %s%s%s's explosion ring shrapnel.\n";
				else
					str = "%s%s%s was hit by %s%s%s's ring.\n";
				break;
			default:
				break;
		}
	}

	CONS_Printf(str, CTFTEAMCODE(target->player), player_names[target->player-players],
		CTFTEAMENDCODE(target->player), CTFTEAMCODE(source->player),
		player_names[source->player-players], CTFTEAMENDCODE(source->player));

	return;
}

/** Checks if a player's score is over the pointlimit and the round should end.
  * Verify that the value of ::cv_pointlimit is greater than zero before
  * calling this function.
  *
  * \param p Player to check score of. This is ignored in CTF games, so you may
  *          pass NULL in that case.
  * \sa cv_pointlimit, P_UpdateSpecials
  */
void P_CheckPointLimit(player_t *p)
{
	if (!cv_pointlimit.value)
		return;

	// pointlimit is nonzero, check if it's been reached by this player
	if (gametype == GT_CTF)
	{
		// Just check both teams
		if (cv_pointlimit.value <= redscore || cv_pointlimit.value <= bluescore)
			G_ExitLevel();
	}
	else if (gametype == GT_TAG)
	{
		if (cv_pointlimit.value <= p->tagcount)
			G_ExitLevel();
	}
	else if (cv_pointlimit.value <= p->score)
		G_ExitLevel();
}

/** Kills an object.
  *
  * \param target    The victim.
  * \param inflictor The attack weapon. May be NULL (environmental damage).
  * \param source    The attacker. May be NULL.
  * \todo Cleanup, refactor, split up.
  * \sa P_DamageMobj, P_DeathMessages
  */
void P_KillMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source)
{
	mobjtype_t item;
	mobj_t *mo;

	if (mariomode && inflictor && (inflictor->type == MT_SHELL || inflictor->type == MT_FIREBALL))
		target->tracer = inflictor;

	// dead target is no more shootable
	target->flags &= ~MF_SHOOTABLE;
	target->flags2 &= ~MF2_SKULLFLY;
	target->flags &= ~MF_FLOAT;

	if (target->flags & MF_BOSS)
		target->momx = target->momy = target->momz = 0;
	else if (target->flags & MF_ENEMY)
		target->momz = 0;

	if (target->type != MT_PLAYER && !(target->flags & MF_MONITOR))
		target->flags |= MF_NOGRAVITY; // Don't drop Tails 03-08-2000

	// Let EVERYONE know what happened to a player! 01-29-2002 Tails
	if (target->player && !(gametype == GT_CTF && !target->player->ctfteam))
	{
#ifdef CHAOSISNOTDEADYET
		if (gametype == GT_CHAOS)
			target->player->score /= 2; // Halve the player's score in Chaos Mode
		else
#endif
		if ((gametype == GT_MATCH || gametype == GT_TAG)
			&& ((target == source) || (source == NULL && inflictor == NULL) || (source && !source->player))
			&& target->player->score >= 50 && cv_match_scoring.value == 0) // Suicide penalty
			target->player->score -= 50;

		P_DeathMessages(target, inflictor, source);

		target->flags2 &= ~MF2_DONTDRAW;
	}

	// if killed by a player
	if (source && source->player)
	{
		if (target->flags & MF_MONITOR)
		{
			target->target = source;
			source->player->numboxes++;
			if ((cv_itemrespawn.value && (modifiedgame || netgame || multiplayer)))
				target->fuse = cv_itemrespawntime.value*TICRATE + 2; // Random box generation
		}

		// Time Attacked Thingy Tails 12-14-2003
		if (cv_timeattacked.value)
		{
			source->player->linkcount++;
			source->player->linktimer = 2*TICRATE;
		}

		// Award Score Tails
		{
			int score = 0;

#ifdef CHAOSISNOTDEADYET
			if (gametype == GT_CHAOS)
			{
				if ((target->flags & MF_ENEMY)
					&& !(target->flags & MF_MISSILE))
					source->player->scoreadd++;

				switch (target->type)
				{
					case MT_BLUECRAWLA:
					case MT_GOOMBA:
						score = 100*source->player->scoreadd;
						break;
					case MT_REDCRAWLA:
					case MT_BLUEGOOMBA:
						score = 150*source->player->scoreadd;
						break;
					case MT_JETTBOMBER:
						score = 400*source->player->scoreadd;
						break;
					case MT_JETTGUNNER:
						score = 500*source->player->scoreadd;
						break;
					case MT_CRAWLACOMMANDER:
						score = 300*source->player->scoreadd;
						break;
					default:
						score = 100*source->player->scoreadd;
						break;
				}
			}
			else
#endif
			{
				if (target->flags & MF_BOSS)
					score = 1000;
				else if ((target->flags & MF_ENEMY)
					&& !(target->flags & MF_MISSILE))
				{
					mobj_t *scoremobj;

					source->player->scoreadd++; // Tails 11-03-2000
					if (source->player->scoreadd == 1)
					{
						score = 100; // Score! Tails 03-01-2000
						scoremobj = P_SpawnMobj(target->x, target->y, target->z + (target->height / 2), MT_SCRA);

						if (scoremobj)
							P_SetMobjState(scoremobj, scoremobj->info->spawnstate);
					}
					if (source->player->scoreadd == 2)
					{
						score = 200; // Score! Tails 03-01-2000
						scoremobj = P_SpawnMobj(target->x, target->y, target->z + (target->height / 2), MT_SCRB);

						if (scoremobj)
							P_SetMobjState(scoremobj, scoremobj->info->spawnstate);
					}
					if (source->player->scoreadd == 3)
					{
						score = 500; // Score! Tails 03-01-2000
						scoremobj = P_SpawnMobj(target->x, target->y, target->z + (target->height / 2), MT_SCRC);

						if (scoremobj)
							P_SetMobjState(scoremobj, scoremobj->info->spawnstate);
					}
					if (source->player->scoreadd >= 4)
					{
						score = 1000; // Score! Tails 03-01-2000
						scoremobj = P_SpawnMobj(target->x, target->y, target->z + (target->height / 2), MT_SCRD);

						if (scoremobj)
							P_SetMobjState(scoremobj, scoremobj->info->spawnstate);
					}
				}
			}

			P_AddPlayerScore(source->player, score);
		}
	}

	// if a player avatar dies...
	if (target->player)
	{
		target->flags &= ~MF_SOLID; // does not block

		if (gametype == GT_COOP || gametype == GT_RACE) // Coop and race only Graue 12-13-2003
		{
			target->player->lives -= 1; // Lose a life Tails 03-11-2000

			if (target->player->lives <= 0) // Tails 03-14-2000
			{
				if (P_IsLocalPlayer(target->player))
				{
					S_StopMusic(); // Stop the Music! Tails 03-14-2000
					S_ChangeMusic(mus_gmover, false); // Yousa dead now, Okieday? Tails 03-14-2000
				}
			}
		}
		target->player->playerstate = PST_DEAD;

		if (target->player == &players[consoleplayer])
		{
			// don't die in auto map,
			// switch view prior to dying
			if (automapactive)
				AM_Stop();

			//added : 22-02-98: recenter view for next life...
			localaiming = 0;
		}
		if (cv_splitscreen.value && target->player == &players[secondarydisplayplayer])
		{
			// added : 22-02-98: recenter view for next life...
			localaiming2 = 0;
		}

		// You died via pit/drowning, so now you become "IT".
		if (gametype == GT_TAG && !target->player->tagit && (!source || !source->player))
		{
			int w;

			for (w = 0; w < MAXPLAYERS; w++)
				players[w].tagit = 0;

			target->player->tagit = 300*TICRATE + 1;
			CONS_Printf("%s is it!\n", player_names[target->player-players]); // Tell everyone who is it!
		}
	}

	if (target->player)
		P_SetPlayerMobjState(target, target->info->deathstate);
	else
		P_SetMobjState(target, target->info->deathstate);
	
	/** \note For player, the above is redundant because of P_SetMobjState (target, S_PLAY_DIE1)
	   in P_DamageMobj()
	   Graue 12-22-2003 */

	if (source && target && target->player && source->player)
		P_PlayVictorySound(source); // Killer laughs at you. LAUGHS! BWAHAHAHA!

	if (target->tics < 1)
		target->tics = 1;

	if (mariomode // Don't show birds, etc. in Mario Mode Tails 12-23-2001
#ifdef CHAOSISNOTDEADYET
		|| gametype == GT_CHAOS // Or Chaos Mode!
#endif
		) 
		return;

	// Drop stuff.
	// This determines the kind of object spawned
	// during the death frame of a thing.
	if (target->flags & MF_ENEMY)
	{
		if (cv_soniccd.value)
			item = MT_SEED;
		else
		{
			int prandom;

			switch (target->type)
			{
				case MT_REDCRAWLA:
				case MT_GOLDBUZZ:
				case MT_SKIM:
					item = MT_SQRL;
					break;

				case MT_BLUECRAWLA:
				case MT_JETTBOMBER:
				case MT_GFZFISH:
					item = MT_BIRD;
					break;

				case MT_JETTGUNNER:
				case MT_CRAWLACOMMANDER:
				case MT_REDBUZZ:
					item = MT_MOUSE;
					break;

				default:
					prandom = P_Random();

					if (prandom < 86)
						item = MT_SQRL;
					else if (prandom < 172)
						item = MT_BIRD;
					else
						item = MT_MOUSE;

					break;
			}
		}

		mo = P_SpawnMobj(target->x, target->y, target->z + (target->height / 2), item);
	}
}

/** Plays a player's ringloss sound.
  * One of four sound effects is chosen at random.
  *
  * \param source Object whose ringloss sound will be played.
  * \todo Generalize this and the other three random sound functions.
  * \sa P_PlayDeathSound, P_PlayVictorySound, P_PlayTauntSound
  * \author SSNTails <http://www.ssntails.org>
  */
void P_PlayRinglossSound(mobj_t *source)
{
	int prandom;

	prandom = P_Random();

	if (prandom <= 63)
		S_StartSound(source, sfx_altow1);
	else if (prandom <= 127)
		S_StartSound(source, sfx_altow2);
	else if (prandom <= 191)
		S_StartSound(source, sfx_altow3);
	else
		S_StartSound(source, sfx_altow4);
}

/** Plays an object's death sound.
  * Used when you kick the bucket, buy the farm, etc. One of four sound effects
  * is chosen at random.
  *
  * \param source Object whose death sound will be played.
  * \todo Generalize this and the other three random sound functions.
  * \sa P_PlayRinglossSound, P_PlayVictorySound, P_PlayTauntSound
  * \author SSNTails <http://www.ssntails.org>
  */
void P_PlayDeathSound(mobj_t *source)
{
	int prandom;

	prandom = P_Random();

	if (prandom <= 63)
		S_StartSound(source, sfx_altdi1);
	else if (prandom <= 127)
		S_StartSound(source, sfx_altdi2);
	else if (prandom <= 191)
		S_StartSound(source, sfx_altdi3);
	else
		S_StartSound(source, sfx_altdi4);
}

/** Plays an object's victory sound.
  * Used when a player kills another in multiplayer. One of four sound effects
  * is chosen at random.
  *
  * \param source Object whose victory sound will be played.
  * \todo Generalize this and the other three random sound functions.
  * \sa P_PlayRinglossSound, P_PlayDeathSound, P_PlayTauntSound
  * \author SSNTails <http://www.ssntails.org>
  */
void P_PlayVictorySound(mobj_t *source)
{
	int prandom;

	prandom = P_Random();

	if (prandom <= 63)
		S_StartSound(source, sfx_victr1);
	else if (prandom <= 127)
		S_StartSound(source, sfx_victr2);
	else if (prandom <= 191)
		S_StartSound(source, sfx_victr3);
	else
		S_StartSound(source, sfx_victr4);
}

/** Plays an object's taunt sound.
  * Used when a player hits the taunt key. One of four sound effects is chosen
  * at random.
  *
  * \param source Object whose taunt sound will be played.
  * \todo Generalize this and the other three random sound functions.
  * \sa P_PlayRinglossSound, P_PlayDeathSound, P_PlayVictorySound
  * \author SSNTails <http://www.ssntails.org>
  */
void P_PlayTauntSound(mobj_t *source)
{
	int prandom;

	prandom = P_Random();

	if (prandom <= 63)
		S_StartSound(source, sfx_taunt1);
	else if (prandom <= 127)
		S_StartSound(source, sfx_taunt2);
	else if (prandom <= 191)
		S_StartSound(source, sfx_taunt3);
	else
		S_StartSound(source, sfx_taunt4);

	// In the future... taunt animation?
}

/** Damages an object, which may or may not be a player.
  * For melee attacks, source and inflictor are the same.
  *
  * \param target    The object being damaged.
  * \param inflictor The thing that caused the damage: creature, missile,
  *                  gargoyle, and so forth. Can be NULL in the case of
  *                  environmental damage, such as slime or crushing.
  * \param source    The creature or person responsible. For example, if a
  *                  player is hit by a ring, the player who shot it. In some
  *                  cases, the target will go after this object after
  *                  receiving damage. This can be NULL.
  * \param damage    Amount of damage to be dealt. 10000 is instant death.
  * \return True if the target sustained damage, otherwise false.
  * \todo Clean up this mess, split into multiple functions.
  * \todo Get rid of the magic number 10000.
  * \sa P_KillMobj
  */
boolean P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, int damage)
{
	angle_t ang;
	player_t *player;

	if (cv_objectplace.value)
		return false;

	if (!(target->flags & MF_SHOOTABLE))
		return false; // shouldn't happen...

	if (target->player && target->player->nightsmode && source == target)
		return false;

	if (target->player && !target->player->nightsmode && !target->player->nightsfall && (maptol & TOL_NIGHTS))
		return false;

	// Make sure that boxes cannot be popped by enemies, red rings, etc.
	if (target->flags & MF_MONITOR && ((!source || !source->player)
		|| (inflictor && !inflictor->player)))
	{
		return false;
	}

	if (target->health <= 0)
		return false;

	if (target->flags2 & MF2_SKULLFLY)
		target->momx = target->momy = target->momz = 0;

	if (target->flags & MF_BOSS)
	{
		if (target->flags2 & MF2_FRET)
			return false;
	}

	if (gametype == GT_CTF)
	{
		if (damage == 42000 && target->player && !target->player->ctfteam)
			damage = 10000;
		else if (target->player && !target->player->ctfteam)
			return false;
		
		if (source && source->player && !source->player->ctfteam)
			return false;
	}

	// Special case for Crawla Commander
	if (target->type == MT_CRAWLACOMMANDER)
	{
		if (target->fuse) // Invincible
			return false;

		if (target->health > 1)
		{

#ifdef CHAOSISNOTDEADYET
			if (gametype == GT_CHAOS && source && source->player)
			{
				source->player->scoreadd++;
				P_AddPlayerScore(source->player, 300*source->player->scoreadd);
			}
#endif
			if (target->info->painsound) S_StartSound(target, target->info->painsound);
			target->fuse = TICRATE/2;
			target->flags2 |= MF2_FRET;
		}
		else
		{
			target->flags |= MF_NOGRAVITY;
			target->fuse = 0;
		}

		target->momx = target->momy = target->momz = 0;

		P_InstaThrust(target, target->angle-ANG180, 5*FRACUNIT);
	}
	else if (target->flags & MF_BOSS)
	{
		if (target->health > 1)
		{
			target->flags2 |= MF2_FRET;
			target->flags |= MF_TRANSLATION;
		}


#ifdef CHAOSISNOTDEADYET
		if (gametype == GT_CHAOS && source && source->player)
		{
			source->player->scoreadd++;
			P_AddPlayerScore(source->player, 300*source->player->scoreadd);
		}
#endif
	}

	player = target->player;

	if (player && player->exiting)
		return false;

	if (player) // Player is the target
	{
		if (player->nightsmode) // NiGHTS damage handling
		{
			if (!player->powers[pw_flashing]
				&& !(player->cheats & CF_GODMODE))
			{
				angle_t fa;

				P_UnsetThingPosition(target);
				player->angle_pos = player->old_angle_pos;
				player->speed /= 5;
				player->flyangle += 180;
				player->flyangle %= 360;

				if (gametype == GT_RACE)
					player->drillmeter -= 5*20;
				else
				{
					if (source && source->player)
					{
						if (player->nightstime > 20)
							player->nightstime -= 20;
						else
							player->nightstime = 1;
					}
					else
					{
						if (player->nightstime > 5)
							player->nightstime -= 5;
						else
							player->nightstime = 1;
					}
				}

				if (player->transfertoclosest)
				{
					target->x -= target->momx;
					target->y -= target->momy;
				}
				else
				{
					fa = player->old_angle_pos>>ANGLETOFINESHIFT;

					target->x = target->target->x + FixedMul(finecosine[fa],target->target->radius);
					target->y = target->target->y + FixedMul(finesine[fa],target->target->radius);
				}

				target->momx = target->momy = 0;
				P_SetThingPosition(target);
				player->powers[pw_flashing] = flashingtics;
				P_SetMobjState(target->tracer, S_NIGHTSHURT1);
				S_StartSound(target, sfx_nghurt);
			}
			return true;
		}

		if (inflictor && (inflictor->flags & MF_FIRE))
		{
			if (player->powers[pw_fireshield])
				return false; // Invincible to fire objects

			if ((gametype == GT_COOP || gametype == GT_RACE) && source && source->player)
				return false; // Don't get hurt by fire generated from friends.
		}

		if (source && source->type == MT_PLAYER)
		{
			if ((gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF) && cv_suddendeath.value
				&& !player->powers[pw_flashing] && !player->powers[pw_invulnerability])
				damage = 10000; // Instant-death!
		}

		if (source && source->player) // Player hits another player
		{

			if (source == target) // You can't kill yourself, idiot...
				return false;

			if (!cv_friendlyfire.value && (gametype == GT_COOP ||
				gametype == GT_RACE 
#ifdef CHAOSISNOTDEADYET
				|| gametype == GT_CHAOS
#endif
				))
				return false;

			if (gametype == GT_TAG) // Tag Mode!
			{
				int i;

				// If flashing, or in the tag zone, or invulnerable, ignore the tag.
				if (target->player->powers[pw_flashing] || target->player->tagzone || target->player->powers[pw_invulnerability])
					return false;

				if ((player->health-damage) > 0)
					P_HitMessages(target, inflictor, source);

				// Make the player IT!
				if (source->player->tagit < 298*TICRATE && source->player->tagit > 0)
				{
					target->player->tagit = 300*TICRATE + 1;
					source->player->tagit = 0;
				}

				// Award points to all those who didn't get tagged.
				for (i = 0; i < MAXPLAYERS; i++)
				{
					if (playeringame[i])
					{
						if (&players[i] != source->player
							&& &players[i] != target->player)
							players[i].tagcount++;

						P_CheckPointLimit(&players[i]);
					}
				}

				target->z++;

				if (target->eflags & MF_UNDERWATER)
					target->momz = (10511*FRACUNIT)/2600;
				else
					target->momz = (69*FRACUNIT)/10;

				if (source->player->mfjumped && source->momz < 0) // Bounce tagger off target
					source->momz = -source->momz;

				ang = R_PointToAngle2(inflictor->x,	inflictor->y, target->x, target->y);

				P_InstaThrust(target, ang, 4*FRACUNIT);
				P_ResetPlayer(target->player);
				target->player->powers[pw_flashing] = flashingtics;
				P_SetPlayerMobjState(target, S_PLAY_PAIN);

				// Check for a shield
				if (target->player->powers[pw_fireshield] || target->player->powers[pw_jumpshield] || target->player->powers[pw_ringshield] || target->player->powers[pw_bombshield] || target->player->powers[pw_watershield])
				{
					target->player->powers[pw_fireshield] = false;
					target->player->powers[pw_jumpshield] = false; // Get rid of shield
					target->player->powers[pw_ringshield] = false;
					target->player->powers[pw_bombshield] = false;
					target->player->powers[pw_watershield] = false;
					S_StartSound(target, sfx_shldls);
					return true;
				}

				if (target->health <= 1) // Death
				{
					P_PlayDeathSound(target);
					P_PlayVictorySound(source); // Killer laughs at you! LAUGHS! BWAHAHAHHAHAA!!
				}
				else if (target->health > 1) // Ring loss
				{
					P_PlayRinglossSound(target);
					P_PlayerRingBurst(target->player, target->player->mo->health - 1);
				}

				target->player->health = target->health = 1;
				return true;
			}
			else if (gametype == GT_CTF) // CTF
			{
				// Don't allow players on the same team to hurt one another,
				// unless cv_teamdamage is on.
				if (!cv_teamdamage.value && target->player->ctfteam == source->player->ctfteam)
				{
					if (!(inflictor->flags & MF_FIRE))
						P_GivePlayerRings(target->player, 1, false);

					return false;
				}
			}
			else if (gametype == GT_MATCH && cv_teamplay.value) // Match
			{
				// Don't allow players on the same team to hurt one another,
				// unless cv_teamdamage is on.
				if (!cv_teamdamage.value)
				{
					if (cv_teamplay.value == 1) // Color
					{
						if (target->player->skincolor == source->player->skincolor)
						{
							if (!(inflictor->flags & MF_FIRE))
								P_GivePlayerRings(target->player, 1, false);

							return false;
						}
					}
					else if (cv_teamplay.value == 2) // Skin
					{
						if (target->player->skin == source->player->skin)
						{
							if (!(inflictor->flags & MF_FIRE))
								P_GivePlayerRings(target->player, 1, false);
							return false;
						}
					}
				}
			}

			if ((player->health-damage) > 0 && !(player->powers[pw_flashing] || player->powers[pw_invulnerability]))
				P_HitMessages(target, inflictor, source);
		}

		if (player->cheats & CF_GODMODE)
		{
			return false;
		}
		else if (damage == 10000)
		{
			player->powers[pw_fireshield] = false;
			player->powers[pw_jumpshield] = false; // Get rid of shield
			player->powers[pw_ringshield] = false;
			player->powers[pw_bombshield] = false;
			player->powers[pw_watershield] = false;
			player->mo->momx = player->mo->momy = player->mo->momz = 0;

			player->powers[pw_fireflower] = false;
			player->mo->flags =  (player->mo->flags & ~MF_TRANSLATION)
			                   | ((player->skincolor)<<MF_TRANSSHIFT);

			if (player->powers[pw_underwater] != 1) // Don't jump up when drowning
				player->mo->momz = 18*FRACUNIT;
			else
				player->mo->momz++;

			P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

			if (source && source->type == MT_DISS && source->threshold == 42) // drowned
				S_StartSound(target, sfx_drown);
			else
				P_PlayDeathSound(target);

			P_SetPlayerMobjState(target, target->info->deathstate);
			if (gametype == GT_CTF && (player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG))
			{
				P_PlayerFlagBurst(player, false);
				if (source && source->player)
					P_AddPlayerScore(source->player, 25);
			}
			if (source && source->player)
				P_AddPlayerScore(source->player, 100);
		}
		else if (damage < 10000 // ignore bouncing & such in invulnerability
			&& (player->powers[pw_invulnerability] || player->powers[pw_flashing]
			|| player->powers[pw_super]))
		{
			if (inflictor && (inflictor->flags & MF_MISSILE)
				&& (inflictor->flags2 & MF2_SUPERFIRE)
				&& player->powers[pw_super])
			{
				fixed_t fallbackspeed;

				P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);
				damage = 0; // Don't take rings away
				player->mo->z++;

				if (player->mo->eflags & MF_UNDERWATER)
					player->mo->momz = (10511*FRACUNIT)/2600;
				else
					player->mo->momz = (69*FRACUNIT)/10;

				ang = R_PointToAngle2(inflictor->x,	inflictor->y, target->x, target->y);

				// explosion and rail rings send you farther back, making it more difficult
				// to recover
				if (inflictor->flags2 & MF2_EXPLOSION)
				{
					if (inflictor->flags2 & MF2_RAILRING)
						fallbackspeed = 24*FRACUNIT; // 6x
					else
						fallbackspeed = 16*FRACUNIT; // 4x
				}
				else if (inflictor->flags2 & MF2_RAILRING)
					fallbackspeed = 12*FRACUNIT; // 3x
				else
					fallbackspeed = 4*FRACUNIT; // the usual amount of force

				P_InstaThrust(target, ang, fallbackspeed);

				if (player->charflags & SF_SUPERANIMS)
					P_SetPlayerMobjState(target, S_PLAY_SUPERHIT);
				else
					P_SetPlayerMobjState(target, S_PLAY_PAIN);

				P_ResetPlayer(player);
				return true;
			}
			else
				return false;
		}

		else if (damage < 10000 && (player->powers[pw_fireshield] || player->powers[pw_jumpshield] || player->powers[pw_ringshield] || player->powers[pw_bombshield] || player->powers[pw_watershield]))  //If One-Hit Shield
		{
			player->powers[pw_jumpshield] = false; // Get rid of shield
			player->powers[pw_ringshield] = false;
			player->powers[pw_watershield] = false;
			player->powers[pw_fireshield] = false;

			if (player->powers[pw_bombshield]) // Give them what's coming to them!
			{
				player->blackow = 1; // BAM!
				player->powers[pw_bombshield] = false;
				player->jumpdown = true;
			}
			P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);
			damage = 0; // Don't take rings away
			player->mo->z++;

			if (player->mo->eflags & MF_UNDERWATER)
				player->mo->momz = (10511*FRACUNIT)/2600;
			else
				player->mo->momz = (69*FRACUNIT)/10;

			if (inflictor == NULL)
				P_InstaThrust (player->mo, player->mo->angle-ANG180, 4*FRACUNIT);
			else
			{
				fixed_t fallbackspeed;

				ang = R_PointToAngle2(inflictor->x,	inflictor->y, target->x, target->y);

				// explosion and rail rings send you farther back, making it more difficult
				// to recover
				if (inflictor->flags2 & MF2_EXPLOSION)
				{
					if (inflictor->flags2 & MF2_RAILRING)
						fallbackspeed = 24*FRACUNIT; // 6x
					else
						fallbackspeed = 16*FRACUNIT; // 4x
				}
				else if (inflictor->flags2 & MF2_RAILRING)
					fallbackspeed = 12*FRACUNIT; // 3x
				else
					fallbackspeed = 4*FRACUNIT; // the usual amount of force

				P_InstaThrust(target, ang, fallbackspeed);
			}

			P_SetPlayerMobjState(target, target->info->painstate);
			target->player->powers[pw_flashing] = flashingtics;

			player->powers[pw_fireflower] = false;
			player->mo->flags = (player->mo->flags & ~MF_TRANSLATION)
								| ((player->skincolor)<<MF_TRANSSHIFT);

			P_ResetPlayer(player);

			if (source && (source->type == MT_DISS || source->type == MT_FLOORSPIKE || source->type == MT_CEILINGSPIKE) && source->threshold == 43) // spikes
				S_StartSound(target, sfx_spkdth);
			else
				S_StartSound (target, sfx_shldls); // Ba-Dum! Shield loss.

			if (gametype == GT_CTF && (player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG))
			{
				P_PlayerFlagBurst(player, false);
				if (source && source->player)
					P_AddPlayerScore(source->player, 25);
			}
			if (source && source->player)
				P_AddPlayerScore(source->player, cv_match_scoring.value == 1 ? 25 : 50);
			return true;
		}
		else if (player->mo->health > 1) // No shield but have rings.
		{
			damage = player->mo->health - 1;

			player->mo->z++;

			player->powers[pw_fireflower] = false;
			player->mo->flags =  (player->mo->flags & ~MF_TRANSLATION)
								| ((player->skincolor)<<MF_TRANSSHIFT);

			if (player->mo->eflags & MF_UNDERWATER)
				player->mo->momz = (10511*FRACUNIT)/2600;
			else
				player->mo->momz = (69*FRACUNIT)/10;

			if (inflictor == NULL)
				P_InstaThrust (player->mo, player->mo->angle-ANG180, 4*FRACUNIT);
			else
			{
				fixed_t fallbackspeed;

				ang = R_PointToAngle2(inflictor->x,	inflictor->y, target->x, target->y);

				// explosion and rail rings send you farther back, making it more difficult
				// to recover
				if (inflictor->flags2 & MF2_EXPLOSION)
				{
					if (inflictor->flags2 & MF2_RAILRING)
						fallbackspeed = 24*FRACUNIT; // 6x
					else
						fallbackspeed = 16*FRACUNIT; // 4x
				}
				else if (inflictor->flags2 & MF2_RAILRING)
					fallbackspeed = 12*FRACUNIT; // 3x
				else
					fallbackspeed = 4*FRACUNIT; // the usual amount of force

				P_InstaThrust(target, ang, fallbackspeed);
			}

			P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

			P_ResetPlayer(player);

			if (source && (source->type == MT_DISS || source->type == MT_FLOORSPIKE || source->type == MT_CEILINGSPIKE) && source->threshold == 43) // spikes
				S_StartSound(target, sfx_spkdth);

			// Ring loss sound plays despite hitting spikes
			P_PlayRinglossSound(target); // Ringledingle!

			if (gametype == GT_CTF && (player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG))
			{
				P_PlayerFlagBurst(player, false);
				if (source && source->player)
					P_AddPlayerScore(source->player, 25);
			}
			if (source && source->player)
				P_AddPlayerScore(source->player, 50);
		}
		else // No shield, no rings, no invincibility.
		{
			damage = 1;
			player->mo->momz = 18*FRACUNIT;
			player->mo->momx = player->mo->momy = 0;

			player->powers[pw_fireflower] = false;
			player->mo->flags =  (player->mo->flags & ~MF_TRANSLATION)
								| ((player->skincolor)<<MF_TRANSSHIFT);

			P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);

			P_ResetPlayer(player);

			if (source && (source->type == MT_DISS || source->type == MT_FLOORSPIKE || source->type == MT_CEILINGSPIKE) && source->threshold == 43) // Spikes
				S_StartSound(target, sfx_spkdth);
			else
				P_PlayDeathSound(target);

			if (gametype == GT_CTF && (player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG))
			{
				P_PlayerFlagBurst(player, false);
				if (source && source->player)
					P_AddPlayerScore(source->player, 25);
			}
			if (source && source->player)
				P_AddPlayerScore(source->player, 100);
		}
	}

	// player specific
	if (player)
	{
		player->health -= damage; // mirror mobj health here for Dave

		if (player->health < 0)
			player->health = 0;

		if (damage < 10000)
		{
			P_PlayerRingBurst(player, damage);
			target->player->powers[pw_flashing] = flashingtics;
		}

		P_ForceFeed(player, 40, 10, TICRATE, 40 + min(damage, 100)*2);
	}

	if (cv_killingdead.value && source && source->player && P_Random() < 192)
		P_DamageMobj(source, target, target, 1);

	// do the damage
	target->health -= damage;
	if (target->health <= 0)
	{
		P_KillMobj(target, inflictor, source);
		return true;
	}

	if (target->player)
		P_SetPlayerMobjState(target, target->info->painstate);
	else
		P_SetMobjState(target, target->info->painstate);

	if (target->player)
		P_ResetPlayer(target->player);

	target->reactiontime = 0; // we're awake now...

	if (source && source != target)
	{
		// if not intent on another player,
		// chase after this one
		target->target = source;
		target->threshold = BASETHRESHOLD;
		if (target->state == &states[target->info->spawnstate] && target->info->seestate != S_NULL)
		{
			if (target->player)
				P_SetPlayerMobjState(target, target->info->seestate);
			else
				P_SetMobjState(target, target->info->seestate);
		}
	}

	return true;
}

/** Spills an injured player's rings.
  *
  * \param player    The player who is losing rings.
  * \param num_rings Number of rings lost. A maximum of 32 rings will be
  *                  spawned.
  * \todo This function is too long. Clean up, factor, and possibly split it
  *       up.
  * \sa P_PlayerFlagBurst
  */
void P_PlayerRingBurst(player_t *player, int num_rings)
{
	int i;
	mobj_t *mo;
	byte randomangle;
	angle_t fa;
	fixed_t ns;

	// If no health, don't spawn ring!
	if (player->mo->health <= 1)
		return;

	// If in sk_nightmare or higher, don't spill rings.
	if (gameskill >= sk_nightmare && !(maptol & TOL_NIGHTS))
	{
		player->nightsfall = false;
		return;
	}

	if (num_rings > 32 && !player->nightsfall)
		num_rings = 32;

	for (i = 0; i < num_rings; i++)
	{
		if (player->powers[pw_infinityring])
		{
			mo = P_SpawnMobj(player->mo->x,
							player->mo->y,
							player->mo->z,
							MT_INFINITYRING);
			mo->health = player->powers[pw_infinityring];
			mo->flags2 |= MF2_DONTRESPAWN;
			mo->flags &= ~MF_NOGRAVITY;
			mo->flags2 &= ~MF2_NOCLIPHEIGHT;
			player->powers[pw_infinityring] = 0;
			mo->fuse = 12*TICRATE;
		}
		else if (player->powers[pw_homingring])
		{
			mo = P_SpawnMobj(player->mo->x,
							player->mo->y,
							player->mo->z,
							MT_HOMINGRING);
			mo->health = player->powers[pw_homingring];
			mo->flags2 |= MF2_DONTRESPAWN;
			mo->flags &= ~MF_NOGRAVITY;
			mo->flags2 &= ~MF2_NOCLIPHEIGHT;
			player->powers[pw_homingring] = 0;
			mo->fuse = 12*TICRATE;
		}
		else if (player->powers[pw_railring])
		{
			mo = P_SpawnMobj(player->mo->x,
							player->mo->y,
							player->mo->z,
							MT_RAILRING);
			mo->health = player->powers[pw_railring];
			mo->flags2 |= MF2_DONTRESPAWN;
			mo->flags &= ~MF_NOGRAVITY;
			mo->flags2 &= ~MF2_NOCLIPHEIGHT;
			player->powers[pw_railring] = 0;
			mo->fuse = 12*TICRATE;
		}
		else if (player->powers[pw_automaticring])
		{
			mo = P_SpawnMobj(player->mo->x,
							player->mo->y,
							player->mo->z,
							MT_AUTOMATICRING);
			mo->health = player->powers[pw_automaticring];
			mo->flags2 |= MF2_DONTRESPAWN;
			mo->flags &= ~MF_NOGRAVITY;
			mo->flags2 &= ~MF2_NOCLIPHEIGHT;
			player->powers[pw_automaticring] = 0;
			mo->fuse = 12*TICRATE;
		}
		else if (player->powers[pw_explosionring])
		{
			mo = P_SpawnMobj(player->mo->x,
							player->mo->y,
							player->mo->z,
							MT_EXPLOSIONRING);
			mo->health = player->powers[pw_explosionring];
			mo->flags2 |= MF2_DONTRESPAWN;
			mo->flags &= ~MF_NOGRAVITY;
			mo->flags2 &= ~MF2_NOCLIPHEIGHT;
			player->powers[pw_explosionring] = 0;
			mo->fuse = 12*TICRATE;
		}
		else if (mariomode)
		{
			mo = P_SpawnMobj(player->mo->x,
							 player->mo->y,
							 player->mo->z,
							 MT_FLINGCOIN);

			mo->fuse = (8-player->losscount)*TICRATE;
		}
		else
		{
			mo = P_SpawnMobj(player->mo->x,
							 player->mo->y,
							 player->mo->z,
							 MT_FLINGRING);

			mo->fuse = (8-player->losscount)*TICRATE;
		}

		randomangle = P_Random();

		fa = (randomangle+i*FINEANGLES/16) & FINEMASK;

		// Make rings spill out around the player in 16 directions like SA, but spill like Sonic 2.
		// Technically a non-SA way of spilling rings. They just so happen to be a little similar.
		if (player->nightsfall)
		{
			ns = ((i*FRACUNIT)/16)+2*FRACUNIT;
			mo->momx = FixedMul(finesine[fa],ns);

			if (!twodlevel)
				mo->momy = FixedMul(finecosine[fa],ns);

			mo->momz = 8*FRACUNIT;
			mo->fuse = 20*TICRATE; // Adjust fuse for NiGHTS
		}
		else
		{
			if (i > 15)
			{
				ns = 3 * FRACUNIT;
				mo->momx = FixedMul(finesine[fa],ns);

				if (!twodlevel)
					mo->momy = FixedMul(finecosine[fa],ns);

				mo->momz = 4*FRACUNIT;

				if ((i&1) && !(maptol & TOL_ADVENTURE))
					mo->momz += 4*FRACUNIT;
			}
			else
			{
				ns = 2 * FRACUNIT;
				mo->momx = FixedMul(finesine[fa], ns);

				if (!twodlevel)
					mo->momy = FixedMul(finecosine[fa],ns);

				mo->momz = 3*FRACUNIT;

				if ((i&1) && !(maptol & TOL_ADVENTURE))
					mo->momz += 3*FRACUNIT;
			}
		}
	}

	if (gameskill >= sk_medium)
	{
		player->losscount += 2;

		if (player->losscount > 6) // Don't go over 6.
			player->losscount = 6;
	}

	player->nightsfall = false;
	return;
}

/** Makes an injured or dead player lose possession of the flag.
  *
  * \param player The player with the flag, about to lose it.
  * \sa P_PlayerRingBurst
  */
void P_PlayerFlagBurst(player_t *player, boolean toss)
{
	mobj_t *flag;
	angle_t fa;
	mobjtype_t type;

	if (!(player->gotflag & MF_REDFLAG || player->gotflag & MF_BLUEFLAG))
		return;

	if (player->gotflag & MF_REDFLAG)
		type = MT_REDFLAG;
	else
		type = MT_BLUEFLAG;

	flag = P_SpawnMobj(player->mo->x, player->mo->y, player->mo->z, type);

	if (toss)
		fa = player->mo->angle>>ANGLETOFINESHIFT;
	else
		fa = P_Random()*FINEANGLES/256;

	flag->momx = FixedMul(finecosine[fa], (6 * FRACUNIT));

	if (toss)
		fa = player->mo->angle>>ANGLETOFINESHIFT;
	else
		fa = P_Random()*FINEANGLES/256;

	flag->momy = FixedMul(finesine[fa], (6 * FRACUNIT));
	flag->momz = 8*FRACUNIT;

	if (type == MT_REDFLAG)
	{
		flag->spawnpoint = rflagpoint;
		rflagpoint = NULL;
	}
	else
	{
		flag->spawnpoint = bflagpoint;
		bflagpoint = NULL;
	}

	flag->fuse = cv_flagtime.value * TICRATE;

	if (toss)
		CONS_Printf("%s tossed the %s flag.\n", player_names[player-players], (type == MT_REDFLAG ? "red" : "blue"));
	else
		CONS_Printf("%s dropped the %s flag.\n", player_names[player-players], (type == MT_REDFLAG ? "red" : "blue"));

	player->gotflag = 0;
	return;
}
