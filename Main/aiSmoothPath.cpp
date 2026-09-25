#include "StdAfx.h"
#include "aiSmoothPath.h"
#include "aiMoves.h"   // GetTransitionType( const IPathNetwork*, const SPathPlace&, const SPathPlace& )
#include <math.h>

namespace NAI
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// @0xa3ab0: is point i a corner? Compares the backward differences around i per axis; an axis is
// "smooth" when its delta repeats and the point actually moves along it. The inner equal-deltas
// recheck catches the all-three-points-coincide case.
bool NotSmooth( const vector<SPathPlace> &pts, int i )
{
	int nDXF = (int)pts[i].GetX() - (int)pts[i + 1].GetX();
	int nDXB = (int)pts[i - 1].GetX() - (int)pts[i].GetX();
	int nDYF = (int)pts[i].GetY() - (int)pts[i + 1].GetY();
	int nDYB = (int)pts[i - 1].GetY() - (int)pts[i].GetY();
	// @0xa3ab0: an axis is a "corner axis" when its backward delta CHANGES, OR the point actually MOVES along
	// that axis (`!=`). FIX (2026-07-01): the original reconstruction used `==` here, which inverted the
	// movement test -> NotSmooth only fired on full 90-degree L-corners (both axes reverse) and MISSED the
	// horizontal->diagonal junction (one axis keeps moving), so SmoothenPath left the dog-leg uncut. The retail
	// disasm is `!=` on both axes.
	if ( ( nDXF != nDXB || pts[i].GetX() != pts[i + 1].GetX() ) &&
	     ( nDYF != nDYB || pts[i].GetY() != pts[i + 1].GetY() ) )
	{
		if ( nDXF == nDXB && nDYF == nDYB )
			return false;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// @0xa3b20: length of the straight run ENDING at i -- same layer as i, constant backward step,
// no INACTIVE pose (nPose==3) on the run (an INACTIVE predecessor stops the scan).
int FindMaxPreSize( const vector<SPathPlace> &pts, int i )
{
	if ( pts[i - 1].GetPose() == 3 )
		return 0;
	int nDX = (int)pts[i].GetX() - (int)pts[i - 1].GetX();
	int nDY = (int)pts[i].GetY() - (int)pts[i - 1].GetY();
	int j = i - 1;
	while ( j > 0 )
	{
		if ( pts[j].GetLayer() != pts[i].GetLayer() ||
		     (int)pts[j].GetX() - (int)pts[j - 1].GetX() != nDX ||
		     (int)pts[j].GetY() - (int)pts[j - 1].GetY() != nDY ||
		     pts[j - 1].GetPose() == 3 )
			break;
		--j;
	}
	return i - j;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// @0xa3be0: length of the straight run STARTING at i (forward steps), stopping one short of the
// path's end; same INACTIVE rules.
int FindMaxPostSize( const vector<SPathPlace> &pts, int i )
{
	if ( pts[i + 1].GetPose() == 3 )
		return 0;
	int nDX = (int)pts[i + 1].GetX() - (int)pts[i].GetX();
	int nDY = (int)pts[i + 1].GetY() - (int)pts[i].GetY();
	int j = i + 1;
	int nLast = (int)pts.size() - 1;
	while ( j < nLast )
	{
		if ( pts[j].GetLayer() != pts[i].GetLayer() ||
		     (int)pts[j + 1].GetX() - (int)pts[j].GetX() != nDX ||
		     (int)pts[j + 1].GetY() - (int)pts[j].GetY() != nDY ||
		     pts[j + 1].GetPose() == 3 )
			break;
		++j;
	}
	return j - i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// @0xa3cc0: replace the span (i-nPre, i+nPost-1) -- exclusive ends -- with an interleave of nPre-1
// "pre" steps and nPost-1 "post" steps, scheduled proportionally (epsilon 0.01; ties prefer the
// longer remainder). Each generated point takes the stepped coords, point i's layer, and the
// pose/moving bits of the original point it replaces; the previous point's direction is set to the
// step's direction. The candidate sequence must be all-passable, internally connected, and
// connected to both untouched neighbours; otherwise the LONGER side shrinks by one (pre also slides
// the span start) and it retries while both sides still have >= 2 steps. The binary builds the
// candidates in a fixed 1000-place scratch with no bounds check (kept).
bool SmoothenPrePost( vector<SPathPlace> &pts, IPathNetwork *pNet, int nPre, int nPost, int i )
{
	SPathPlace buf[1000];
	int nStart = i - nPre;   // element index before the replaced span
	while ( nPre >= 2 )
	{
		if ( nPost < 2 )
			return false;
		double fRatio = (double)( nPre - 1 ) / (double)( nPost - 1 );
		int nA = nPre - 1, nB = nPost - 1;
		int nLayer = pts[i].GetLayer();
		unsigned nDirPre = pts[i - 1].GetDirection();
		unsigned nDirPost = pts[i].GetDirection();
		int nDXPre = (int)pts[i].GetX() - (int)pts[i - 1].GetX();
		int nDYPre = (int)pts[i].GetY() - (int)pts[i - 1].GetY();
		int nDXPost = (int)pts[i + 1].GetX() - (int)pts[i].GetX();
		int nDYPost = (int)pts[i + 1].GetY() - (int)pts[i].GetY();
		buf[0] = pts[nStart + 1];
		int nX = pts[nStart + 1].GetX();
		int nY = pts[nStart + 1].GetY();
		int nOrig = nStart + 1;   // original element whose pose/moving flags feed buf[k] (retail @0xa3deb: buf[1]
		                          // inherits from pts[nStart+1], not +2 -- flag-source off-by-one, geometry-neutral)
		int k = 0;
		while ( nA > 0 || nB > 0 )
		{
			bool bPre;
			double fShare = (double)nB * fRatio;
			if ( fabs( (double)nA - fShare ) < 0.01 )
				bPre = nA > nB;
			else
				bPre = (double)nA > fShare;
			if ( nA == 0 )
				bPre = false;
			unsigned nDir;
			if ( nB == 0 || bPre )
			{
				nDir = nDirPre;
				nX += nDXPre;
				nY += nDYPre;
				--nA;
			}
			else
			{
				nDir = nDirPost;
				nX += nDXPost;
				nY += nDYPost;
				--nB;
			}
			buf[k].SetDirection((unsigned short)nDir);
			SPathPlace p = pts[nOrig];   // pose/moving inherited
			p.SetXY(nX & 0xff, nY & 0xff);
			p.SetLayer(nLayer);
			p.SetIntegral(1);
			p.SetFinal(0);   // retail masks off the nFinal/n3D bit (word & 0xfc01 @0xa3ed0) on interleaved points
			++nOrig;
			buf[++k] = p;
		}
		int nCount = nPre + nPost - 2;   // == k; elements written back
		bool bOK = true;
		int j = 0;
		for ( ; j < nCount; ++j )
			if ( !pNet->IsPassable( buf[j] ) )
				break;
		if ( j == nCount )
		{
			for ( j = 0; j < nCount - 1; ++j )
				if ( GetTransitionType( pNet, buf[j], buf[j + 1] ) == TT_NO_WAY )
					break;
			if ( j == nCount - 1 )
			{
				if ( GetTransitionType( pNet, pts[nStart], buf[0] ) == TT_NO_WAY )
					bOK = false;
				if ( GetTransitionType( pNet, buf[nCount - 1], pts[i + nPost - 1] ) != TT_NO_WAY && bOK )
				{
					for ( int n = 0; n < nCount; ++n )
						pts[nStart + 1 + n] = buf[n];
					return true;
				}
			}
		}
		if ( nPre > nPost )
		{
			--nPre;
			++nStart;
		}
		else
		{
			--nPost;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// @0xa4060: one smoothing pass -- the first fixable corner wins.
bool SmoothenPathIter( CPath &path )
{
	vector<SPathPlace> &pts = path.points;
	if ( (int)pts.size() < 5 )
		return false;
	IPathNetwork *pNet = path.pNet.GetPtr();
	if ( pNet == 0 )
		return false;
	for ( int i = 2; i < (int)pts.size() - 2; ++i )
	{
		if ( !NotSmooth( pts, i ) )
			continue;
		int nPre = FindMaxPreSize( pts, i );
		if ( nPre <= 1 )
			continue;
		int nPost = FindMaxPostSize( pts, i );
		if ( nPost <= 1 )
			continue;
		if ( SmoothenPrePost( pts, pNet, nPre, nPost, i ) )
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// @0xa40f0: iterate to a fixpoint, capped at ~100 passes.
void SmoothenPath( CPath &path )
{
	int n = 0;
	bool b = SmoothenPathIter( path );
	while ( b )
	{
		if ( n > 100 )
			return;
		++n;
		b = SmoothenPathIter( path );
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
}  // namespace NAI
