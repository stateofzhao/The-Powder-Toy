#include "RenderModesUI.h"
#include "defines.h"
#include "graphics.h"
#include "misc.h"
#include "powdergraphics.h"
#include "common/Point.h"
#include "game/ToolTip.h"
#include "graphics/Renderer.h"
#include "graphics/VideoBuffer.h"
#include "interface/Label.h"
#include "interface/Button.h"
#include "interface/Checkbox.h"


RenderModesUI::RenderModesUI():
	Window_(Point(0, YRES), Point(XRES, MENUSIZE)),
	last_render_mode(render_mode),
	last_display_mode(display_mode),
	last_color_mode(Renderer::Ref().GetColorMode()),
	interfaceSwap(false),
	renderCheckboxes(std::vector<std::pair<Checkbox*, unsigned int> >()),
	displayCheckboxes(std::vector<std::pair<Checkbox*, unsigned int> >()),
	colorCheckboxes(std::vector<std::pair<Checkbox*, unsigned int> >()),
	buttons(std::vector<Button*>())
{
#ifndef TOUCHUI
	InitializeCheckboxes();
#endif
	InitializeButtons();

#ifdef TOUCHUI
	class SwapInterfaceAction : public ButtonAction
	{
	public:
		virtual void ButtionActionCallback(Button *button, unsigned char b)
		{
			dynamic_cast<RenderModesUI*>(button->GetParent())->SwapInterface();
		}
	};
	swapButton = new Button(Point(0, 0), Point(40, MENUSIZE), "Adv.");
	swapButton->SetCallback(new SwapInterfaceAction());
	this->AddComponent(swapButton);
#endif
}

void RenderModesUI::SwapInterface()
{
#ifdef TOUCHUI
	if (!interfaceSwap)
	{
		for (std::vector<Button*>::iterator iter = buttons.begin(), end = buttons.end(); iter != end; iter++)
			this->RemoveComponent(*iter);
		buttons.clear();
		InitializeCheckboxes();
		swapButton->SetText("Simple");
	}
	else
	{
		for (std::vector<std::pair<Checkbox*, unsigned int> >::iterator iter = renderCheckboxes.begin(), end = renderCheckboxes.end(); iter != end; iter++)
			this->RemoveComponent((*iter).first);
		for (std::vector<std::pair<Checkbox*, unsigned int> >::iterator iter = displayCheckboxes.begin(), end = displayCheckboxes.end(); iter != end; iter++)
			this->RemoveComponent((*iter).first);
		for (std::vector<std::pair<Checkbox*, unsigned int> >::iterator iter = colorCheckboxes.begin(), end = colorCheckboxes.end(); iter != end; iter++)
			this->RemoveComponent((*iter).first);
		renderCheckboxes.clear();
		displayCheckboxes.clear();
		colorCheckboxes.clear();
		InitializeButtons();
		swapButton->SetText("Adv.");
	}
	interfaceSwap = !interfaceSwap;
#endif
}

class ToggleRenderModeAction : public CheckboxAction
{
	int render_md;
public:
	ToggleRenderModeAction(unsigned int render_md):
		render_md(render_md)
	{

	}

	virtual void CheckboxActionCallback(Checkbox *checkbox, unsigned char b)
	{
		if (checkbox->IsChecked())
			Renderer::Ref().AddRenderMode(render_md);
		else
			Renderer::Ref().RemoveRenderMode(render_md);
	}
};

void RenderModesUI::InitializeRenderCheckbox(Checkbox *checkbox, unsigned int mode)
{
	checkbox->SetCallback(new ToggleRenderModeAction(mode));
#ifdef TOUCHUI
	checkbox->SetColor(COLRGB(255, 127, 255));
#endif
	if (Renderer::Ref().HasRenderMode(mode))
		checkbox->SetChecked(true);
	this->AddComponent(checkbox);
	renderCheckboxes.push_back(std::pair<Checkbox*, unsigned int>(checkbox, mode));
}

