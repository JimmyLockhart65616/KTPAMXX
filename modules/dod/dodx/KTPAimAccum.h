// KTP: per-usercmd aim/movement sampling, fed from SV_PlayerRunPreThink.
//
// THIS IS A SENSOR, NOT A DETECTOR. It measures the geometry of sustained-fire aim
// motion and reports it. It holds no detection threshold and reaches no conclusion --
// whatever consumes these numbers decides what they mean. Keep it that way: this
// repository is public, so a bound written here is a bound a reader can sit outside.
//
// The bounds that remain are STRUCTURAL, not selective: a least-squares line needs at
// least three points to have a residual at all. They are deliberately far below
// anything a consumer would gate on, so they cannot function as an invisibility floor.
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
	const int    GAP_BRIDGE = 2;      // non-attack usercmds tolerated inside one burst
	const int    MIN_FRAMES = 3;      // algebraic floor: below this a residual is undefined
	const double MIN_DUR    = 0.05;   // seconds; guards a zero time-span fit, nothing more

	// Windows retained per player between flushes. Retained by LONGEST DURATION, not by
	// smallest residual: duration carries no detection meaning, so it cannot be gamed
	// toward, and it avoids two real biases. Selecting the minimum residual would make
	// the retained set an extreme-value sample -- systematically lower for a player who
	// fires more -- and would let any smooth non-combat pan outcompete real bursts for
	// every slot.
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

	// Time is accumulated RELATIVE to the window's first sample. gpGlobals->time at
	// PreThink is a float cast of svtimebase, so its absolute magnitude grows with map
	// uptime and its ULP grows with it; keeping the origin at zero holds the sums in
	// the range where that cast is still exact.
	void Add(double t, double p)
	{
		if (n == 0) { tFirst = t; }
		double rel = t - tFirst;
		n++;
		sT += rel; sTT += rel * rel;
		sP += p;   sPP += p * p;
		sTP += rel * p;
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
		// Divisor is n, not n-2: this is a reported measurement rather than an unbiased
		// variance estimate, and n ships alongside so a consumer can re-derive either.
		out->rms   = sqrt(ssRes / dn);
		out->n     = n;
		return true;
	}
};

struct KTPAimStats
{
	KTPFireWindow cur;

	int           windowsScored;               // windows meeting the structural bounds
	KTPWindowStat kept[KTPAim::KEEP_WINDOWS];  // the KEEP_WINDOWS longest ones
	int           keptCount;

	// Ground contact, measured in MILLISECONDS. It used to be counted in usercmds,
	// which silently made it a function of the client's own cl_cmdrate -- two commands
	// is 20ms at rate 100 and 4ms at rate 500, so a fully cvar-compliant player could
	// move the signal by changing a legal setting. Time is the same for everyone.
	int    groundTouches;
	int    shortestGroundMs;      // -1 until a landing has been left
	double groundEnterTime;
	bool   onGroundPrev;

	void Reset()
	{
		cur.Reset();
		ResetCounters();
	}

	// Clears what a flush has just shipped WITHOUT discarding the window still in
	// progress. The read natives exclude the open window precisely because it will be
	// reported once it closes; wiping it here would make that promise false and lose
	// every burst that straddles a flush.
	void ResetCounters()
	{
		windowsScored = 0;
		keptCount = 0;
		groundTouches = 0;
		shortestGroundMs = -1;
		groundEnterTime = 0.0;
		onGroundPrev = false;
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

	// Insertion into a fixed slot set, longest retained. Linear over KEEP_WINDOWS and
	// only on window close, so it never touches the per-usercmd path.
	void Keep(const KTPWindowStat &w)
	{
		if (keptCount < KTPAim::KEEP_WINDOWS)
		{
			kept[keptCount++] = w;
			return;
		}
		int shortest = 0;
		for (int i = 1; i < keptCount; i++)
			if (kept[i].dur < kept[shortest].dur) shortest = i;
		if (w.dur > kept[shortest].dur) kept[shortest] = w;
	}
};

#endif // KTP_AIM_ACCUM_H
