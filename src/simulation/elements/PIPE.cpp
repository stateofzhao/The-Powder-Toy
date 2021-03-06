/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simulation/ElementsCommon.h"
#include "simulation/elements/PPIP.h"
#include "simulation/elements/PRTI.h"
#include "graphics.h"

#define PFLAG_NORMALSPEED 0x00010000
#define PFLAG_REVERSE 0x00020000

// parts[].tmp flags
// trigger flags to be processed this frame (trigger flags for next frame are shifted 3 bits to the left):
#define PPIP_TMPFLAG_TRIGGER_ON 0x10000000
#define PPIP_TMPFLAG_TRIGGER_OFF 0x08000000
#define PPIP_TMPFLAG_TRIGGER_REVERSE 0x04000000
#define PPIP_TMPFLAG_TRIGGERS 0x1C000000
// current status of the pipe
#define PPIP_TMPFLAG_PAUSED 0x02000000
#define PPIP_TMPFLAG_REVERSED 0x01000000
// 0x000000FF element
// 0x00000100 is single pixel pipe
// 0x00000200 will transfer like a single pixel pipe when in forward mode
// 0x00001C00 forward single pixel pipe direction
// 0x00002000 will transfer like a single pixel pipe when in reverse mode
// 0x0001C000 reverse single pixel pipe direction

signed char pos_1_rx[] = {-1,-1,-1, 0, 0, 1, 1, 1};
signed char pos_1_ry[] = {-1, 0, 1,-1, 1,-1, 0, 1};

void PPIP_flood_trigger(Simulation* sim, int x, int y, int sparkedBy)
{
	int coord_stack_limit = XRES*YRES;
	unsigned short (*coord_stack)[2];
	int coord_stack_size = 0;
	int x1, x2;

	// Separate flags for on and off in case PPIP is sparked by PSCN and NSCN on the same frame
	// - then PSCN can override NSCN and behaviour is not dependent on particle order
	int prop = 0;
	if (sparkedBy==PT_PSCN) prop = PPIP_TMPFLAG_TRIGGER_ON << 3;
	else if (sparkedBy==PT_NSCN) prop = PPIP_TMPFLAG_TRIGGER_OFF << 3;
	else if (sparkedBy==PT_INST) prop = PPIP_TMPFLAG_TRIGGER_REVERSE << 3;

	if (prop==0 || (pmap[y][x]&0xFF)!=PT_PPIP || (parts[pmap[y][x]>>8].tmp & prop))
		return;

	coord_stack = (unsigned short(*)[2])malloc(sizeof(unsigned short)*2*coord_stack_limit);
	coord_stack[coord_stack_size][0] = x;
	coord_stack[coord_stack_size][1] = y;
	coord_stack_size++;

	do
	{
		coord_stack_size--;
		x = coord_stack[coord_stack_size][0];
		y = coord_stack[coord_stack_size][1];
		x1 = x2 = x;
		// go left as far as possible
		while (x1>=CELL)
		{
			if ((pmap[y][x1-1]&0xFF)!=PT_PPIP)
			{
				break;
			}
			x1--;
		}
		// go right as far as possible
		while (x2<XRES-CELL)
		{
			if ((pmap[y][x2+1]&0xFF)!=PT_PPIP)
			{
				break;
			}
			x2++;
		}
		// fill span
		for (x=x1; x<=x2; x++)
		{
			if (!(parts[pmap[y][x]>>8].tmp & prop))
				((PPIP_ElementDataContainer*)sim->elementData[PT_PPIP])->ppip_changed = 1;
			parts[pmap[y][x]>>8].tmp |= prop;
		}

		// add adjacent pixels to stack
		// +-1 to x limits to include diagonally adjacent pixels
		// Don't need to check x bounds here, because already limited to [CELL, XRES-CELL]
		if (y>=CELL+1)
			for (x=x1-1; x<=x2+1; x++)
				if ((pmap[y-1][x]&0xFF)==PT_PPIP && !(parts[pmap[y-1][x]>>8].tmp & prop))
				{
					coord_stack[coord_stack_size][0] = x;
					coord_stack[coord_stack_size][1] = y-1;
					coord_stack_size++;
					if (coord_stack_size>=coord_stack_limit)
					{
						free(coord_stack);
						return;
					}
				}
		if (y<YRES-CELL-1)
			for (x=x1-1; x<=x2+1; x++)
				if ((pmap[y+1][x]&0xFF)==PT_PPIP && !(parts[pmap[y+1][x]>>8].tmp & prop))
				{
					coord_stack[coord_stack_size][0] = x;
					coord_stack[coord_stack_size][1] = y+1;
					coord_stack_size++;
					if (coord_stack_size>=coord_stack_limit)
					{
						free(coord_stack);
						return;
					}
				}
	} while (coord_stack_size>0);
	free(coord_stack);
}

