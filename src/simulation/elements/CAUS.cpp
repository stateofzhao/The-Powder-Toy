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

int CAUS_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF) == PT_GAS)
				{
					if (sim->air->pv[(y+ry)/CELL][(x+rx)/CELL] > 3)
					{
						sim->part_change_type(r>>8, x+rx, y+ry, PT_RFRG);
						sim->part_change_type(i, x, y, PT_RFRG);
					}
				}
				else if ((r&0xFF)!=PT_ACID && (r&0xFF)!=PT_CAUS && (r&0xFF)!=PT_RFRG && (r&0xFF)!=PT_RFGL)
				{
					if ((!(sim->elements[r&0xFF].Properties&PROP_CLONE) && sim->elements[r&0xFF].Hardness>(rand()%1000))&&parts[i].life>=50)
					{
						if (parts_avg(i, r>>8,PT_GLAS)!= PT_GLAS)//GLAS protects stuff from acid
						{
							float newtemp = ((60.0f-(float)sim->elements[r&0xFF].Hardness))*7.0f;
							if(newtemp < 0){
								newtemp = 0;
							}
							parts[i].temp += newtemp;
							parts[i].life--;
							sim->part_kill(r>>8);
							continue;
						}
					}
					else if (parts[i].life<=50)
					{
						sim->part_kill(i);
						return 1;
					}
				}
			}
	return 0;
}

void CAUS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CAUS";
	elem->Name = "CAUS";
	elem->Colour = COLPACK(0x80FFA0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 1.50f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Caustic Gas, acts like ACID.";

	elem->Properties = TYPE_GAS|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 75;

	elem->Update = &CAUS_update;
	elem->Graphics = NULL;
	elem->Init = &CAUS_init_element;
}
