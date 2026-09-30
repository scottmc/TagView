/*
 * Copyright 2026, Scott McCreary. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
// Barberpole.h
//
// A small animated "busy" indicator: diagonal stripes that scroll while
// running, an idle label when not. This is the same widget/technique
// already used in the user's ArmyKnife and Hare apps -- an 8x8 stipple
// `pattern` rotated one step per Pulse() and painted into an offscreen
// bitmap, then blitted, to avoid flicker -- kept here as its own,
// app-agnostic widget so it can be dropped into other apps unchanged.
//
// Original technique: public domain, jonas.sundstrom@kirilla.com
// (ArmyKnife's Barberpole).

#ifndef WIDGETKIT_BARBERPOLE_H
#define WIDGETKIT_BARBERPOLE_H

#include <Bitmap.h>
#include <Box.h>
#include <String.h>
#include <View.h>


class Barberpole : public BBox {
public:
								Barberpole(const char* name, uint32 flags,
									const char* idleText = "Idle");
	virtual						~Barberpole();

			void				Start();
			void				Pause();
			void				Stop();
			bool				IsRunning() const { return fIsRunning; }

	virtual	void				Pulse();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameMoved(BPoint point);
	virtual	void				FrameResized(float width, float height);

private:
			void				_CreateBitmap();
			void				_DrawOnBitmap();
			void				_LightenBitmapHighColor(rgb_color* color);

			bool				fIsRunning;
			pattern				fPattern;
			BBitmap*			fBitmap;
			BView*				fBitmapView;
			BString				fIdleText;
};

#endif // WIDGETKIT_BARBERPOLE_H
