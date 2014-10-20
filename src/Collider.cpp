/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street, 
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
#include <algorithm>
#include <limits>
#include <math.h>
#include "inc/Collider.h"
#include "inc/Segment.h"
#include "inc/Slot.h"

#define ISQRT2 0.707106781f

using namespace graphite2;

// Initialize the Collider to hold the basic movement limits for the
// target slot, the one we are focusing on fixing.
void ShiftCollider::initSlot(Segment *seg, Slot *aSlot, const Rect &limit, float margin)
{
    int i;
    float max, min;
    const GlyphCache &gc = seg->getFace()->glyphs();
    unsigned short gid = aSlot->gid();
    for (i = 0; i < 4; ++i)
    {
        switch (i) {
            case 0 :	// x direction
                min = limit.bl.x + aSlot->origin().x + gc.getBoundingMetric(gid, 0);
                max = limit.tr.x + aSlot->origin().x + gc.getBoundingMetric(gid, 2);
                break;
            case 1 :	// y direction
                min = limit.bl.y + aSlot->origin().y + gc.getBoundingMetric(gid, 1);
                max = limit.tr.y + aSlot->origin().y + gc.getBoundingMetric(gid, 3);
                break;
            case 2 :	// sum (negatively sloped diagonal)
                min = 2.f * std::max(limit.bl.x, -limit.tr.y) + aSlot->origin().x + aSlot->origin().y + gc.getBoundingMetric(gid, 4);
                max = 2.f * std::min(limit.tr.x, -limit.bl.y) + aSlot->origin().x + aSlot->origin().y + gc.getBoundingMetric(gid, 6);
                break;
            case 3 :	// diff (positively sloped diagonal)
                min = 2.f * std::max(limit.bl.x, limit.bl.y) + aSlot->origin().x - aSlot->origin().y + gc.getBoundingMetric(gid, 5);
                max = 2.f * std::min(limit.tr.x, limit.tr.y) + aSlot->origin().x - aSlot->origin().y + gc.getBoundingMetric(gid, 7);
                break;
        }
        _ranges[i].clear();
        _ranges[i].add(min, max);
    }
    _target = aSlot;
    _limit = limit;
    _margin = margin;
}

