/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2010-2013 blue42u, Jonathon Anderson <anderjon@umail.iu.edu>
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "hud.h"
#include "main.h"
#include "settings.h"
#include "util/numeric.h"
#include "log.h"
#include "gamedef.h"
#include "itemdef.h"
#include "inventory.h"
#include "tile.h"
#include "localplayer.h"

#include <IGUIStaticText.h>


Hud::Hud(video::IVideoDriver *driver, gui::IGUIEnvironment* guienv,
		gui::IGUIFont *font, u32 text_height, IGameDef *gamedef,
		LocalPlayer *player, Inventory *inventory) {
	this->driver      = driver;
	this->guienv      = guienv;
	this->font        = font;
	this->text_height = text_height;
	this->gamedef     = gamedef;
	this->player      = player;
	this->inventory   = inventory;
	
	screensize       = v2u32(0, 0);
	displaycenter    = v2s32(0, 0);
	hotbar_imagesize = 48;
	
	tsrc = gamedef->getTextureSource();
	
	v3f crosshair_color = g_settings->getV3F("crosshair_color");
	u32 cross_r = rangelim(myround(crosshair_color.X), 0, 255);
	u32 cross_g = rangelim(myround(crosshair_color.Y), 0, 255);
	u32 cross_b = rangelim(myround(crosshair_color.Z), 0, 255);
	u32 cross_a = rangelim(g_settings->getS32("crosshair_alpha"), 0, 255);
	crosshair_argb = video::SColor(cross_a, cross_r, cross_g, cross_b);
	
	v3f selectionbox_color = g_settings->getV3F("selectionbox_color");
	u32 sbox_r = rangelim(myround(selectionbox_color.X), 0, 255);
	u32 sbox_g = rangelim(myround(selectionbox_color.Y), 0, 255);
	u32 sbox_b = rangelim(myround(selectionbox_color.Z), 0, 255);
	selectionbox_argb = video::SColor(255, sbox_r, sbox_g, sbox_b);
	
	use_crosshair_image = tsrc->isKnownSourceImage("crosshair.png");

	hotbar_image = "";
	use_hotbar_image = false;
	hotbar_selected_image = "";
	use_hotbar_selected_image = false;
}