class ToggleDisplayModeAction : public CheckboxAction
{
	int display_md;
public:
	ToggleDisplayModeAction(unsigned int display_md):
		display_md(display_md)
	{

	}

	virtual void CheckboxActionCallback(Checkbox *checkbox, unsigned char b)
	{
		if (checkbox->IsChecked())
			Renderer::Ref().AddDisplayMode(display_md);
		else
			Renderer::Ref().RemoveDisplayMode(display_md);
		display_mode = Renderer::Ref().GetDisplayModesRaw();
	}
};

void RenderModesUI::InitializeDisplayCheckbox(Checkbox *checkbox, unsigned int mode)
{
	checkbox->SetCallback(new ToggleDisplayModeAction(mode));
#ifdef TOUCHUI
	checkbox->SetColor(COLRGB(127, 255, 255));
#endif
	if (Renderer::Ref().HasDisplayMode(mode))
		checkbox->SetChecked(true);
	this->AddComponent(checkbox);
	displayCheckboxes.push_back(std::pair<Checkbox*, unsigned int>(checkbox, mode));
}

class ToggleColorModeAction : public CheckboxAction
{
	int color_md;
public:
	ToggleColorModeAction(unsigned int color_md):
		color_md(color_md)
	{

	}

	virtual void CheckboxActionCallback(Checkbox *checkbox, unsigned char b)
	{
		if (checkbox->IsChecked())
			Renderer::Ref().SetColorMode(color_md);
		else
			Renderer::Ref().SetColorMode(COLOR_DEFAULT);
	}
};

void RenderModesUI::InitializeColorCheckbox(Checkbox *checkbox, unsigned int mode)
{
	checkbox->SetCallback(new ToggleColorModeAction(mode));
#ifdef TOUCHUI
	checkbox->SetColor(COLRGB(255, 255, 127));
#endif
	if (Renderer::Ref().GetColorMode() == mode)
		checkbox->SetChecked(true);
	this->AddComponent(checkbox);
	colorCheckboxes.push_back(std::pair<Checkbox*, unsigned int>(checkbox, mode));
}

void RenderModesUI::SetCheckboxToolTip(Checkbox *checkbox, std::string tooltip)
{
	checkbox->SetTooltip(new ToolTip(tooltip, Point(16, YRES-24), TOOLTIP, 255));
}


Point RenderModesUI::CheckboxPos(Checkbox *prev1, Checkbox *prev2)
{
#ifdef TOUCHUI
	return prev1->Right(Point(4, 0));
#else
	if (!prev2)
		return prev1->Below(Point(0, 2));
	else if (prev1->GetPosition().Y > MENUSIZE/2)
		return prev2->Right(Point(4, 0));
	else
		return prev1->Below(Point(0, 2));
#endif
}

