/*	VIEW3D
	Copyright (C) 2008 John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/
#ifndef V3D_RENDER_H
#define V3D_RENDER_H

#include <vector>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoBaseColor.h>

extern const SbColor WHITE;
extern const SbColor RED;
extern const SbColor GREEN;
extern const SbColor YELLOW;
extern const SbColor BLUE;
extern const SbColor ORANGE;
extern const SbColor PURPLE;
extern const SbColor CYAN;

SoSeparator *text(const SbVec3f &left, const char *str, const SbColor &c=WHITE);
SoSeparator *sphere(const SbVec3f &C, const double &r=0.1, const SbColor &c=YELLOW);
SoSeparator *cylinder(const SbVec3f &A, const SbVec3f &B, const double &radius=0.1, const SbColor &c=YELLOW);
SoSeparator *cone(const SbVec3f &A, const SbVec3f &B, const double &r, const SbColor &c = YELLOW);
SoSeparator *axes(const double &size=1.0, double thickness=0.0, bool labelled=true);
SoSeparator *arrow(const SbVec3f &A, const SbVec3f &B, const SbColor &c=YELLOW, const char *label=NULL, double thickness=0.05);
SoSeparator *face(const SbVec3f &n, const std::vector<SbVec3f> &vertices, const SbColor &c=YELLOW);

#endif