void PIPE_transfer_pipe_to_part(particle *pipe, particle *part)
{
	part->type = (pipe->tmp & 0xFF);
	part->temp = pipe->temp;
	part->life = pipe->tmp2;
	part->tmp = (int)pipe->pavg[0];
	part->ctype = (int)pipe->pavg[1];
	pipe->tmp &= ~0xFF;

	if (!(ptypes[part->type].properties & TYPE_ENERGY))
	{
		part->vx = 0.0f;
		part->vy = 0.0f;
	}
	else if (part->type == PT_PHOT && part->ctype == 0x40000000)
		part->ctype = 0x3FFFFFFF;
	part->tmp2 = 0;
	part->flags = 0;
	part->dcolour = COLARGB(0, 0, 0, 0);
}

void PIPE_transfer_part_to_pipe(particle *part, particle *pipe)
{
	pipe->tmp = (pipe->tmp&~0xFF) | part->type;
	pipe->temp = part->temp;
	pipe->tmp2 = part->life;
	pipe->pavg[0] = (float)part->tmp;
	pipe->pavg[1] = (float)part->ctype;
}

void PIPE_transfer_pipe_to_pipe(particle *src, particle *dest)
{
	dest->tmp = (dest->tmp&~0xFF) | (src->tmp&0xFF);
	dest->temp = src->temp;
	dest->tmp2 = src->tmp2;
	dest->pavg[0] = src->pavg[0];
	dest->pavg[1] = src->pavg[1];
	src->tmp &= ~0xFF;
}