void RenderModesUI::InitializeCheckboxes()
{
#ifdef TOUCHUI
	Point size = Point(Checkbox::TEXTINSIDE, 27);
	Point pos = Point(42, (MENUSIZE-size.Y)/2);
#else
	Point size = Point(Checkbox::AUTOSIZE, Checkbox::AUTOSIZE);
	// hardcoded checkbox length (34)
	Point pos = Point(34*9, 3);
#endif

	Checkbox *effectsCheckbox = new Checkbox(pos, size, "\xE1");
	InitializeRenderCheckbox(effectsCheckbox, RENDER_EFFE);
	SetCheckboxToolTip(effectsCheckbox, "Adds Special flare effects to some elements");

	Checkbox *glowCheckbox = new Checkbox(CheckboxPos(effectsCheckbox, NULL), size, "\xDF");
	InitializeRenderCheckbox(glowCheckbox, RENDER_GLOW);
	SetCheckboxToolTip(glowCheckbox, "Glow effect on some elements");

	Checkbox *fireCheckbox = new Checkbox(CheckboxPos(glowCheckbox, effectsCheckbox), size, "\x9B");
	InitializeRenderCheckbox(fireCheckbox, RENDER_FIRE);
	SetCheckboxToolTip(fireCheckbox, "Fire effect for gasses");

	Checkbox *blurCheckbox = new Checkbox(CheckboxPos(fireCheckbox, glowCheckbox), size, "\xC4");
	InitializeRenderCheckbox(blurCheckbox, RENDER_BLUR);
	SetCheckboxToolTip(blurCheckbox, "Blur effect for liquids");

	Checkbox *basicCheckbox = new Checkbox(CheckboxPos(blurCheckbox, fireCheckbox), size, "\xDB");
	InitializeRenderCheckbox(basicCheckbox, RENDER_BASC);
	SetCheckboxToolTip(basicCheckbox, "Basic rendering, without this, most things will be invisible");

	Checkbox *blobCheckbox = new Checkbox(CheckboxPos(basicCheckbox, blurCheckbox), size, "\xBF");
	InitializeRenderCheckbox(blobCheckbox, RENDER_BLOB);
	SetCheckboxToolTip(blobCheckbox, "Makes everything be drawn like a blob");

	Checkbox *sparkCheckbox = new Checkbox(CheckboxPos(blobCheckbox, basicCheckbox), size, "\xDF");
	InitializeRenderCheckbox(sparkCheckbox, RENDER_SPRK);
	SetCheckboxToolTip(sparkCheckbox, "Glow effect on sparks");

	Checkbox *noneCheckbox = new Checkbox(CheckboxPos(sparkCheckbox, blobCheckbox), size, "\xDB");
	InitializeRenderCheckbox(noneCheckbox, RENDER_NONE);
	SetCheckboxToolTip(noneCheckbox, "Even more basic rendering - only elements and stickmen are rendered");

	line1Pos = noneCheckbox->Right(Point(4, 0)).X;

	Checkbox *crackerCheckbox = new Checkbox(Point(line1Pos+6, sparkCheckbox->GetPosition().Y), size, "\xD4");
	InitializeDisplayCheckbox(crackerCheckbox, DISPLAY_AIRC);
	SetCheckboxToolTip(crackerCheckbox, "Displays pressure as red and blue, and velocity as white");

	Checkbox *airPressureCheckbox = new Checkbox(CheckboxPos(crackerCheckbox, NULL), size, "\x99");
	InitializeDisplayCheckbox(airPressureCheckbox, DISPLAY_AIRP);
	SetCheckboxToolTip(airPressureCheckbox, "Displays pressure, red is positive and blue is negative");

	Checkbox *airVelocityCheckbox = new Checkbox(CheckboxPos(airPressureCheckbox, crackerCheckbox), size, "\x98");
	InitializeDisplayCheckbox(airVelocityCheckbox, DISPLAY_AIRV);
	SetCheckboxToolTip(airVelocityCheckbox, "Displays velocity and positive pressure: up/down adds blue, right/left adds red, still pressure adds green");

	Checkbox *airHeatCheckbox = new Checkbox(CheckboxPos(airVelocityCheckbox, airPressureCheckbox), size, "\xBD");
	InitializeDisplayCheckbox(airHeatCheckbox, DISPLAY_AIRH);
	SetCheckboxToolTip(airHeatCheckbox, "Displays the temperature of the air like heat display does");

	Checkbox *warpCheckbox = new Checkbox(CheckboxPos(airHeatCheckbox, airVelocityCheckbox), size, "\xDE");
	InitializeDisplayCheckbox(warpCheckbox, DISPLAY_WARP);
	SetCheckboxToolTip(warpCheckbox, "Gravity lensing, Newtonian Gravity bends light with this on");

	Checkbox *persistentCheckbox = new Checkbox(CheckboxPos(warpCheckbox, crackerCheckbox), size, "\x9A");
	InitializeDisplayCheckbox(persistentCheckbox, DISPLAY_PERS);
	SetCheckboxToolTip(persistentCheckbox, "Element paths persist on the screen for a while");

	line2Pos = persistentCheckbox->Right(Point(4, 0)).X;

	Checkbox *basic2Checkbox = new Checkbox(Point(line2Pos+6, warpCheckbox->GetPosition().Y), size, "\xDB");
	InitializeColorCheckbox(basic2Checkbox, COLOR_BASC);
	SetCheckboxToolTip(basic2Checkbox, "No special effects at all for anything, overrides all other options and deco");

	Checkbox *lifeCheckbox = new Checkbox(CheckboxPos(basic2Checkbox, NULL), size, "\xE0");
	InitializeColorCheckbox(lifeCheckbox, COLOR_LIFE);
	SetCheckboxToolTip(lifeCheckbox, "Displays the life value of elements in greyscale gradients");

	Checkbox *heatCheckbox = new Checkbox(CheckboxPos(lifeCheckbox, basic2Checkbox), size, "\xBD");
	InitializeColorCheckbox(heatCheckbox, COLOR_HEAT);
	SetCheckboxToolTip(heatCheckbox, "Displays temperatures of the elements, dark blue is coldest, pink is hottest");

	Checkbox *heatGradientCheckbox = new Checkbox(CheckboxPos(heatCheckbox, lifeCheckbox), size, "\xD3");
	InitializeColorCheckbox(heatGradientCheckbox, COLOR_GRAD);
	SetCheckboxToolTip(heatGradientCheckbox, "Changes colors of elements slightly to show heat diffusing through them");
}


