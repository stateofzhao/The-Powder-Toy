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

int SING_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, cry, crx, nb = -1, spawncount;
	int singularity = -parts[i].life;
	float angle, v;

	if (sim->air->pv[y/CELL][x/CELL]<singularity)
		sim->air->pv[y/CELL][x/CELL] += 0.1f*(singularity-sim->air->pv[y/CELL][x/CELL]);
	if (sim->air->pv[y/CELL+1][x/CELL]<singularity)
		sim->air->pv[y/CELL+1][x/CELL] += 0.1f*(singularity-sim->air->pv[y/CELL+1][x/CELL]);
	if (sim->air->pv[y/CELL-1][x/CELL]<singularity)
		sim->air->pv[y/CELL-1][x/CELL] += 0.1f*(singularity-sim->air->pv[y/CELL-1][x/CELL]);

	sim->air->pv[y/CELL][x/CELL+1] += 0.1f*(singularity-sim->air->pv[y/CELL][x/CELL+1]);
	sim->air->pv[y/CELL+1][x/CELL+1] += 0.1f*(singularity-sim->air->pv[y/CELL+1][x/CELL+1]);
	sim->air->pv[y/CELL][x/CELL-1] += 0.1f*(singularity-sim->air->pv[y/CELL][x/CELL-1]);
	sim->air->pv[y/CELL-1][x/CELL-1] += 0.1f*(singularity-sim->air->pv[y/CELL-1][x/CELL-1]);

	if (parts[i].life<1)
	{
		//Pop!
		for (rx=-1; rx<2; rx++)
		{
			crx = (x/CELL)+rx;
			for (ry=-1; ry<2; ry++)
			{
				cry = (y/CELL)+ry;
				if (cry >= 0 && crx >= 0 && crx < (XRES/CELL) && cry < (YRES/CELL))
					sim->air->pv[cry][crx] += (float)parts[i].tmp;
			}
		}
		spawncount = std::abs(parts[i].tmp);
		spawncount = (spawncount>255) ? 3019 : (int)(std::pow((double)(spawncount/8), 2)*M_PI);
		for (int j = 0; j < spawncount; j++)
		{
			switch(rand()%3)
			{
				case 0:
					nb = sim->part_create(-3, x, y, PT_PHOT);
					break;
				case 1:
					nb = sim->part_create(-3, x, y, PT_NEUT);
					break;
				case 2:
					nb = sim->part_create(-3, x, y, PT_ELEC);
					break;
			}
			if (nb != -1)
			{
				parts[nb].life = (rand()%300);
				parts[nb].temp = MAX_TEMP/2;
				angle = rand()*2.0f*M_PI/RAND_MAX;
				v = (float)(rand())*5.0f/RAND_MAX;
				parts[nb].vx = v*cosf(angle);
				parts[nb].vy = v*sinf(angle);
			}
			else if (sim->pfree==-1)
				break;//if we've run out of particles, stop trying to create them - saves a lot of lag on "sing bomb" saves
		}
		kill_part(i);
		return 1;
	}
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (!(ptypes[r&0xFF].properties&PROP_INDESTRUCTIBLE) && !(ptypes[r&0xFF].properties&PROP_CLONE) && !(ptypes[r&0xFF].properties&PROP_BREAKABLECLONE) && !(rand()%3))
				{
					if ((r&0xFF)==PT_SING && parts[r>>8].life >10)
					{
						if (parts[i].life+parts[r>>8].life > 255)
							continue;
						parts[i].life += parts[r>>8].life;
					}
					else
					{
						if (parts[i].life+3 > 255)
						{
							if (parts[r>>8].type!=PT_SING && !(rand()%100))
							{
								int np;
								np = sim->part_create(r>>8,x+rx,y+ry,PT_SING);
								parts[np].life = rand()%50+60;
								parts[np].tmp2 = parts[i].tmp2;
							}
							continue;
						}
						parts[i].life += 3;
						parts[i].tmp++;
					}
					parts[i].temp = restrict_flt(parts[r>>8].temp+parts[i].temp, MIN_TEMP, MAX_TEMP);
					kill_part(r>>8);
				}
			}
	return 0;
}

void SING_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = rand()%50+60;
}

void SING_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SING";
	elem->Name = "SING";
	elem->Colour = COLPACK(0x242424);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.36f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.1f;
	elem->Gravity = 0.12f;
	elem->Diffusion = 0.00f;
	elem->HotAir = -0.001f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 86;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Singularity. Creates huge amounts of negative pressure and destroys everything.";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &SING_update;
	elem->Graphics = NULL;
	elem->Func_Create = &SING_create;
	elem->Init = &SING_init_element;
}