//NOTE: selectitem = 0 -> no selected; selectitem 1-based
void Hud::drawItem(v2s32 upperleftpos, s32 imgsize, s32 itemcount,
		InventoryList *mainlist, u16 selectitem, u16 direction)
{
	s32 padding = imgsize / 12;
	s32 height  = imgsize + padding * 2;
	s32 width   = itemcount * (imgsize + padding * 2);
	if (direction == HUD_DIR_TOP_BOTTOM || direction == HUD_DIR_BOTTOM_TOP) {
		width  = imgsize + padding * 2;
		height = itemcount * (imgsize + padding * 2);
	}
	s32 fullimglen = imgsize + padding * 2;

	// Position of upper left corner of bar
	v2s32 pos = upperleftpos;

	// Draw background color
	/*core::rect<s32> barrect(0,0,width,height);
	barrect += pos;
	video::SColor bgcolor(255,128,128,128);
	driver->draw2DRectangle(bgcolor, barrect, NULL);*/

	core::rect<s32> imgrect(0, 0, imgsize, imgsize);
	const video::SColor hbar_color(255, 255, 255, 255);
	const video::SColor hbar_colors[] = {hbar_color, hbar_color, hbar_color, hbar_color};

	if (hotbar_image != player->hotbar_image) {
		hotbar_image = player->hotbar_image;
		if (hotbar_image != "")
			use_hotbar_image = tsrc->isKnownSourceImage(hotbar_image);
		else
			use_hotbar_image = false;
	}

	if (hotbar_selected_image != player->hotbar_selected_image) {
		hotbar_selected_image = player->hotbar_selected_image;
		if (hotbar_selected_image != "")
			use_hotbar_selected_image = tsrc->isKnownSourceImage(hotbar_selected_image);
		else
			use_hotbar_selected_image = false;
	}

	if (use_hotbar_image) {
		core::rect<s32> imgrect2(-padding/2, -padding/2, width+padding/2, height+padding/2);
		core::rect<s32> rect2 = imgrect2 + pos;
		video::ITexture *texture = tsrc->getTexture(hotbar_image);
		core::dimension2di imgsize(texture->getOriginalSize());
		driver->draw2DImage(texture, rect2,
			core::rect<s32>(core::position2d<s32>(0,0), imgsize),
			NULL, hbar_colors, true);
	}

	for (s32 i = 0; i < itemcount; i++)
	{
		const ItemStack &item = mainlist->getItem(i);

		v2s32 steppos;
		switch (direction) {
			case HUD_DIR_RIGHT_LEFT:
				steppos = v2s32(-(padding + i * fullimglen), padding);
				break;
			case HUD_DIR_TOP_BOTTOM:
				steppos = v2s32(padding, padding + i * fullimglen);
				break;
			case HUD_DIR_BOTTOM_TOP:
				steppos = v2s32(padding, -(padding + i * fullimglen));
				break;
			default:
				steppos = v2s32(padding + i * fullimglen, padding);	
		}
			
		core::rect<s32> rect = imgrect + pos + steppos;

		if (selectitem == i + 1) {
			if (use_hotbar_selected_image) {
				core::rect<s32> imgrect2(-padding*2, -padding*2, height, height);
				rect = imgrect2 + pos + steppos;
				video::ITexture *texture = tsrc->getTexture(hotbar_selected_image);
				core::dimension2di imgsize(texture->getOriginalSize());
				driver->draw2DImage(texture, rect,
					core::rect<s32>(core::position2d<s32>(0,0), imgsize),
					NULL, hbar_colors, true);
				rect = imgrect + pos + steppos;
			} else {
				rect = imgrect + pos + steppos;
				video::SColor c_outside(255,255,0,0);
				//video::SColor c_outside(255,0,0,0);
				//video::SColor c_inside(255,192,192,192);
				s32 x1 = rect.UpperLeftCorner.X;
				s32 y1 = rect.UpperLeftCorner.Y;
				s32 x2 = rect.LowerRightCorner.X;
				s32 y2 = rect.LowerRightCorner.Y;
				// Black base borders
				driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x1 - padding, y1 - padding),
						v2s32(x2 + padding, y1)
					), NULL);
				driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x1 - padding, y2),
						v2s32(x2 + padding, y2 + padding)
					), NULL);
				driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x1 - padding, y1),
						v2s32(x1, y2)
					), NULL);
				driver->draw2DRectangle(c_outside,
					core::rect<s32>(
						v2s32(x2, y1),
						v2s32(x2 + padding, y2)
					), NULL);
				/*// Light inside borders
				driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x1 - padding/2, y1 - padding/2),
						v2s32(x2 + padding/2, y1)
					), NULL);
				driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x1 - padding/2, y2),
						v2s32(x2 + padding/2, y2 + padding/2)
					), NULL);
				driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x1 - padding/2, y1),
						v2s32(x1, y2)
					), NULL);
				driver->draw2DRectangle(c_inside,
					core::rect<s32>(
						v2s32(x2, y1),
						v2s32(x2 + padding/2, y2)
					), NULL);
				*/
			}
		}

		video::SColor bgcolor2(128, 0, 0, 0);
		if (!use_hotbar_image)
			driver->draw2DRectangle(bgcolor2, rect, NULL);
		drawItemStack(driver, font, item, rect, NULL, gamedef);
	}
}