Point RenderModesUI::ButtonPos(Button *prev1, Button *prev2)
{
#ifdef TOUCHUI
	return prev1->Left(Point(40, 0));
#else
	if (!prev2)
		return prev1->Below(Point(0, 2));
	else if (prev1->GetPosition().Y > MENUSIZE/2)
		return prev2->Right(Point(4, 0));
	else
		return prev1->Below(Point(0, 2));
#endif
}

class ToggleDisplayPresetAction : public ButtonAction
{
	int display_preset;
public:
	ToggleDisplayPresetAction(int display_preset):
		display_preset(display_preset)
	{

	}

	virtual void ButtionActionCallback(Button *button, unsigned char b)
	{
		if (Renderer::Ref().LoadRenderPreset(display_preset))
		{
			std::string tooltip = Renderer::Ref().GetRenderPresetToolTip(display_preset);
			UpdateToolTip(tooltip, Point(XCNTR-textwidth(tooltip.c_str())/2, YCNTR-10), INFOTIP, 255);
			save_presets();
		}
	}
};

void RenderModesUI::InitializeButton(Button *button, int preset)
{
	button->SetCallback(new ToggleDisplayPresetAction(preset));
	buttons.push_back(button);
	this->AddComponent(button);
}


void RenderModesUI::SetButtonToolTip(Button *button, std::string tooltip)
{
	button->SetTooltip(new ToolTip(tooltip, Point(16, YRES-24), TOOLTIP, 255));
}

