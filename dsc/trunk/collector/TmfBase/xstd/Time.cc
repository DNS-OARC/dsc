 /* Made at The Measurement Factory     ::     measurement-factory.com */

#include "xstd/xstd.h"

#include <climits>
#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "xstd/Assert.h"
#include "xstd/Time.h"


// define xtimezone to be whatever ./configure told us
#if defined(HAVE_TIMEZONE)
        inline time_t xtimezone() { return HAVE_TIMEZONE; }
#else
        inline time_t xtimezone() { return 0; }
#endif

#if !defined(HAVE_GETTIMEOFDAY) && defined(HAVE__FTIME)
#include <sys/timeb.h>
static
void gettimeofday(Time *tm, void *) {
	struct _timeb tb;
	_ftime(&tb);
	tm->tv_sec  = tb.time;
	tm->tv_usec = tb.millitm;
}
#endif

#ifndef HAVE_TIMEGM
	static
	time_t fake_timegm(struct tm *t) {
		const time_t res = mktime(t);

#	if defined(HAVE_TM_GMTOFF)
		const struct tm *local = localtime(&res);
		return res + local->tm_gmtoff;
#	elif defined(HAVE_TIMEZONE)
		const time_t dst = t->tm_isdst > 0 ? -3600 : 0;
		return res - (xtimezone() + dst);
#	else
		return res;
#	endif
		}
#endif

time_t xtimegm(struct tm *t) {
#	ifdef HAVE_TIMEGM
		return timegm(t);
#	else
		return fake_timegm(t);
#	endif
}


Time Time::Now() {
	Time tm;
	gettimeofday(&tm, 0);
	return tm;
}

Time Time::Max() {
	return Time(INT_MAX, INT_MAX);
}

Time Time::Msec(long msec) {
	return Time(msec / 1000, 1000*(msec % 1000));
}

Time Time::Secd(double dsec) {
	Assert(dsec <= LONG_MAX && dsec >= LONG_MIN);
	const long sec = (long)dsec;
	return Time(sec, (long)((dsec-sec)*1e6));
}

Time::Time(const struct tm &t) {
	struct tm t2 = t;    // must copy -- timegm modifies its param
	if (t2.tm_year < 70) // a Y2K plug-in!
		t2.tm_year += 100;
	tv_sec = (0 <= t2.tm_mon && t2.tm_mon < 12) ? xtimegm(&t2) : -1;
	tv_usec = tv_sec >= 0 ? 0 : -1;
}

Time &Time::operator +=(const Time &tm) {

	tv_sec += tm.tv_sec;
	tv_usec += tm.tv_usec;

	if (tv_usec >= 1000000L) {
		tv_usec -= 1000000L;
		tv_sec++;
	}

	return *this;
}

// note: negative times are confusing for humans; e.g., "-1:1" means "-0.1"
Time &Time::operator -=(const Time &tm) {

	tv_sec -= tm.tv_sec;
	tv_usec -= tm.tv_usec;

	if (tv_usec < 0) {
		tv_usec += 1000000L;
		tv_sec--;
	}

	return *this;
}

Time &Time::operator *=(int factor) {
	tv_sec *= factor;
	tv_usec *= factor;

	if (tv_usec >= 1000000L) {
		tv_sec += tv_usec/1000000L;
		tv_usec %= 1000000L;
	} else
	if (tv_usec < 0) {
		tv_usec = -tv_usec;
		tv_sec -= tv_usec/1000000L;
		tv_sec--;
		tv_usec = 1000000L - (tv_usec % 1000000L);
	}

	return *this;
}

Time &Time::operator /=(double factor) {
	if (factor) {
		const double dsec = secd()/factor;
		tv_sec = (long) dsec;
		tv_usec = (long)((dsec-tv_sec)*1e6);
	} else {
		tv_sec = tv_usec = -1;
	}

	return *this;
}

struct tm *Time::gmtime() const {
	time_t clock = (time_t) tv_sec;
	return ::gmtime(&clock);
}

ostream &Time::print(ostream &os) const {
	if (tv_sec < 60*60*24*365 && tv_usec >= 0)
		return printInterval(os);
	if (tv_sec == -1 && tv_usec == -1)
		return os << "<none>"; // works for everybody?

	const char osfill = os.fill();
	return os << tv_sec << '.' << setw(6) << setfill('0') << tv_usec << setfill(osfill);
}

ostream &Time::printInterval(ostream &os) const {

	const int osprec = os.precision(2);
	const char osfill = os.fill();

	if (tv_sec < 0)
		os << tv_sec << '.' << setfill('0') << setw(6) << tv_usec << "sec";
	else
	if (tv_sec == 0)
		if (tv_usec < 1000)
			os << tv_usec << "usec";
		else
			os << (tv_usec/1000) << "msec";
	else
	if (tv_sec < 60)
		os << secd() << "sec";
	else
	if (tv_sec < 60*60)
		os << secd()/60 << "min";
	else
	if (tv_sec < 24*60*60)
		os << secd()/(60*60) << "hour";
	else
	if (tv_sec < 365*24*60*60)
		os << secd()/(24*60*60) << "day";
	else
		os << secd()/(365*24*60*60) << "year";

	os.fill(osfill);
	os.precision(osprec);

	return os;
}

// useful for debugging
ostream &operator <<(ostream &os, const struct tm &t) {
	return os << here << endl
		<< '\t' << "tm_mday: " << "\t " << setw(4) << t.tm_mday << endl
		<< '\t' << "tm_mon:  " << "\t " << setw(4) << t.tm_mon << endl
		<< '\t' << "tm_year: " << "\t " << setw(4) << t.tm_year << endl
		<< '\t' << "tm_hour: " << "\t " << setw(4) << t.tm_hour << endl
		<< '\t' << "tm_min:  " << "\t " << setw(4) << t.tm_min << endl
		<< '\t' << "tm_sec:  " << "\t " << setw(4) << t.tm_sec << endl
		<< '\t' << "tv_sec:  " << "\t " << setw(4) << Time(t) << endl
		<< endl;
}