void pushParticle(Simulation *sim, int i, int count, int original)
{
	int rndstore, rnd, rx, ry, r, x, y, np, q, notctype=(((parts[i].ctype)%3)+2);
	if ((parts[i].tmp&0xFF) == 0 || count >= 2)//don't push if there is nothing there, max speed of 2 per frame
		return;
	x = (int)(parts[i].x+0.5f);
	y = (int)(parts[i].y+0.5f);
	if( !(parts[i].tmp&0x200) )
	{ 
		//normal random push
		rndstore = rand();
		// RAND_MAX is at least 32767 on all platforms i.e. pow(8,5)-1
		// so can go 5 cycles without regenerating rndstore
		for (q=0; q<3; q++)//try to push 3 times
		{
			rnd = rndstore&7;
			rndstore = rndstore>>3;
			rx = pos_1_rx[rnd];
			ry = pos_1_ry[rnd];
			if (BOUNDS_CHECK)
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				else if (((r&0xFF)==PT_PIPE || (r&0xFF) == PT_PPIP) && parts[r>>8].ctype!=notctype && (parts[r>>8].tmp&0xFF)==0)
				{
					PIPE_transfer_pipe_to_pipe(parts+i, parts+(r>>8));
					if (r>>8 > original)
						parts[r>>8].flags |= PFLAG_NORMALSPEED;//skip particle push, normalizes speed
					count++;
					pushParticle(sim, r>>8,count,original);
				}
				else if ((r&0xFF) == PT_PRTI) //Pass particles into PRTI for a pipe speed increase
				{
					PortalChannel *channel = ((PRTI_ElementDataContainer*)sim->elementData[PT_PRTI])->GetParticleChannel(sim, r>>8);
					int slot = PRTI_ElementDataContainer::GetSlot(-rx, -ry);
					particle *storePart = channel->AllocParticle(slot);
					if (storePart)
					{
						PIPE_transfer_pipe_to_part(parts+i, storePart);
						count++;
						break;
					}
				}
			}
		}
	}
	else //predefined 1 pixel thick pipe movement
	{
		int coords = 7 - ((parts[i].tmp>>10)&7);
		r = pmap[y+ pos_1_ry[coords]][x+ pos_1_rx[coords]];
		if (((r&0xFF)==PT_PIPE || (r&0xFF) == PT_PPIP) && parts[r>>8].ctype!=notctype && (parts[r>>8].tmp&0xFF)==0)
		{
			PIPE_transfer_pipe_to_pipe(parts+i, parts+(r>>8));
			if (r>>8 > original)
				parts[r>>8].flags |= PFLAG_NORMALSPEED;//skip particle push, normalizes speed
			count++;
			pushParticle(sim, r>>8,count,original);
		}
		else if ((r&0xFF) == PT_PRTI) //Pass particles into PRTI for a pipe speed increase
		{
			PortalChannel *channel = ((PRTI_ElementDataContainer*)sim->elementData[PT_PRTI])->GetParticleChannel(sim, r>>8);
			int slot = PRTI_ElementDataContainer::GetSlot(-pos_1_rx[coords], -pos_1_ry[coords]);
			particle *storePart = channel->AllocParticle(slot);
			if (storePart)
			{
				PIPE_transfer_pipe_to_part(parts+i, storePart);
				count++;
			}
		}
		else if ((r&0xFF) == PT_NONE) //Move particles out of pipe automatically, much faster at ends
		{
			rx = pos_1_rx[coords];
			ry = pos_1_ry[coords];
			np = sim->part_create(-1,x+rx,y+ry,parts[i].tmp&0xFF);
			if (np!=-1)
			{
				PIPE_transfer_pipe_to_part(parts+i, parts+np);
			}
		}
	}
	return;
}

void detach(int i);