void RenderModesUI::InitializeButtons()
{
#ifdef TOUCHUI
	Point size = Point(33, 33);
	Point pos = Point(XRES-40, 3);
#else
	Point size = Point(31, 16);
	Point pos = Point(3, 3);
#endif

	Button *velocityButton = new Button(pos, size, "\x98");
	SetButtonToolTip(velocityButton, "Velocity display mode preset");
	InitializeButton(velocityButton, CM_VEL);

	Button *pressureButton = new Button(ButtonPos(velocityButton, NULL), size, "\x99");
	SetButtonToolTip(pressureButton, "Pressure display mode preset");
	InitializeButton(pressureButton, CM_PRESS);

	Button *persistentButton = new Button(ButtonPos(pressureButton, velocityButton), size, "\x9A");
	SetButtonToolTip(persistentButton, "Persistent display mode preset");
	InitializeButton(persistentButton, CM_PERS);

	Button *fireButton = new Button(ButtonPos(persistentButton, pressureButton), size, "\x9B");
	SetButtonToolTip(fireButton, "Fire display mode preset");
	InitializeButton(fireButton, CM_FIRE);

	Button *blobButton = new Button(ButtonPos(fireButton, persistentButton),size, "\xBF");
	SetButtonToolTip(blobButton, "Blob display mode preset");
	InitializeButton(blobButton, CM_BLOB);

	Button *heatButton = new Button(ButtonPos(blobButton, fireButton), size, "\xBD");
	SetButtonToolTip(heatButton, "Heat display mode preset");
	InitializeButton(heatButton, CM_HEAT);

	Button *fancyButton = new Button(ButtonPos(heatButton, blobButton), size, "\xC4");
	SetButtonToolTip(fancyButton, "Fancy display mode preset");
	InitializeButton(fancyButton, CM_FANCY);

	Button *nothingButton = new Button(ButtonPos(fancyButton, heatButton), size, "\xDB");
	SetButtonToolTip(nothingButton, "Nothing display mode preset");
	InitializeButton(nothingButton, CM_NOTHING);

	Button *heatGradientButton = new Button(ButtonPos(nothingButton, fancyButton), size, "\xD3");
	SetButtonToolTip(heatGradientButton, "Heat gradient display mode preset");
	InitializeButton(heatGradientButton, CM_GRAD);

	Button *alternateVelocityButton = new Button(ButtonPos(heatGradientButton, nothingButton), size, "\xD4");
	SetButtonToolTip(alternateVelocityButton, "Alternate Velocity display mode preset");
	InitializeButton(alternateVelocityButton, CM_CRACK);

	Button *lifeButton = new Button(ButtonPos(alternateVelocityButton, heatGradientButton), size, "\xE0");
	SetButtonToolTip(lifeButton, "Life display mode preset");
	InitializeButton(lifeButton, CM_LIFE);
}

void RenderModesUI::OnTick(uint32_t ticks)
{
	if (render_mode != last_render_mode)
	{
		for (std::vector<std::pair<Checkbox*, unsigned int> >::iterator iter = renderCheckboxes.begin(), end = renderCheckboxes.end(); iter != end; iter++)
		{
			unsigned int mode = (*iter).second;
			if (Renderer::Ref().HasRenderMode(mode))
				(*iter).first->SetChecked(true);
			else
				(*iter).first->SetChecked(false);
		}
		last_render_mode = render_mode;
	}

	if (display_mode != last_display_mode)
	{
		for (std::vector<std::pair<Checkbox*, unsigned int> >::iterator iter = displayCheckboxes.begin(), end = displayCheckboxes.end(); iter != end; iter++)
		{
			unsigned int mode = (*iter).second;
			if (Renderer::Ref().HasDisplayMode(mode))
				(*iter).first->SetChecked(true);
			else
				(*iter).first->SetChecked(false);
		}
		last_display_mode = display_mode;
	}

	if (Renderer::Ref().GetColorMode() != last_color_mode)
	{
		last_color_mode = Renderer::Ref().GetColorMode();
		for (std::vector<std::pair<Checkbox*, unsigned int> >::iterator iter = colorCheckboxes.begin(), end = colorCheckboxes.end(); iter != end; iter++)
		{
			unsigned int mode = (*iter).second;
			if (last_color_mode == mode)
				(*iter).first->SetChecked(true);
			else
				(*iter).first->SetChecked(false);
		}
	}
}

void RenderModesUI::OnDraw(VideoBuffer *buf)
{
#ifdef TOUCHUI
	if (interfaceSwap)
#endif
	{
		buf->DrawLine(line1Pos, 0, line1Pos, MENUSIZE, 255, 255, 255, 255);
		buf->DrawLine(line2Pos, 0, line2Pos, MENUSIZE, 255, 255, 255, 255);
	}
}

void RenderModesUI::OnKeyPress(int key, unsigned short character, unsigned short modifiers)
{
	if (key == SDLK_RETURN)
		this->toDelete = true;
}
