/*
	This file is part of the GUI library.
	Copyright (C) 2008-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2008-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2008-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef GUI_BITMAP_FONT_H
#define GUI_BITMAP_FONT_H

#include "../ImageData.h"
#include "AbstractFont.h"
#include <Geometry/Rect.h>
#include <Util/IO/FileName.h>

#include <unordered_map>

namespace GUI {

/***
 **     BitmapFont ---|> AbstractFont
 **/
class BitmapFont : public AbstractFont{
		PROVIDES_TYPE_NAME(BitmapFont)
	public:
		/*! Load a .ttf or .otf file.
			Returns a BitmapFont or throws an exception.	*/
		static BitmapFont * createFont(const Util::FileName & fontFile,uint32_t fontSize,const std::string & charMap_utf8);
		
		/*
		    +cursor(0,0)                       _
              |screenOffset                    |
			  +_____________   _               |
			  |  ###### ### |  |               | lineHeight 
			  |  #     ###  |  |               |
			  | #       #   |  |               |
			  | ##      #   |  |               |
			  |  ########   |  | height        |
			  |    ##   #   |  |               |
			  |   #     #   |  |               |
			  |    #####    |  |               -
			  |_____________|  -
			       width
			|-------------|
			    xAdvance 

		*/
		
		struct Glyph{
			Geometry::Rect uvRect;
			Geometry::Rect_i screenRect;
			int xAdvance;

			Glyph() :  xAdvance(-1) {}
			Glyph(int _xAdvance) : xAdvance(_xAdvance) {}
			Glyph(const Geometry::Rect & _imageRect, const Geometry::Rect_i & _screenRect, int _xAdvance) :
					uvRect(_imageRect), screenRect(_screenRect),xAdvance(_xAdvance) {}
			Glyph(const Glyph & o) :
					uvRect(o.uvRect), screenRect(o.screenRect), xAdvance(o.xAdvance) {}

			bool isValid()const					{   return xAdvance>0;  }
			Glyph& operator=(const Glyph& o)  = default;
		};
		
		typedef std::unordered_map<uint32_t, Glyph> typefaceMap_t; // unicode -> Glyph

		BitmapFont(Util::Reference<ImageData> bitmap,int lineHeight);
		virtual ~BitmapFont();

		void addGlyph(uint32_t characterCode,uint32_t width, uint32_t height, const Geometry::Vec2i & textureOffset, const Geometry::Vec2i & screenOffset, int xAdvance);
		
		const Glyph & getGlyph(uint32_t characterCode)const{
			static const Glyph emptyGlyph;
			const auto it = glyphs.find(characterCode);
			return it == glyphs.end() ? emptyGlyph : it->second;
		}
		const Util::Reference<Util::Bitmap> & getBitmap() const {
			return bitmap->getBitmap();
		}

		// ---|> AbstractFont
		virtual void enable();
		virtual void disable();
		virtual void renderText(const Geometry::Vec2 & pos, const std::string & text, const Util::Color4ub & color);
		virtual Geometry::Vec2 getRenderedTextSize( const std::string & text);

	private:

		Util::Reference<ImageData> bitmap;
		typefaceMap_t glyphs;
};
}

#endif // GUI_BITMAP_FONT_H