// Adjust the movement limits for the target to avoid having it collide
// with the given slot. Also determine if there is in fact a
// collision between the target and the given slot.
bool ShiftCollider::mergeSlot(Segment *seg, Slot *slot)
{
    bool isCol = true;
    const float tx = _target->origin().x;
    const float ty = _target->origin().y;
    const float td = tx - ty;
    const float ts = tx + ty;
    const float sx = slot->origin().x;
    const float sy = slot->origin().y;
    const float sd = sx - sy;
    const float ss = sx + sy;
    float vmin, vmax;
    float omin, omax, otmin, otmax;
    float cmin, cmax;
    const GlyphCache &gc = seg->getFace()->glyphs();
    const unsigned short gid = slot->gid();
    const unsigned short tgid = _target->gid();
    
    // Process main bounding octabox.
    for (int i = 0; i < 4; ++i)
    {
        uint16 m = _margin * (i > 1 ? ISQRT2 : 1.);
        switch (i) {
            case 0 :	// x direction
                vmin = std::max(std::max(gc.getBoundingMetric(gid, 0) + sx,
                                gc.getBoundingMetric(gid, 5) + sd + gc.getBoundingMetric(tgid, 1) + ty),
                                gc.getBoundingMetric(gid, 4) + ss - gc.getBoundingMetric(tgid, 3) - ty);
                vmax = std::min(std::min(gc.getBoundingMetric(gid, 2) + sx,
                                gc.getBoundingMetric(gid, 7) + sd + gc.getBoundingMetric(tgid, 3) + ty),
                                gc.getBoundingMetric(gid, 6) + ss - gc.getBoundingMetric(tgid, 1) - ty);
                otmin = gc.getBoundingMetric(tgid, 1) + ty;
                otmax = gc.getBoundingMetric(tgid, 3) + ty;
                omin = gc.getBoundingMetric(gid, 1) + sy;
                omax = gc.getBoundingMetric(gid, 3) + sy;
                cmin = _limit.bl.x + tx + gc.getBoundingMetric(tgid, 0);
                cmax = _limit.tr.x + tx + gc.getBoundingMetric(tgid, 2);
                break;
            case 1 :	// y direction
                vmin = std::max(std::max(gc.getBoundingMetric(gid, 1) + sy,
                                gc.getBoundingMetric(tgid, 0) + tx - gc.getBoundingMetric(gid, 7) - sd),
                                gc.getBoundingMetric(gid, 4) + ss - gc.getBoundingMetric(tgid, 2) - tx);
                vmax = std::min(std::min(gc.getBoundingMetric(gid, 3) + sy,
                                gc.getBoundingMetric(tgid, 2) + tx - gc.getBoundingMetric(gid, 5) - sd),
                                gc.getBoundingMetric(gid, 6) + ss - gc.getBoundingMetric(tgid, 0) - tx);
                otmin = gc.getBoundingMetric(tgid, 0) + tx;
                otmax = gc.getBoundingMetric(tgid, 2) + tx;
                omin = gc.getBoundingMetric(gid, 0) + sx;
                omax = gc.getBoundingMetric(gid, 2) + sx;
                cmin = _limit.bl.y + ty + gc.getBoundingMetric(tgid, 1);
                cmax = _limit.tr.y + ty + gc.getBoundingMetric(tgid, 3);
                break;
            case 2 :    // sum
                vmin = std::max(std::max(gc.getBoundingMetric(gid, 4) + ss,
                                2 * gc.getBoundingMetric(gid, 1) + 2 * sy + gc.getBoundingMetric(tgid, 5) + td),
                                2 * gc.getBoundingMetric(gid, 0) + 2 * sx - gc.getBoundingMetric(tgid, 7) - td);
                vmax = std::min(std::min(gc.getBoundingMetric(gid, 6) + ss,
                                2 * gc.getBoundingMetric(gid, 3) + 2 * sy + gc.getBoundingMetric(tgid, 7) + td),
                                2 * gc.getBoundingMetric(gid, 2) + 2 * sx - gc.getBoundingMetric(tgid, 5) - td);
                otmin = gc.getBoundingMetric(tgid, 5) + td;
                otmax = gc.getBoundingMetric(tgid, 7) + td;
                omin = gc.getBoundingMetric(gid, 5) + sd;
                omax = gc.getBoundingMetric(gid, 7) + sd;
                cmin = _limit.bl.x + _limit.bl.y + ts + gc.getBoundingMetric(tgid, 4);
                cmax = _limit.tr.x + _limit.tr.y + ts + gc.getBoundingMetric(tgid, 6);
                break;
            case 3 :    // diff
                vmin = std::max(std::max(gc.getBoundingMetric(gid, 5) + sd,
                                2 * gc.getBoundingMetric(gid, 0) + 2 * sx - gc.getBoundingMetric(tgid, 6) - ts),
                                gc.getBoundingMetric(tgid, 4) + ts - 2 * gc.getBoundingMetric(gid, 3) - 2 * sy);
                vmax = std::min(std::min(gc.getBoundingMetric(gid, 7) + sd,
                                2 * gc.getBoundingMetric(gid, 2) + 2 * sx - gc.getBoundingMetric(tgid, 4) - ts),
                                gc.getBoundingMetric(tgid, 6) + ts - 2 * gc.getBoundingMetric(gid, 1) - 2 * sy);
                otmin = gc.getBoundingMetric(tgid, 4) + ts;
                otmax = gc.getBoundingMetric(tgid, 6) + ts;
                omin = gc.getBoundingMetric(gid, 4) + ss;
                omax = gc.getBoundingMetric(gid, 6) + ss;
                cmin = _limit.bl.x - _limit.tr.y + td + gc.getBoundingMetric(tgid, 5);
                cmax = _limit.tr.x - _limit.bl.y + td + gc.getBoundingMetric(tgid, 7);
                break;
            default :
                continue;
        }
        if ((vmin < cmin - m && vmax < cmin - m) || (vmin > cmax + m && vmax > cmax + m)
            || (omin < otmin - m && omax < otmin - m) || (omin > otmax + m && omax > otmax + m))
        {
            isCol = false;
            continue;
        }
        
        // Process sub-boxes that are defined for this glyph.
        // We only need to do this if there was in fact a collision with the main octabox.
        uint8 numsub = gc.numSubBounds(gid);
        if (numsub > 0)
        {
            bool anyhits = false;
            for (int j = 0; j < numsub; ++j)
            {
                switch (i) {
                    case 0 :    // x
                        vmin = std::max(std::max(gc.getSubBoundingMetric(gid, j, 0) + sx,
                                        gc.getSubBoundingMetric(gid, j, 5) + sd + gc.getBoundingMetric(tgid, 1) + ty),
                                        gc.getSubBoundingMetric(gid, j, 4) + ss - gc.getBoundingMetric(tgid, 3) - ty);
                        vmax = std::min(std::min(gc.getSubBoundingMetric(gid, j, 2) + sx,
                                        gc.getSubBoundingMetric(gid, j, 7) + sd + gc.getBoundingMetric(tgid, 3) + ty),
                                        gc.getSubBoundingMetric(gid, j, 6) + ss - gc.getBoundingMetric(tgid, 1) - ty);
                        omin = gc.getSubBoundingMetric(gid, j, 1) + sy;
                        omax = gc.getSubBoundingMetric(gid, j, 3) + sy;
                        break;
                    case 1 :    // y
                        vmin = std::max(std::max(gc.getSubBoundingMetric(gid, j, 1) + sy,
                                        gc.getBoundingMetric(tgid, 0) + tx - gc.getSubBoundingMetric(gid, j, 7) - sd),
                                        gc.getSubBoundingMetric(gid, j, 4) + ss - gc.getBoundingMetric(tgid, 2) - tx);
                        vmax = std::min(std::min(gc.getSubBoundingMetric(gid, j, 3) + sy,
                                        gc.getBoundingMetric(tgid, 2) + tx - gc.getSubBoundingMetric(gid, j, 5) - sd),
                                        gc.getSubBoundingMetric(gid, j, 6) + ss - gc.getBoundingMetric(tgid, 0) - tx);
                        omin = gc.getSubBoundingMetric(gid, j, 0) + sx;
                        omax = gc.getSubBoundingMetric(gid, j, 2) + sx;
                        break;
                    case 2 :    // sum
                        vmin = std::max(std::max(gc.getSubBoundingMetric(gid, j, 4) + ss,
                                        2 * gc.getSubBoundingMetric(gid, j, 1) + 2 * sy + gc.getBoundingMetric(tgid, 5) + td),
                                        2 * gc.getSubBoundingMetric(gid, j, 0) + 2 * sx - gc.getBoundingMetric(tgid, 7) - td);
                        vmax = std::min(std::min(gc.getSubBoundingMetric(gid, j, 6) + ss,
                                        2 * gc.getSubBoundingMetric(gid, j, 3) + 2 * sy + gc.getBoundingMetric(tgid, 7) + td),
                                        2 * gc.getSubBoundingMetric(gid, j, 2) + 2 * sx - gc.getBoundingMetric(tgid, 5) - td);
                        omin = gc.getSubBoundingMetric(gid, j, 5) + sd;
                        omax = gc.getSubBoundingMetric(gid, j, 7) + sd;
                        break;
                    case 3 :    // diff
                        vmin = std::max(std::max(gc.getSubBoundingMetric(gid, j, 5) + sd,
                                        2 * gc.getSubBoundingMetric(gid, j, 0) + 2 * sx - gc.getBoundingMetric(tgid, 6) - ts),
                                        gc.getBoundingMetric(tgid, 4) + ts - 2 * gc.getSubBoundingMetric(gid, j, 3) - 2 * sy);
                        vmax = std::min(std::min(gc.getSubBoundingMetric(gid, j, 7) + sd,
                                        2 * gc.getSubBoundingMetric(gid, j, 2) + 2 * sx - gc.getBoundingMetric(tgid, 4) - ts),
                                        gc.getBoundingMetric(tgid, 6) + ts - 2 * gc.getSubBoundingMetric(gid, j, 1) - 2 * sy);
                        omin = gc.getSubBoundingMetric(gid, j, 4) + ss;
                        omax = gc.getSubBoundingMetric(gid, j, 6) + ss;
                        break;
                }
                if ((vmin < cmin - m && vmax < cmin - m) || (vmin > cmax + m && vmax > cmax + m)
                    		|| (omin < otmin - m && omax < otmin - m) || (omin > otmax + m && omax > otmax + m))
                    continue;
                _ranges[i].remove(vmin, vmax);
                anyhits = true;
            }
            if (!anyhits)
                isCol = false;
        }
        else
            _ranges[i].remove(vmin, vmax);
    }
    return isCol;
}