int PIPE_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, np;
	int rnd, rndstore;
	if ((parts[i].tmp&0xFF)>=PT_NUM || !sim->elements[parts[i].tmp&0xFF].Enabled)
		parts[i].tmp &= ~0xFF; 
	if (parts[i].tmp & PPIP_TMPFLAG_TRIGGERS)
	{
		int pause_changed = 0;
		if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_ON) // TRIGGER_ON overrides TRIGGER_OFF
		{
			if (parts[i].tmp & PPIP_TMPFLAG_PAUSED)
				pause_changed = 1;
			parts[i].tmp &= ~PPIP_TMPFLAG_PAUSED;
		}
		else if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_OFF)
		{
			if (!(parts[i].tmp & PPIP_TMPFLAG_PAUSED))
				pause_changed = 1;
			parts[i].tmp |= PPIP_TMPFLAG_PAUSED;
		}
		if (pause_changed)
		{
			int rx, ry, r;
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
				{
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r&0xFF) == PT_BRCK)
						{
							if (parts[i].tmp & PPIP_TMPFLAG_PAUSED)
								parts[r>>8].tmp = 0;
							else
								parts[r>>8].tmp = 1; //make surrounding BRCK glow
						}
					}
				}
		}

		if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_REVERSE)
		{
			parts[i].tmp ^= PPIP_TMPFLAG_REVERSED;
			if (parts[i].ctype == 2) //Switch colors so it goes in reverse
				parts[i].ctype = 4;
			else if (parts[i].ctype == 4)
				parts[i].ctype = 2;
			if (parts[i].tmp & 0x100) //Switch one pixel pipe direction
			{
				int coords = (parts[i].tmp>>13)&0xF;
				int coords2 = (parts[i].tmp>>9)&0xF;
				parts[i].tmp &= ~0x1FE00;
				parts[i].tmp |= coords<<9;
				parts[i].tmp |= coords2<<13;
			}
		}

		parts[i].tmp &= ~PPIP_TMPFLAG_TRIGGERS;
	}

	if (parts[i].ctype>=2 && parts[i].ctype<=4 && !(parts[i].tmp & PPIP_TMPFLAG_PAUSED))
	{
		if (parts[i].life==3)
		{
			int lastneighbor = -1;
			int neighborcount = 0;
			int count = 0;
			// make automatic pipe pattern
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (!r)
							continue;
						if (((r&0xFF)==PT_PIPE || (r&0xFF) == PT_PPIP)&&parts[r>>8].ctype==1)
						{
							parts[r>>8].ctype = (((parts[i].ctype)%3)+2);//reverse
							parts[r>>8].life = 6;
							if ( parts[i].tmp&0x100)//is a single pixel pipe
							{
								parts[r>>8].tmp |= 0x200;//will transfer to a single pixel pipe
								parts[r>>8].tmp |= count<<10;//coords of where it came from
								parts[i].tmp |= ((7-count)<<14);
								parts[i].tmp |= 0x2000;
							}
							neighborcount ++;
							lastneighbor = r>>8;
						}
						else if (((r&0xFF)==PT_PIPE || (r&0xFF) == PT_PPIP)&&parts[r>>8].ctype!=(((parts[i].ctype-1)%3)+2))
						{
							neighborcount ++;
							lastneighbor = r>>8;
						}
						count++;
					}
			if(neighborcount == 1)
				parts[lastneighbor].tmp |= 0x100;
		}
		else
		{
			if (parts[i].flags&PFLAG_NORMALSPEED)//skip particle push to prevent particle number being higher causing speed up
			{
				parts[i].flags &= ~PFLAG_NORMALSPEED;
			}
			else
			{
				pushParticle(sim, i,0,i);
			}

			if (nt)//there is something besides PIPE around current particle
			{
				rndstore = rand();
				rnd = rndstore&7;
				rndstore = rndstore>>3;
				rx = pos_1_rx[rnd];
				ry = pos_1_ry[rnd];
				if (BOUNDS_CHECK)
				{
					r = pmap[y+ry][x+rx];
					if(!r)
						r = photons[y+ry][x+rx];
					if (surround_space && !r && (parts[i].tmp&0xFF)!=0)  //creating at end
					{
						np = sim->part_create(-1,x+rx,y+ry,parts[i].tmp&0xFF);
						if (np!=-1)
						{
							PIPE_transfer_pipe_to_part(parts+i, parts+np);
						}
					}
					//try eating particle at entrance
					else if ((parts[i].tmp&0xFF) == 0 && (ptypes[r&0xFF].properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)))
					{
						if ((r&0xFF)==PT_SOAP)
							detach(r>>8);
						PIPE_transfer_part_to_pipe(parts+(r>>8), parts+i);
						kill_part(r>>8);
					}
					else if ((parts[i].tmp&0xFF) == 0 && (r&0xFF)==PT_STOR && sim->IsElement(parts[r>>8].tmp) && (ptypes[parts[r>>8].tmp].properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)))
					{
						// STOR stores properties in the same places as PIPE does
						PIPE_transfer_pipe_to_pipe(parts+(r>>8), parts+i);
					}
				}
			}
		}
	}
	else if (!parts[i].ctype && parts[i].life<=10)
	{
		// make a border
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
			{
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
					{
						int index = sim->part_create(-1,x+rx,y+ry,PT_BRCK);//BRCK border, people didn't like DMND
						if (parts[i].type == PT_PPIP && index != -1)
							parts[index].tmp = 1;
					}
				}
			}
		if (parts[i].life<=1)
			parts[i].ctype = 1;
	}
	else if (parts[i].ctype==1)//wait for empty space before starting to generate automatic pipe pattern
	{
		if (!parts[i].life)
		{
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						if (!pmap[y+ry][x+rx] && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_ALLOWAIR && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_WALL && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_WALLELEC && (bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_EWALL || emap[(y+ry)/CELL][(x+rx)/CELL]))
							parts[i].life=50;
					}
		}
		else if (parts[i].life==5)//check for beginning of pipe single pixel
		{
			int issingle = 1;
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (((r&0xFF)==PT_PIPE || (r&0xFF) == PT_PPIP) && parts[i].ctype==1 && parts[i].life )
							issingle = 0;
					}
			if (issingle)
				parts[i].tmp |= 0x100;
		}
		else if (parts[i].life==2)
		{
			parts[i].ctype = 2;
			parts[i].life = 6;
		}
	}
	return 0;
}

