// KTP: per-usercmd aim/movement sampling, fed from SV_PlayerRunPreThink.
//
// THIS IS A SENSOR, NOT A DETECTOR, and the split is deliberate rather than tidiness:
// it measures the geometry of sustained-fire aim motion and reports it. It holds no
// thresholds and reaches no conclusion -- whatever consumes these numbers decides what
// they mean. Keep it that way. A judgement compiled in here would also be published
// here, and a threshold a reader can see is a threshold they can sit just outside.
//
// WHY STREAMING. The offline reference buffers every frame of a window and fits at the
// end. That is fine offline and wrong here: this runs in the game frame on a live fleet
// at up to sv_maxcmdrate 500 per player. A least-squares line needs only five running
// sums, so the fit is exact with O(1) work and fixed memory -- no buffer, no allocation,
// no I/O. The entloop/CLog fopen incident is the precedent for what per-frame work must
// never do.
#ifndef KTP_AIM_ACCUM_H
#define KTP_AIM_ACCUM_H

namespace KTPAim
{
	// Recording bounds, not detection bounds: a window this short cannot support a
	// meaningful line fit at all, so reporting it would be noise rather than evidence.
	const int    GAP_BRIDGE = 2;      // non-attack usercmds tolerated inside one burst
	const int    MIN_FRAMES = 10;
	const double MIN_DUR    = 0.40;   // seconds

	// How many windows to keep per player between flushes. Retained by SMALLEST residual,
	// because that is the tail any consumer cares about and it bounds memory at a fixed
	// cost regardless of how long a player stays connected.
	const int    KEEP_WINDOWS = 8;
}

// Geometry of one sustained-fire window. Reported as-is.
struct KTPWindowStat
{
	double dur;      // seconds of sustained fire
	double slope;    // deg/s, signed
	double rms;      // residual about the fitted line, degrees
	int    n;        // usercmd samples in the window
};

struct KTPFireWindow
{
	int    n;
	double tFirst, tLast;                 // tLast = last ATTACKING sample, never a bridged one
	double sT, sTT, sP, sPP, sTP;         // running sums for the exact least-squares fit

	// A bridged non-attack sample only belongs to the window if a later attack sample
	// confirms the bridge -- a window ends at its last attacking frame, so trailing
	// bridge samples are not part of it. Holding at most GAP_BRIDGE of them preserves
	// that without buffering the window itself.
	int    gap;
	double pendT[KTPAim::GAP_BRIDGE];
	double pendP[KTPAim::GAP_BRIDGE];
	int    pendCount;

	bool   open;

	void Reset()
	{
		n = 0; tFirst = tLast = 0.0;
		sT = sTT = sP = sPP = sTP = 0.0;
		gap = 0; pendCount = 0; open = false;
	}

	void Add(double t, double p)
	{
		if (n == 0) tFirst = t;
		n++;
		sT += t; sTT += t * t;
		sP += p; sPP += p * p;
		sTP += t * p;
	}

	bool Finish(KTPWindowStat *out) const
	{
		double dur = tLast - tFirst;
		if (n < KTPAim::MIN_FRAMES || dur < KTPAim::MIN_DUR) return false;

		double dn  = (double)n;
		double sxx = sTT - (sT * sT) / dn;
		if (sxx <= 0.0) return false;             // no time spread; the fit is undefined

		double sxy = sTP - (sT * sP) / dn;
		double syy = sPP - (sP * sP) / dn;

		// Clamped at zero because catastrophic cancellation can drive the residual
		// slightly negative on a near-perfect fit -- exactly the case worth reporting,
		// so a NaN here would drop the most interesting windows.
		double ssRes = syy - (sxy * sxy) / sxx;
		if (ssRes < 0.0) ssRes = 0.0;

		out->dur   = dur;
		out->slope = sxy / sxx;
		out->rms   = sqrt(ssRes / dn);
		out->n     = n;
		return true;
	}
};

struct KTPAimStats
{
	KTPFireWindow cur;

	int           windowsScored;               // windows meeting the recording bounds
	KTPWindowStat kept[KTPAim::KEEP_WINDOWS];  // the KEEP_WINDOWS lowest-residual ones
	int           keptCount;

	// Movement. Ground contact is counted in usercmds rather than seconds so it does
	// not move with tickrate; what it means is not decided here.
	int groundTouches;
	int shortGroundContacts;      // landings left within 2 usercmds
	int consecutiveShort;
	int maxConsecutiveShort;
	int usercmdsOnGround;

	int maxConsecutiveShortContacts() const { return maxConsecutiveShort; }

	void Reset()
	{
		cur.Reset();
		windowsScored = 0;
		keptCount = 0;
		groundTouches = shortGroundContacts = 0;
		consecutiveShort = maxConsecutiveShort = 0;
		usercmdsOnGround = 0;
	}

	void CloseWindow()
	{
		if (!cur.open) { cur.Reset(); return; }

		KTPWindowStat w;
		if (cur.Finish(&w))
		{
			windowsScored++;
			Keep(w);
		}
		cur.Reset();
	}

	// Insertion into a fixed slot set, smallest residual retained. Linear over
	// KEEP_WINDOWS and only on window close, so it never touches the per-usercmd path.
	void Keep(const KTPWindowStat &w)
	{
		if (keptCount < KTPAim::KEEP_WINDOWS)
		{
			kept[keptCount++] = w;
			return;
		}
		int worst = 0;
		for (int i = 1; i < keptCount; i++)
			if (kept[i].rms > kept[worst].rms) worst = i;
		if (w.rms < kept[worst].rms) kept[worst] = w;
	}
};

#endif // KTP_AIM_ACCUM_H