Position ShiftCollider::resolve(Segment *seg, bool &isCol, const Position &currshift, GR_MAYBE_UNUSED json * const dbgout)
{
    const GlyphCache &gc = seg->getFace()->glyphs();
    int gid = _target->gid();
    float margin;
    float tlen, torig;
    float totald = std::numeric_limits<float>::max();
    Position totalp;
    // float cmax, cmin;
    bool isGoodFit, tIsGoodFit = false;
    IntervalSet aFit;
#if !defined GRAPHITE2_NTRACING
    if (dbgout)
    {
        *dbgout << json::object
                << "slot" << objectid(dslot(seg, _target)) 
                << "ranges" << json::array;
    }
#endif
    for (int i = 0; i < 4; ++i)
    {
        float bestc = std::numeric_limits<float>::max();
        float bestd = bestc;
        float cmin, cmax;
        // Calculate the margin depending on whether we are moving diagonally or not:
        margin = seg->collisionInfo(_target)->margin() * (i > 1 ? ISQRT2 : 1.f);
        switch (i) {
            case 0 :	// x direction
                tlen = gc.getBoundingMetric(gid, 2) - gc.getBoundingMetric(gid, 0);
                torig = _target->origin().x + currshift.x + gc.getBoundingMetric(gid, 0);
                cmin = _limit.bl.x + torig;
                cmax = _limit.tr.x + torig;
                break;
            case 1 :	// y direction
                tlen = gc.getBoundingMetric(gid, 3) - gc.getBoundingMetric(gid, 1);
                torig = _target->origin().y + currshift.y + gc.getBoundingMetric(gid, 1);
                cmin = _limit.bl.y + torig;
                cmax = _limit.tr.y + torig;
                break;
            case 2 :	// sum (negatively-sloped diagonals)
                tlen = gc.getBoundingMetric(gid, 6) - gc.getBoundingMetric(gid, 4);
                torig = _target->origin().x + _target->origin().y + currshift.x + currshift.y + gc.getBoundingMetric(gid, 4);
                cmin = _limit.bl.x + _limit.bl.y + torig;
                cmax = _limit.tr.x + _limit.tr.y + torig;
                break;
            case 3 :	// diff (positively-sloped diagonals)
                tlen = gc.getBoundingMetric(gid, 7) - gc.getBoundingMetric(gid, 5);
                torig = _target->origin().x - _target->origin().y + currshift.x - currshift.y + gc.getBoundingMetric(gid, 5);
                cmin = _limit.bl.x - _limit.tr.y + torig;
                cmax = _limit.tr.x - _limit.bl.y + torig;
                break;
        }
        isGoodFit = true;
        aFit = _ranges[i].locate(torig, torig + tlen);
        bestd = aFit.findBestWithMarginAndLimits(0., margin, cmin - torig, cmax - torig, isGoodFit);
#if !defined GRAPHITE2_NTRACING
        if (dbgout)
        {
            *dbgout << json::object
                        << "testorigin" << torig
                        << "testlen" << tlen
                        << "bestfit" << bestd
                        << "ranges";
            debug(dbgout, seg, i);
            *dbgout << "fits" << json::flat << json::array;
            for (IntervalSet::ivtpair s = aFit.begin(), e = aFit.end(); s != e; ++s)
                *dbgout << Position(s->first, s->second);
            *dbgout << json::close << json::close;
        }
#endif
        //bestd = _ranges[i].bestfit(torig - margin, torig + tlen + margin, isGoodFit);
        // bestd += bestd > 0.f ? -margin : margin;
        if ((isGoodFit && !tIsGoodFit) || ((isGoodFit || !tIsGoodFit) && fabs(bestd) * (i > 1 ? ISQRT2 : 1.f) < totald))
        {
            totald = fabs(bestd) * (i > 1 ? ISQRT2 : 1.);
            tIsGoodFit = isGoodFit;
            switch (i) {
                case 0 : totalp = Position(bestd, 0.); break;
                case 1 : totalp = Position(0., bestd); break;
                case 2 : totalp = Position(bestd, bestd); break;
                case 3 : totalp = Position(bestd, -bestd); break;
            }
        }
    }
#if !defined GRAPHITE2_NTRACING
    if (dbgout)
        *dbgout << json::close << json::close;
#endif
    isCol = !tIsGoodFit;
    return totalp;
}



