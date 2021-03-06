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

int CBNW_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	if (sim->air->pv[y/CELL][x/CELL]<=3)
	{
		if (sim->air->pv[y/CELL][x/CELL] <= -0.5 || !(rand()%4000))
		{
			part_change_type(i, x, y, PT_CO2);
			parts[i].ctype = 5;
			sim->air->pv[y/CELL][x/CELL] += 0.5f;
		}
	}
	if (parts[i].tmp2!=20)
	{
		parts[i].tmp2 -= (parts[i].tmp2>20)?1:-1;
	}
	else if (!(rand()%200))
	{
		parts[i].tmp2 = rand()%40;
	}

	if (parts[i].tmp > 0)
	{
		//Explode
		if (parts[i].tmp==1 && rand()%4)
		{
			part_change_type(i, x, y, PT_CO2);
			parts[i].ctype = 5;
			sim->air->pv[y/CELL][x/CELL] += 0.2f;
		}
		parts[i].tmp--;
	}
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (ptypes[r&0xFF].properties&TYPE_PART && parts[i].tmp == 0 && !(rand()%83))
				{
					//Start explode
					parts[i].tmp = rand()%25;//(rand()%100)+50;
				}
				else if (ptypes[r&0xFF].properties&TYPE_SOLID && !(ptypes[r&0xFF].properties&PROP_INDESTRUCTIBLE) && (r&0xFF)!=PT_GLAS && parts[i].tmp == 0 && (2-sim->air->pv[y/CELL][x/CELL])>(rand()%6667))
				{
					if (rand()%2)
					{
						part_change_type(i, x, y, PT_CO2);
						parts[i].ctype = 5;
						sim->air->pv[y/CELL][x/CELL] += 0.2f;
					}
				}
				if ((r&0xFF)==PT_CBNW)
				{
					if (!parts[i].tmp)
					{
						if (parts[r>>8].tmp)
						{
							parts[i].tmp = parts[r>>8].tmp;
							if ((r>>8)>i) //If the other particle hasn't been life updated
								parts[i].tmp--;
						}
					}
					else if (!parts[r>>8].tmp)
					{
						parts[r>>8].tmp = parts[i].tmp;
						if ((r>>8)>i) //If the other particle hasn't been life updated
							parts[r>>8].tmp++;
					}
				}
				else if ((r&0xFF)==PT_RBDM || (r&0xFF)==PT_LRBD)
				{
					if ((legacy_enable||parts[i].temp>(273.15f+12.0f)) && !(rand()%166))
					{
						part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 4;
						parts[i].ctype = PT_WATR;
					}
				}
				else if ((r&0xFF)==PT_FIRE && parts[r>>8].ctype!=PT_WATR)
				{
					kill_part(r>>8);
					if (!(rand()%50)){
						kill_part(i);
						return 1;
					}
				}
			}
	return 0;
}

int CBNW_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = cpart->tmp2 - 20;//speckles!
	*colr += z * 1;
	*colg += z * 2;
	*colb += z * 8;
	return 0;
}

void CBNW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CBNW";
	elem->Name = "BUBW";
	elem->Colour = COLPACK(0x2030D0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 7500;
	elem->Description = "Carbonated water. Slowly releases CO2.";

	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 273.15f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &CBNW_update;
	elem->Graphics = &CBNW_graphics;
	elem->Init = &CBNW_init_element;
}
