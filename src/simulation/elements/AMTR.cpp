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

int AMTR_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				rt = (r&0xFF);
				//would a table lookup be faster than 11 checks?
				if (rt!=PT_AMTR && !(ptypes[rt].properties&PROP_INDESTRUCTIBLE) && !(ptypes[rt].properties&PROP_CLONE) && rt!=PT_NONE && rt!=PT_VOID && rt!=PT_BHOL && rt!=PT_NBHL && rt!=PT_PRTI && rt!=PT_PRTO)
				{
#ifndef NOMOD
					if (rt == PT_PPTI || rt == PT_PPTO)
						continue;
#endif
					if(!parts[i].ctype || (parts[i].ctype==(r&0xFF))!=(parts[i].tmp&1))
					{
						parts[i].life++;
						if (parts[i].life==4)
						{
							kill_part(i);
							return 1;
						}
						if (!(rand()%10))
							sim->part_create(r>>8, x+rx, y+ry, PT_PHOT);
						else
							kill_part(r>>8);
						sim->air->pv[y/CELL][x/CELL] -= 2.0f;
					}
				}
			}
	return 0;
}

int AMTR_graphics(GRAPHICS_FUNC_ARGS)
{
	// don't render AMTR as a gas
	// this function just overrides the default graphics
	return 1;
}

void AMTR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_AMTR";
	elem->Name = "AMTR";
	elem->Colour = COLPACK(0x808080);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.00f;
	elem->Gravity = 0.10f;
	elem->Diffusion = 1.00f;
	elem->HotAir = 0.0000f * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Anti-Matter, destroys a majority of particles.";

	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &AMTR_update;
	elem->Graphics = &AMTR_graphics;
	elem->Init = &AMTR_init_element;
}