void KernCollider::initSlot(Segment *seg, Slot *aSlot, const Rect &limit, float margin)
{
    const GlyphCache &gc = seg->getFace()->glyphs();
    unsigned short gid = aSlot->gid();
    float min = limit.bl.x + aSlot->origin().x + gc.getBoundingMetric(gid, 0);
    float max = limit.tr.x + aSlot->origin().x + gc.getBoundingMetric(gid, 2);
    int i;
    for (i = 0; i < 4; ++i)
    {
        _ranges[i].clear();
        _ranges[i].add(min, max);
    }
    _target = aSlot;
    _limit = limit;
    _margin = margin;
}

bool KernCollider::mergeSlot(Segment *seg, Slot *slot)
{
    
}

Position KernCollider::resolve(Segment *seg, bool &isCol, const Position &currshift, GR_MAYBE_UNUSED json * const dbgout)
{

}

// Initialize the structure for the given slot.
SlotCollision::SlotCollision(Segment *seg, Slot *slot)
{
    uint16 gid = slot->gid();
    uint16 aCol = seg->silf()->aCollision();
/*    _limit = Rect(Position(seg->glyphAttr(gid, aCol+1) + slot->origin().x,
                           seg->glyphAttr(gid, aCol+2) + slot->origin().y),
                  Position(seg->glyphAttr(gid, aCol+3) + slot->origin().x,
                           seg->glyphAttr(gid, aCol+4) + slot->origin().y)); */
    _limit = Rect(Position(seg->glyphAttr(gid, aCol+1), seg->glyphAttr(gid, aCol+2)),
                  Position(seg->glyphAttr(gid, aCol+3), seg->glyphAttr(gid, aCol+4)));
    _margin = seg->glyphAttr(gid, aCol+5);
    _flags = seg->glyphAttr(gid, aCol);
}