void Hud::drawLuaElements() {
	for (size_t i = 0; i != player->hud.size(); i++) {
		HudElement *e = player->hud[i];
		if (!e)
			continue;
		
		v2s32 pos(e->pos.X * screensize.X, e->pos.Y * screensize.Y);
		switch (e->type) {
			case HUD_ELEM_IMAGE: {
				video::ITexture *texture = tsrc->getTexture(e->text);
				if (!texture)
					continue;

				const video::SColor color(255, 255, 255, 255);
				const video::SColor colors[] = {color, color, color, color};
				core::dimension2di imgsize(texture->getOriginalSize());
				v2s32 dstsize(imgsize.Width * e->scale.X,
				              imgsize.Height * e->scale.Y);
				if (e->scale.X < 0)
					dstsize.X = screensize.X * (e->scale.X * -0.01);
				if (e->scale.Y < 0)
					dstsize.Y = screensize.Y * (e->scale.Y * -0.01);
				v2s32 offset((e->align.X - 1.0) * dstsize.X / 2,
				             (e->align.Y - 1.0) * dstsize.Y / 2);
				core::rect<s32> rect(0, 0, dstsize.X, dstsize.Y);
				rect += pos + offset + v2s32(e->offset.X, e->offset.Y);
				driver->draw2DImage(texture, rect,
					core::rect<s32>(core::position2d<s32>(0,0), imgsize),
					NULL, colors, true);
				break; }
			case HUD_ELEM_TEXT: {
				video::SColor color(255, (e->number >> 16) & 0xFF,
										 (e->number >> 8)  & 0xFF,
										 (e->number >> 0)  & 0xFF);
				core::rect<s32> size(0, 0, e->scale.X, text_height * e->scale.Y);
				std::wstring text = narrow_to_wide(e->text);
				core::dimension2d<u32> textsize = font->getDimension(text.c_str());
				v2s32 offset((e->align.X - 1.0) * (textsize.Width / 2),
				             (e->align.Y - 1.0) * (textsize.Height / 2));
				v2s32 offs(e->offset.X, e->offset.Y);
				font->draw(text.c_str(), size + pos + offset + offs, color);
				break; }
			case HUD_ELEM_STATBAR: {
				v2s32 offs(e->offset.X, e->offset.Y);
				drawStatbar(pos, HUD_CORNER_UPPER, e->dir, e->text, e->number, offs);
				break; }
			case HUD_ELEM_INVENTORY: {
				InventoryList *inv = inventory->getList(e->text);
				drawItem(pos, hotbar_imagesize, e->number, inv, e->item, e->dir);
				break; }
			default:
				infostream << "Hud::drawLuaElements: ignoring drawform " << e->type <<
					" of hud element ID " << i << " due to unrecognized type" << std::endl;
		}
	}
}


void Hud::drawStatbar(v2s32 pos, u16 corner, u16 drawdir, std::string texture, s32 count, v2s32 offset) {
	const video::SColor color(255, 255, 255, 255);
	const video::SColor colors[] = {color, color, color, color};
	
	video::ITexture *stat_texture = tsrc->getTexture(texture);
	if (!stat_texture)
		return;
		
	core::dimension2di srcd(stat_texture->getOriginalSize());

	v2s32 p = pos;
	if (corner & HUD_CORNER_LOWER)
		p -= srcd.Height;

	p += offset;

	v2s32 steppos;
	switch (drawdir) {
		case HUD_DIR_RIGHT_LEFT:
			steppos = v2s32(-1, 0);
			break;
		case HUD_DIR_TOP_BOTTOM:
			steppos = v2s32(0, 1);
			break;
		case HUD_DIR_BOTTOM_TOP:
			steppos = v2s32(0, -1);
			break;
		default:
			steppos = v2s32(1, 0);	
	}
	steppos.X *= srcd.Width;
	steppos.Y *= srcd.Height;
	
	for (s32 i = 0; i < count / 2; i++)
	{
		core::rect<s32> srcrect(0, 0, srcd.Width, srcd.Height);
		core::rect<s32> dstrect(srcrect);

		dstrect += p;
		driver->draw2DImage(stat_texture, dstrect, srcrect, NULL, colors, true);
		p += steppos;
	}
	
	if (count % 2 == 1)
	{
		core::rect<s32> srcrect(0, 0, srcd.Width / 2, srcd.Height);
		core::rect<s32> dstrect(srcrect);

		dstrect += p;
		driver->draw2DImage(stat_texture, dstrect, srcrect, NULL, colors, true);
	}
}


void Hud::drawHotbar(v2s32 centerlowerpos, s32 halfheartcount, u16 playeritem, s32 breath) {
	InventoryList *mainlist = inventory->getList("main");
	if (mainlist == NULL) {
		errorstream << "draw_hotbar(): mainlist == NULL" << std::endl;
		return;
	}
	
	s32 hotbar_itemcount = player->hud_hotbar_itemcount;
	s32 padding = hotbar_imagesize / 12;
	s32 width = hotbar_itemcount * (hotbar_imagesize + padding * 2);
	v2s32 pos = centerlowerpos - v2s32(width / 2, hotbar_imagesize + padding * 3);
	
	if (player->hud_flags & HUD_FLAG_HOTBAR_VISIBLE)
		drawItem(pos, hotbar_imagesize, hotbar_itemcount, mainlist, playeritem + 1, 0);
	if (player->hud_flags & HUD_FLAG_HEALTHBAR_VISIBLE)
		drawStatbar(pos - v2s32(0, 4), HUD_CORNER_LOWER, HUD_DIR_LEFT_RIGHT,
				"heart.png", halfheartcount, v2s32(0, 0));
	if (player->hud_flags & HUD_FLAG_BREATHBAR_VISIBLE && breath <= 10)
		drawStatbar(pos - v2s32(-180, 4), HUD_CORNER_LOWER, HUD_DIR_LEFT_RIGHT,
				"bubble.png", breath*2, v2s32(0, 0));
}


