/// __________________________________________________
/// transition.h
/// __________________________________________________

#ifndef TRANSITION_H
#define TRANSITION_H

// #define DEBUG

/// __________________________________________________
/// Class and Function declarations

// Development and debugging
#ifdef DEBUG

void _ctrsgn(const std::type_info&, bool = false);
class Demangler;
std::ostream& operator<< (std::ostream&, const Demangler&);

#endif // #ifdef DEBUG

// Utility
template<class T, class U> 
inline std::vector<U> get_vec_attr(const T&, const char*);
template<class T>
inline std::string str_tolower(std::string);
int colpos(const DataFrame, const char*);
template<class T>
std::vector<T> get_unique(const std::vector<T>);
inline int adjust(int, int, int);

/// Class Transitiondata

class Transitiondata {
	DataFrame df;
	const IntegerVector id;
	const DateVector testdate;
	const IntegerVector testresult;
	int nrows = df.nrows();
	template<typename T>
	T typechecker(int, int);

public:
	explicit Transitiondata(DataFrame _df, int idcol, int datecol, int testcol) :
		df(_df), id(typechecker<IntegerVector>(idcol, 1)), testdate(typechecker<DateVector>(datecol, 2)), testresult(typechecker<IntegerVector>(testcol, 3))
		{
//			std::cout << "§Transitiondata::Transitiondata(const DataFrame, int, int, int) "; _ctrsgn(typeid(*this));
		}

	~Transitiondata() = default;
//	~Transitiondata() { std::cout << "§Transitiondata::~Transitiondata() "; _ctrsgn(typeid(*this), true); }

	std::vector<int> unique_sub() const { return get_unique(as<std::vector<int>>(id)); }
	DateVector unique_date() const { return wrap(get_unique(as<std::vector<double>>(testdate))); }
	std::vector<int> unique_test() const { return get_unique(as<std::vector<int>>(testresult)); }

	int get_result(int, double) const;
	std::vector<double> get_id_dates(int) const;
	double get_prevdate(int, double) const;
	std::vector<double> prev_date() const;
	std::vector<int> prev_result() const;
	DataFrame add_transition(const char* colname, int, int);
	std::vector<int> get_transition(int, int) const;
};


// Auxilliary
inline IntegerVector prevres_intvec(DataFrame, const char*, const char*, const char*);


// Exported
DataFrame add_transitions(DataFrame object, const char* subject, const char* timepoint, const char* result, const char* transition, int cap, int modulator); 
IntegerVector get_transitions(DataFrame object, const char* subject, const char* timepoint, const char* result, int cap, int modulator); 
DataFrame add_prev_date(DataFrame object, const char* subject, const char* timepoint, const char* result, const char* prev_date);
DateVector get_prev_date(DataFrame object, const char* subject, const char* timepoint, const char* result);
DataFrame add_prev_result(DataFrame object, const char* subject, const char* timepoint, const char* result, const char* prev_result);
IntegerVector get_prev_result(DataFrame object, const char* subject, const char* timepoint, const char* result);
List uniques(DataFrame object, const char* subject, const char* timepoint, const char* result);

#endif  // TRANSITION_H