//Temp particle used for graphics
particle tpart;
int PIPE_graphics(GRAPHICS_FUNC_ARGS)
{
	int t = cpart->tmp&0xFF;
	if (t > 0 && t < PT_NUM && sim->elements[t].Enabled)
	{
		if (t == PT_STKM || t == PT_STKM2 || t == PT_FIGH)
			return 0;
		if (graphicscache[t].isready)
		{
			*pixel_mode = graphicscache[t].pixel_mode;
			*cola = graphicscache[t].cola;
			*colr = graphicscache[t].colr;
			*colg = graphicscache[t].colg;
			*colb = graphicscache[t].colb;
			*firea = graphicscache[t].firea;
			*firer = graphicscache[t].firer;
			*fireg = graphicscache[t].fireg;
			*fireb = graphicscache[t].fireb;
		}
		else
		{
			//Emulate the graphics of stored particle
			tpart.type = t;
			tpart.temp = cpart->temp;
			tpart.life = cpart->tmp2;
			tpart.tmp = (int)cpart->pavg[0];
			tpart.ctype = (int)cpart->pavg[1];
			if (t == PT_PHOT && tpart.ctype == 0x40000000)
				tpart.ctype = 0x3FFFFFFF;

			*colr = PIXR(sim->elements[t].Colour);
			*colg = PIXG(sim->elements[t].Colour);
			*colb = PIXB(sim->elements[t].Colour);
			if (ptypes[t].graphics_func)
			{
				(*(ptypes[t].graphics_func))(sim, &tpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
			}
			else
			{
				graphics_DEFAULT(sim, &tpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
			}
		}
		//*colr = PIXR(ptypes[cpart->tmp&0xFF].pcolors);
		//*colg = PIXG(ptypes[cpart->tmp&0xFF].pcolors);
		//*colb = PIXB(ptypes[cpart->tmp&0xFF].pcolors);
	}
	else
	{
		switch (cpart->ctype)
		{
		case 2:
			*colr = 50;
			*colg = 1;
			*colb = 1;
			break;
		case 3:
			*colr = 1;
			*colg = 50;
			*colb = 1;
			break;
		case 4:
			*colr = 1;
			*colg = 1;
			*colb = 50;
			break;
		default:
			break;
		}
	}
	return 0;
}

void PIPE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PIPE";
	elem->Name = "PIPE";
	elem->Colour = COLPACK(0x444444);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "PIPE, moves particles around. Once the BRCK generates, erase some for the exit. Then the PIPE generates and is usable.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 10.0f;
	elem->HighPressureTransitionElement = PT_BRMT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 60;

	elem->Update = &PIPE_update;
	elem->Graphics = &PIPE_graphics;
	elem->Init = &PIPE_init_element;

	memset(&tpart, 0, sizeof(particle));
}