void Hud::drawCrosshair() {
	if (!(player->hud_flags & HUD_FLAG_CROSSHAIR_VISIBLE))
		return;
		
	if (use_crosshair_image) {
		video::ITexture *crosshair = tsrc->getTexture("crosshair.png");
		v2u32 size  = crosshair->getOriginalSize();
		v2s32 lsize = v2s32(displaycenter.X - (size.X / 2),
							displaycenter.Y - (size.Y / 2));
		driver->draw2DImage(crosshair, lsize,
				core::rect<s32>(0, 0, size.X, size.Y),
				0, crosshair_argb, true);
	} else {
		driver->draw2DLine(displaycenter - v2s32(10, 0),
				displaycenter + v2s32(10, 0), crosshair_argb);
		driver->draw2DLine(displaycenter - v2s32(0, 10),
				displaycenter + v2s32(0, 10), crosshair_argb);
	}
}


void Hud::drawSelectionBoxes(std::vector<aabb3f> &hilightboxes) {
	for (std::vector<aabb3f>::const_iterator
			i = hilightboxes.begin();
			i != hilightboxes.end(); i++) {
		driver->draw3DBox(*i, selectionbox_argb);
	}
}


void Hud::resizeHotbar() {
	if (screensize.Y <= 800)
		hotbar_imagesize = 32;
	else if (screensize.Y <= 1280)
		hotbar_imagesize = 48;
	else
		hotbar_imagesize = 64;
}

void drawItemStack(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		IGameDef *gamedef)
{
	if(item.empty())
		return;
	
	const ItemDefinition &def = item.getDefinition(gamedef->idef());
	video::ITexture *texture = gamedef->idef()->getInventoryTexture(def.name, gamedef);

	// Draw the inventory texture
	if(texture != NULL)
	{
		const video::SColor color(255,255,255,255);
		const video::SColor colors[] = {color,color,color,color};
		driver->draw2DImage(texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			clip, colors, true);
	}

	if(def.type == ITEM_TOOL && item.wear != 0)
	{
		// Draw a progressbar
		float barheight = rect.getHeight()/16;
		float barpad_x = rect.getWidth()/16;
		float barpad_y = rect.getHeight()/16;
		core::rect<s32> progressrect(
			rect.UpperLeftCorner.X + barpad_x,
			rect.LowerRightCorner.Y - barpad_y - barheight,
			rect.LowerRightCorner.X - barpad_x,
			rect.LowerRightCorner.Y - barpad_y);

		// Shrink progressrect by amount of tool damage
		float wear = item.wear / 65535.0;
		int progressmid =
			wear * progressrect.UpperLeftCorner.X +
			(1-wear) * progressrect.LowerRightCorner.X;

		// Compute progressbar color
		//   wear = 0.0: green
		//   wear = 0.5: yellow
		//   wear = 1.0: red
		video::SColor color(255,255,255,255);
		int wear_i = MYMIN(floor(wear * 600), 511);
		wear_i = MYMIN(wear_i + 10, 511);
		if(wear_i <= 255)
			color.set(255, wear_i, 255, 0);
		else
			color.set(255, 255, 511-wear_i, 0);

		core::rect<s32> progressrect2 = progressrect;
		progressrect2.LowerRightCorner.X = progressmid;
		driver->draw2DRectangle(color, progressrect2, clip);

		color = video::SColor(255,0,0,0);
		progressrect2 = progressrect;
		progressrect2.UpperLeftCorner.X = progressmid;
		driver->draw2DRectangle(color, progressrect2, clip);
	}

	if(font != NULL && item.count >= 2)
	{
		// Get the item count as a string
		std::string text = itos(item.count);
		v2u32 dim = font->getDimension(narrow_to_wide(text).c_str());
		v2s32 sdim(dim.X,dim.Y);

		core::rect<s32> rect2(
			/*rect.UpperLeftCorner,
			core::dimension2d<u32>(rect.getWidth(), 15)*/
			rect.LowerRightCorner - sdim,
			sdim
		);

		video::SColor bgcolor(128,0,0,0);
		driver->draw2DRectangle(bgcolor, rect2, clip);

		video::SColor color(255,255,255,255);
		font->draw(text.c_str(), rect2, color, false, false, clip);
	}
}

