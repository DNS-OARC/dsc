 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_TIME__H
#define TMF_BASE__XSTD_TIME__H

#include <time.h>
#include <sys/time.h>

class ostream;

// a wrapper arround "struct timeval"
class Time: public timeval {
	public:
		static Time Now(); // current time (does system call)
		static Time Max(); // maximum time expressed with this type

		static Time Msec(long msec);
		static Time Sec(long sec) { return Time(sec, 0); }
		static Time Secd(double sec);
		static Time Mind(double min) { return Secd(min*60); }
		static Time Hourd(double hour) { return Mind(hour*60); }
		static Time Dayd(double day) { return Hourd(day*24); }
		static Time Yeard(double year) { return Dayd(year*365); }

	public:
		Time() { tv_sec = tv_usec = -1; }
		Time(long sec, long usec) { tv_sec = sec; tv_usec = usec; }
		Time(const timeval &tv): timeval(tv) {}
		Time(const struct tm &t);

		bool known() const { return (*this) >= 0; }

		bool operator ==(const Time &tm) const { return tv_sec == tm.tv_sec && tv_usec == tm.tv_usec; }
		bool operator !=(const Time &tm) const { return tv_sec != tm.tv_sec || tv_usec != tm.tv_usec; }
		bool operator  >(const Time &tm) const { return tv_sec > tm.tv_sec || (tv_sec == tm.tv_sec && tv_usec > tm.tv_usec); }
		bool operator >=(const Time &tm) const { return tv_sec > tm.tv_sec || (tv_sec == tm.tv_sec && tv_usec >= tm.tv_usec); }
		bool operator  <(const Time &tm) const { return tv_sec < tm.tv_sec || (tv_sec == tm.tv_sec && tv_usec < tm.tv_usec); }
		bool operator <=(const Time &tm) const { return tv_sec < tm.tv_sec || (tv_sec == tm.tv_sec && tv_usec <= tm.tv_usec); }

		// argument (void *) is ignored, '0' is assumed
		bool operator ==(void *) const { return !tv_sec && !tv_usec; }
		bool operator !=(void *) const { return tv_sec || tv_usec; }
		bool operator  >(void *) const { return tv_sec > 0 || (tv_sec == 0 && tv_usec  > 0); }
		bool operator >=(void *) const { return tv_sec > 0 || (tv_sec == 0 && tv_usec >= 0); }
		bool operator  <(void *) const { return tv_sec < 0 || (tv_sec == 0 && tv_usec  < 0); }
		bool operator <=(void *) const { return tv_sec < 0 || (tv_sec == 0 && tv_usec <= 0); }

		long sec() const { return tv_sec; }
		long msec() const { return tv_sec*(long)1e3 + tv_usec/(long)1e3; }
		long usec() const { return tv_sec*(long)1e6 + tv_usec; }
		double msecd() const { return tv_sec*1e3 + tv_usec/1e3; }
		double secd() const { return tv_sec*1e0 + tv_usec/1e6; }

		struct tm *gmtime() const;

		Time &operator +=(const Time &tm);
		Time &operator -=(const Time &tm);
		Time &operator *=(int factor);
		Time &operator /=(double factor);

		ostream &print(ostream &os) const;

	protected:
		ostream &printInterval(ostream &os) const;
};

inline
Time operator +(Time t1, Time t2) {
	return t1 += t2;
}

inline
Time operator -(Time t1, Time t2) {
	return t1 -= t2;
}

inline
Time operator *(Time t, double f) {
	return Time::Secd(t.secd() * f);
}

inline
Time operator *(Time t, int f) {
	return t *= f;
}

inline
Time operator /(Time t, double f) {
	return t /= f;
}

inline
double operator /(Time t1, Time t2) {
	return t2 == 0 ? -1.0 : (t1.secd()/t2.secd());
}

inline
Time operator +(Time t) {
	return t;
}

inline
Time operator -(Time t) {
	return Time(0,0) - t;
}

inline
ostream &operator <<(ostream &os, const Time &tm) {
	return tm.print(os);
}

extern ostream &operator <<(ostream &os, const struct tm &t);

#endif
