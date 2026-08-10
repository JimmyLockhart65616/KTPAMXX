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
	// Burst bridge, in MILLISECONDS. It used to be a usercmd count, which quietly made
	// "one continuous burst" a function of the client's own cl_cmdrate -- two commands
	// is 20ms at rate 100 and 4ms at 500, so identical sprays split differently per
	// player, and a script could chop its own windows by releasing on a chosen cadence.
	const double BRIDGE_MS  = 20.0;
	// Sample slots held while bridging. Bounded by the highest permitted cl_cmdrate
	// (1000) over BRIDGE_MS, plus margin; a bridge longer than this closes the window.
	const int    GAP_SLOTS  = 32;
	const int    MIN_FRAMES = 3;      // algebraic floor: below this a residual is undefined
	const double MIN_DUR    = 0.05;   // seconds; guards a zero time-span fit, nothing more

	// Windows retained per player between flushes. Retained by LONGEST DURATION, not by
	// smallest residual: duration carries no detection meaning, so it cannot be gamed
	// toward, and it avoids two real biases. Selecting the minimum residual would make
	// the retained set an extreme-value sample -- systematically lower for a player who
	// fires more -- and would let any smooth non-combat pan outcompete real bursts for
	// every slot.
	//
	// 16 rather than 8 so a consumer can compute a PROPORTION over the retained set
	// instead of reading an extreme -- the property that makes ClickCadenceAnalyzer the
	// model the other detectors should follow. It does not remove the bias: this is a
	// top-k sample at any k, and k cannot approach windowsScored at a sane payload size.
	// windowsScored ships alongside precisely so the sampling fraction stays recoverable.
	// ⚠️ Raising this raises the payload linearly (~34 bytes per window per player) and
	// the consumer's buffer must be re-derived from ITS loop bound, or truncation returns.
	const int    KEEP_WINDOWS = 16;
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
	double pendT[KTPAim::GAP_SLOTS];
	double pendP[KTPAim::GAP_SLOTS];
	int    pendCount;

	bool   open;

	void Reset()
	{
		n = 0; tFirst = tLast = 0.0;
		sT = sTT = sP = sPP = sTP = 0.0;
		pendCount = 0; open = false;
	}

	// Time is accumulated RELATIVE to the window's first sample. This improves the
	// CONDITIONING of the sums only -- it does not recover precision. gpGlobals->time
	// arrives already quantised: the engine casts svtimebase to float upstream
	// (rehlds sv_user.cpp), so the mantissa bits lost at high map uptime are gone before
	// this function sees them, and no arithmetic here restores them. A consumer that
	// cares about small residuals at long map uptime still has to account for it.
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

	// Ground contact, measured in MILLISECONDS -- a usercmd count would be a function of
	// the client's own cl_cmdrate (20ms at rate 100, 4ms at 500), so a legal cvar change
	// would move the signal. Time is the same for everyone.
	int    groundTouches;
	int    shortestGroundMs;      // -1 until a full contact has been observed
	// In-flight tracking. `groundKnown` distinguishes "was airborne" from "have not
	// looked yet": conflating them made every counter reset and every respawn fabricate
	// a landing, and re-anchoring groundEnterTime turned a 25-second stand on a flag
	// into a 176ms contact. `contactTimed` is false for a contact whose START we did not
	// witness, so its duration is never reported rather than reported wrong.
	double groundEnterTime;
	bool   onGroundPrev;
	bool   groundKnown;
	bool   contactTimed;

	void Reset()
	{
		cur.Reset();
		ResetCounters();
		groundEnterTime = 0.0;
		onGroundPrev = false;
		ForgetGroundState();
	}

	// Clears only what a flush has just SHIPPED. In-flight tracking -- the open window
	// and the ground contact in progress -- is left alone for the same reason: a burst
	// or a contact that straddles a flush is reported once, when it ends, with its true
	// extent. Clearing them here is what made a long contact read as a short one.
	void ResetCounters()
	{
		windowsScored = 0;
		keptCount = 0;
		groundTouches = 0;
		shortestGroundMs = -1;
	}

	// Ground tracking is unusable across a death: the player teleports, so the contact
	// in progress neither continues nor ended where we last saw it.
	void ForgetGroundState()
	{
		groundKnown = false;
		contactTimed = false;
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
		// >= not >: with coarse time quantisation equal durations are common, and a
		// strict > freezes the set on whichever arrived first.
		if (w.dur >= kept[shortest].dur) kept[shortest] = w;
	}
};

#endif // KTP_AIM_ACCUM_H
