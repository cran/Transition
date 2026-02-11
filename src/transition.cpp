
// [[Rcpp::plugins(cpp23)]]

#include <Rcpp.h>
#include <cxxabi.h>
using namespace Rcpp;

#include "transition.h"

using std::cout;
using std::endl;
using std::ostream;
using std::string;
using std::vector;


/// __________________________________________________
/// __________________________________________________
/// Development and Debugging functions
#ifdef DEBUG

/// Report object construction and destruction
void _ctrsgn(const std::type_info& obj, bool destruct)
{
	cout << (destruct ? "Destroying " : "Constructing ") << std::flush;
	string s { obj.name() };
	system(("c++filt -t " + s).data());
}

/// Demangle object names functor
class Demangler {
	char* p;
	int status { 0 };
public:
	Demangler(const std::type_info& obj) : p(abi::__cxa_demangle(obj.name(), NULL, NULL, &status)) {}
	~Demangler() { std::free(p); }
	operator string() const { return string("\"") + p + "\" (status " + std::to_string(status) + ")"; }
};

ostream& operator<< (ostream& stream, const Demangler& d)
{
//  cout << "ostream& operator<< (ostream&, const Demangler&) ";
  return stream << string(d);
}

#endif // #ifdef DEBUG

/// __________________________________________________
/// Utility

/// __________________________________________________
/// Return named attribute as vector<U> or empty vector<U>
template<class T, class U>
inline vector<U> get_vec_attr(const T& t, const char* attrname)
{
//	cout << "@get_vec_attr<T, U>(const T&, const char*) attr \"" << attrname << "\" " << std::boolalpha << t.hasAttribute(attrname) << endl;
	static_assert(std::is_same<NumericVector, T>::value || std::is_same<DataFrame, T>::value, "T must be NumericVector or DataFrame");
	return t.hasAttribute(attrname) ? as<vector<U>>(t.attr(attrname)) : vector<U>();
}


/// __________________________________________________
/// string to lower case (see cppreference.com std::tolower)
inline string str_tolower(string s)
{
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return tolower(c); });
    return s;
}


/// __________________________________________________
/// Find data frame column with a specified name
int colpos(const DataFrame object, const char* colname)
{
//	cout << "@colpos(const DataFrame, const char*) name is " << colname << endl;
	if (!object.containsElementNamed(colname))
		stop("No column named \"%s\" in data frame", colname);
	return object.offset(colname);
}


// Find unique values in vector
template<class T>
vector<T> get_unique(const vector<T> vec)
{
//	cout << "@get_unique<T>(const vector<T>) vec " << Demangler(typeid(vec)) << endl;
	vector<T> out { vec };
	std::sort(out.begin(), out.end());
	auto last { std::unique(out.begin(), out.end()) };
	out.erase(last, out.end());
	return out;
}

// Adjust the difference,symmetrically
inline int adjust(int diff, int cap, int modulate)
{
//	cout << "@adjust(int) diff " << diff << endl;
	bool neg = std::signbit(diff);
	diff = abs(diff);
	if (modulate > 1)
		diff = (diff + modulate - 1) / modulate;
	if (bool(cap))
		diff = (diff < cap) ? diff : cap;
	return neg ? diff *= -1 : diff;
}


/// __________________________________________________
/// Class Transitiondata

// Ctor auxilliary function
template<typename T>
T Transitiondata::typechecker(int colno, int arg)
{	
//	cout << "@Transitiondata::typechecker<T>(int, int) colno " << colno << "; arg " << arg << endl;
	std::string errstr("column `");
	errstr += vector<string>(df.names())[colno] + "`";
	std::string wrnstr(errstr);
	RObject colobj { df[colno] };
	bool good = is<T>(df[colno]);
	bool warn = false;
	switch (arg) {
		case 0:
			if (!good) errstr += " not of class data.frame";
			break;

		case 1:
			if (!good) errstr += " not an integer or factor";
			break;

		case 2:
			if (!good) {
				if (colobj.inherits("Date")) {
					if (!is<NumericVector>(colobj)) {
						if (is<IntegerVector>(colobj)) {
							wrnstr += ": Date converted from integer to numeric";
							warn = true;
							df[colno] = as<NumericVector>(df[colno]);
							good = true; 
						}
						errstr += " not of class Date, type numeric";
					}
				} else errstr += " not of class Date";
			}
			break;

		case 3:
			if (!(good && colobj.inherits("factor") && colobj.inherits("ordered"))) {
				if (is<NumericVector>(colobj)) {
					wrnstr += ": type converted from numeric to integer";
					warn = true;
					df[colno] = as<IntegerVector>(df[colno]);
					good = true;
				}
				if (good) {
					const vector<int>& v = as<vector<int>>(colobj);   
					auto minmax = std::minmax_element(v.begin(), v.end());
					good = !(0 > *minmax.first || 1 < *minmax.second);
				}
			}
			errstr += " neither an ordered factor nor an integer vector with all values either 0 or 1";
			break;

			default:
				stop("Transitiondata::typechecker<T>(int, int) my bad");
	}
	if (!good)
		throw std::invalid_argument(errstr);
	if (warn)
		warning(wrnstr);
	return df[colno];
}


// Test result for given subject and date
int Transitiondata::get_result(int subject, double date) const
{
//	cout << "@Transitiondata::get_result(int, double) const subject = " << subject << "; date = " << Date(date).getYear() << endl;
	for (int x { 0 }; x < nrows; ++x)
		if (id[x] == subject && testdate[x] == date) {
			return testresult[x];
		}
	return NA_INTEGER;
}

// All dates for a given subject, sorted
std::vector<double> Transitiondata::get_id_dates(int target) const
{
//	cout << "@Transitiondata::get_id_dates(int id) const target = " << target << endl;
	vector<double> out;
	for (int x { 0 }; x < nrows; ++x) {
		if (id[x] == target)
			out.push_back(testdate[x]);
	}
	std::sort(out.begin(), out.end());
	return out;
}

// most recent previous date for subject
double Transitiondata::get_prevdate(int subject, double date) const
{
//	cout << "@Transitiondata::get_prevdate(int, double) const subject = " << subject << "; date = " << Date(date).getYear() << endl;
	auto dates { get_id_dates(subject) };
	auto it { find(dates.begin(), dates.end(), date) };
	if (it == dates.end())
		stop("testdate %s not found for subject %i.", Date(date).format("%Y"), subject);
	return (it == dates.begin()) ? NA_REAL : *std::prev(it);
}

// vector of the most recent previous date by subject
vector<double> Transitiondata::prev_date() const
{
//	cout << "@Transitiondata::prev_date() const\n";
	vector<double> previous(nrows);
	transform(id.begin(), id.end(), testdate.begin(), previous.begin(), [this](int id, double date){ return get_prevdate(id, date); });
	return previous;
}

// vector of the most recent previous result by subject
vector<int> Transitiondata::prev_result() const
{
//	cout << "@Transitiondata::prev_result() const\n";	
	auto prevdate { prev_date() };
	vector<int> prevres(nrows);
	transform(id.begin(), id.end(), prevdate.begin(), prevres.begin(), [this](int id, double date){ return get_result(id, date); });
	return prevres;
}


// Add transitions column to data frame
DataFrame Transitiondata::add_transition(const char* colname, int cap, int modulate)
{
//	cout << "@Transitiondata::add_transition(int)\n";
	if (df.containsElementNamed(colname))
		stop("Data frame already has column named \"%s\", try another name", colname);
 	df.push_back(get_transition(cap, modulate), colname);
	return df;
}


// Return transitions vector
vector<int> Transitiondata::get_transition(int cap, int modulate) const
{
//	cout << "@Transitiondata::get_transition(int) cap = " << cap << "; modulate = " << modulate << endl;
	if (cap < 0)
		throw std::invalid_argument("\"cap\" less than zero");
	if (modulate < 0)
		throw std::invalid_argument("\"modulate\" less than zero");
	auto previous { prev_result() };
	std::vector<int> transitions(nrows);
	transform(previous.begin(), previous.end(), testresult.begin(), transitions.begin(),
		[cap, modulate](int prev, int curr) { return (NA_INTEGER == prev) ? NA_INTEGER : adjust(curr - prev, cap, modulate); }
	);
	return transitions;
}


/// __________________________________________________
/// Auxilliary
inline IntegerVector prevres_intvec(DataFrame object, const char* subject, const char* timepoint, const char* result)
{
//	cout << "@prevres_intvec(DataFrame, const char*, const char*, const char*) subject " << subject
//		 << "; timepoint " << timepoint << "; result " << result << endl;
	int testcol { colpos(object, result) };
    RObject colobj { object[testcol] };
	IntegerVector intvec(wrap(vector<int>(Transitiondata(object, colpos(object, subject), colpos(object, timepoint), testcol).prev_result())));
	if (colobj.inherits("factor") && colobj.inherits("ordered"))
		intvec.attr("class") = CharacterVector::create("factor", "ordered");
	intvec.attr("levels") = colobj.attr("levels");
	return intvec;
}


/// __________________________________________________
/// Exported

//' @title
//' Identify Temporal Transitions in Longitudinal Study Data
//'
//' @name
//' Transitions
//'
//' @description
//' \code{get_transitions()} identifies temporal transitions in test results for individual
//' subjects in a longitudinal study.
//'
//' \code{add_transitions()} interpolates these transitions into a data frame for further analysis.
//'
//' @details
//' The data can be presented in any order e.g., ordered by \code{subject}, by \code{timepoint},
//' forwards or backwards in time, or entirely at random, and may have unbalanced designs with different
//' time points or numbers of test results per subject. However, the \emph{user} is responsible for
//' ensuring the data contain unique combinations of \code{subject}, \code{timepoint} and \code{result};
//' if not, outputs will be undefined.
//'
//' Time points should be formatted as \code{\link{Dates}} and included in data frame \code{object} in
//' the column named as specified by argument \code{timepoint} (see \emph{Note}).
//'
//' Test results should either be semi-quantitiative, formatted as an
//' \code{\link[base:ordered]{ordered factor}} (see \emph{Note}), or binary data formatted as an
//' \code{\link{integer}} (or \code{\link{numeric}}) vector with values of either \code{1} or \code{0},
//' and included in \code{object} in the data frame column specified by argument \code{result}.
//'
//' Temporal transitions in the test \code{results} for each \code{subject} within the \code{object}
//' \code{\link{data.frame}} are characterised using methods governed by options \code{cap} and
//' \code{modulate}. If these two parameters are both zero (their defaults), a simple arithmetic
//' difference between the levels of the present and previous result is calculated. Otherwise, if
//' the value of \code{modulate} is a positive, non-zero integer, the arithmetic difference is
//' subjected to integer division by that value. Finally, if \code{cap} is a positive, non-zero
//' integer, the (possibly modulated) absolute arithmetic difference is capped at that value.
//'
//' @family transitions
//' @seealso
//' \code{\link{data.frame}}, \code{\link{Dates}}, and \code{\link[base:factor]{ordered factor}}.
//'
//' @param object a \code{\link{data.frame}} (or object coercible by \code{\link{as.data.frame}()} to
//'   a data frame) containing the data to be analysed.
//'
//' @param subject \code{\link{character}}, name of the column (of type \code{\link{integer}} or
//'   \code{\link{factor}}) identifying individual study subjects; default \code{"subject"}.
//'
//' @param timepoint \code{character}, name of the column recording time points (as \code{\link{Dates}})
//'   of testing of subjects; default \code{"timepoint"}.
//'
//' @param result \code{character}, name of the column (of type \code{\link[base:factor]{ordered factor}},
//'   or binary, see \emph{Details}) recording test results; default \code{"result"}.
//'
//' @param transition \code{character}, name to be used for a new column (of type
//'   \code{\link{integer}}) to record transitions; default \code{"transition"}.
//'
//' @param cap \code{\link{integer}}, required for calculating transitions; default \code{0L}.
//'
//' @param modulate \code{\link{integer}}, required for calculating transitions; default \code{0L}.
//'
//' @return
//'
//' \item{\code{add_transitions()}}{A \code{\link{data.frame}} based on \code{object}, with an added
//'    column of type \code{\link{integer}} containing the values of the test result transitions.}
//'
//' \item{\code{get_transitions()}}{An \code{\link[base:vector]{integer vector}} of length
//'    \code{\link{nrow}(object)}, containing the values of the test result transitions ordered in the exact
//'    sequence of the \code{subject} and \code{timepoint} in \code{object}.}
//'
//' @note
//' Time points represented by \code{\link{integer}} or \code{\link{numeric}} values can be converted
//'   to R \code{Dates} conveniently using \code{\link{as.Date}()}. If only \emph{year} information is
//'   available, arbitrary values could be used consistently for month and day e.g., 1st of January of
//'   each year; likewise, the first day of each month could be used arbitrary, if only the
//'   \emph{year} and \emph{month} were known. See vignette 
//'   \href{../doc/convertDate.pdf}{Converting numeric values to class \code{"Date"}} for examples.
//'
//' Quantitive results available as \code{\link{numeric}} data can be converted to a semi-quantitative
//'   \code{\link[base:factor]{ordered factor}} conveniently using \code{\link{cut}()} (see \emph{examples}).
//'
//' @examples
//'
//'   # Inspect Blackmore data frame using {base} str()
//' Blackmore |> str()
//'
//'   # {base} hist() gives insights into the "exercise" column,
//'   #   useful for choosing `breaks` and `labels` in cut() below
//' hist(Blackmore$exercise, include.lowest = TRUE, plot = FALSE)[1:2]
//'
//'   # Tweak Blackmore data frame by converting "age" to dates for the argument
//'   #   timepoint (using an arbitrary "origin" of 1-Jan-2000), and converting
//'   #   "exercise" to an ordered factor "result" with {base} cut()
//' Blackmore <- transform(Blackmore,
//'     timepoint = as.Date("2000-01-01") + round(age * 365.25),
//'     result = cut(
//'         exercise,
//'         breaks = seq(0, 30, 2),
//'         labels = paste0("<=", seq(0, 30, 2)[-1]),
//'         include.lowest = TRUE,
//'         ordered_result = TRUE
//'     )
//' )
//'
//'   # subject, timepoint and result arguments now defaults and required types
//' Blackmore |> str()
//'
//'   # Integer vector of test result transitions (defaults: cap = modulate = 0)
//' get_transitions(Blackmore)
//'
//'   # Tabulate values of transitions
//' get_transitions(Blackmore) |> table()
//'
//'   # Effect of cap argument
//' get_transitions(Blackmore, cap = 6) |> table()
//'
//'   # Effect of modulate argument
//' get_transitions(Blackmore, modulate = 2) |> table()
//'
//'   # Add column of test result transitions to data frame
//' add_transitions(Blackmore) |> head(22)
//'
//'   # Showing transitions as either positive (1) or negative (-1)
//'   #   (defaults to modulate = 0)
//' add_transitions(Blackmore, cap = 1) |> head(14)
//'
//' rm(Blackmore)
//'
// [[Rcpp::export]]
DataFrame add_transitions(
	DataFrame object,
	const char* subject = "subject",
	const char* timepoint = "timepoint",
	const char* result = "result",
	const char* transition = "transition",
	int cap = 0,
	int modulate = 0)
{
//	cout << "——Rcpp::export——add_transitions(DataFrame, const char*, const char*, const char*, const char*, int) subject " << subject
//		 << "; timepoint " << timepoint << "; result " << result << "; transition " << transition << endl;
	try {
		return Transitiondata(object, colpos(object, subject), colpos(object, timepoint), colpos(object, result)).add_transition(transition, cap, modulate);
	} catch (exception& e) {
		Rcerr << "Error in add_transitions(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
		Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return DataFrame::create();
}


//' @rdname Transitions
// [[Rcpp::export]]
IntegerVector get_transitions(
	DataFrame object,
	const char* subject = "subject",
	const char* timepoint = "timepoint",
	const char* result = "result",
	int cap = 0,
	int modulate = 0)
{
//	cout << "——Rcpp::export——get_transitions(DataFrame, const char*, const char*, const char*, int) subject " << subject
//		 << "; timepoint " << timepoint << "; result " << result << endl;
	try {
		return wrap(Transitiondata(object, colpos(object, subject), colpos(object, timepoint), colpos(object, result)).get_transition(cap, modulate));
	} catch (exception& e) {
		Rcerr << "Error in get_transitions(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
		Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return IntegerVector();
}


//' @title
//' Find Previous Test Date for Subject
//'
//' @name
//' PreviousDate
//'
//' @description
//' \code{get_prev_date()} identifies the previous test date for individual subjects and timepoints
//' in a longitudinal study.
//'
//' \code{add_prev_date()} interpolates these previous test dates into a data frame for further analysis.
//'
//' @details
//' See \code{\link{Transitions}} \emph{details}.
//'
//' @family transitions
//' @seealso
//' \code{\link{data.frame}}, \code{\link{Dates}}, \code{\link[base:factor]{ordered factor}}.
//'
//' @param prev_date \code{character}, name to be used for a new column to record previous test dates;
//'   default \code{"prev_date"}.
//'
//' @inheritParams Transitions
//'
//' @return
//'
//' \item{\code{add_prev_date()}}{A \code{\link{data.frame}} based on \code{object}, with an added
//'    column named as specified by argument \code{prev_date} of class \code{\link{Date}} containing
//'    the values of the previous test dates.}
//'
//' \item{\code{get_prev_date()}}{A \code{vector} of class \code{\link{Date}}, length
//'    \code{\link{nrow}(object)}, containing the values of the previous test dates ordered in the exact
//'    sequence of the \code{subject} and \code{timepoint} in \code{object}.}
//'
//'
//' @examples
//'
//' \dontshow{
//' Blackmore <- transform(Blackmore, timepoint = as.Date("2000-01-01") + round(age * 365.25),
//'     result = cut(exercise, breaks = seq(0, 30, 2), labels = paste0("<=", seq(0, 30, 2)[-1]),
//'         include.lowest = TRUE, ordered_result = TRUE))
//' }
//'
//'  ## Continuing example from `add_transitions()`
//'   # subject, timepoint and result arguments all defaults and required types
//' Blackmore |> str()
//'
//'   # Integer vector of the previous test dates
//' get_prev_date(Blackmore)
//'
//'   # Add column of  previous test dates to data frame
//' add_prev_date(Blackmore) |> head(32)
//'
//' rm(Blackmore)
//'
// [[Rcpp::export]]
DataFrame add_prev_date(
	DataFrame object, const char* subject = "subject",
	const char* timepoint = "timepoint",
	const char* result = "result",
	const char* prev_date = "prev_date")
{
//	cout << "——Rcpp::export——add_prev_date(DataFrame, const char*, const char*, const char*) subject "
//	<< subject << "; timepoint " << timepoint << "; result " << result << "; prev_date " << prev_date << endl;
	try {
        	object.push_back(DateVector(wrap(Transitiondata(object, colpos(object, subject), colpos(object, timepoint), colpos(object, result)).prev_date())), prev_date);
 	return object;
	} catch (exception& e) {
        	Rcerr << "Error in add_prev_date(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
        	Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return DataFrame::create();
}


//' @rdname PreviousDate
// [[Rcpp::export]]
DateVector get_prev_date(DataFrame object, const char* subject = "subject", const char* timepoint = "timepoint", const char* result = "result")
{
//	cout << "——Rcpp::export——get_prev_date(DataFrame, const char*, const char*, const char*) subject " << subject << "; timepoint " << timepoint << "; result " << result << endl;
	try {
		return wrap(Transitiondata(object, colpos(object, subject), colpos(object, timepoint), colpos(object, result)).prev_date());
	} catch (exception& e) {
		Rcerr << "Error in get_prev_date(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
		Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return DateVector(0);
}


//' @title
//' Find Previous Test Result for Subject
//'
//' @name
//' PreviousResult
//'
//' @description
//' \code{get_prev_result()} identifies the previous test result for individual subjects and timepoints
//' in a longitudinal study.
//'
//' \code{add_prev_result()} interpolates these previous test results into a data frame for further analysis.
//'
//' @details
//' See \code{\link{Transitions}} \emph{details}.
//'
//' @family transitions
//' @seealso
//' \code{\link{data.frame}}, \code{\link{Dates}}, \code{\link[base:factor]{ordered factor}}.
//'
//' @param prev_result \code{character}, name to be used for a new column to record previous result;
//'   default \code{"prev_result"}.
//'
//' @inheritParams Transitions
//'
//' @return
//'
//' \item{\code{add_prev_result()}}{A \code{\link{data.frame}} based on \code{object}, with an added
//'    column named as specified by argument \code{prev_result} and of type
//'    \code{\link[base:factor]{ordered factor}} or \code{\link{integer}} depending on whether the
//'    results are semi-quantitiative or binary.}
//'
//' \item{\code{get_prev_result()}}{An \code{\link[base:factor]{ordered factor}} of length
//'    \code{\link{nrow}(object)}, containing the values of the previous test results ordered in the
//'    exact sequence of the \code{subject} and \code{timepoint} in \code{object}.}
//'
//' @examples
//'
//' \dontshow{
//' Blackmore <- transform(Blackmore, timepoint = as.Date("2000-01-01") + round(age * 365.25),
//'     result = cut(exercise, breaks = seq(0, 30, 2), labels = paste0("<=", seq(0, 30, 2)[-1]),
//'         include.lowest = TRUE, ordered_result = TRUE))
//' }
//'
//'  ## Continuing example from `add_transitions()`
//'   # subject, timepoint and result arguments all defaults and required types
//' Blackmore |> str()
//'
//'   # Previous test results as ordered factor
//' get_prev_result(Blackmore)
//'
//'   # Previous test result as column of data frame
//' (Blackmore <- add_prev_result(Blackmore)) |> head(32)
//'
//' rm(Blackmore)
//'
// [[Rcpp::export]]
DataFrame add_prev_result(
	DataFrame object,
	const char* subject = "subject",
	const char* timepoint = "timepoint",
	const char* result = "result",
	const char* prev_result = "prev_result"
)
{
//	cout << "——Rcpp::export——add_prev_result(DataFrame, const char*, const char*, const char*, const char*) subject "
//		 << subject << "; timepoint " << timepoint << "; result " << result << "; prev_result " << prev_result << endl;
	try {
		object.push_back(prevres_intvec(object, subject, timepoint, result), prev_result);
		return object;
	} catch (exception& e) {
		Rcerr << "Error in add_prev_result(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
		Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return DataFrame::create();
}


//' @rdname PreviousResult
// [[Rcpp::export]]
IntegerVector get_prev_result(DataFrame object, const char* subject = "subject", const char* timepoint = "timepoint", const char* result = "result")
{
//	cout << "——Rcpp::export——get_prev_result(DataFrame, const char*, const char*, const char*) subject " << subject << "; timepoint " << timepoint << "; result " << result << endl;
	try {
		return prevres_intvec(object, subject, timepoint, result);
	} catch (exception& e) {
		Rcerr << "Error in get_prev_result(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
		Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return IntegerVector();
}


//' @title
//' Unique Values for Subject, Timepoint and Result
//'
//' @name
//' uniques
//'
//' @description
//' \code{uniques()} identifies unique values for subjects, timepoints and test results in
//' longitudinal study data.
//'
//' @details
//' See \code{\link{Transitions}} \emph{details}.
//'
//' Works for \code{subject} as either an \code{\link[base:vector]{integer vector}} or
//' a \code{\link{factor}}.
//'
//' @family transitions
//' @seealso
//' \code{\link{data.frame}}, \code{\link{Dates}}, \code{\link[base:factor]{ordered factor}}.
//'
//' @inheritParams Transitions
//'
//' @return
//' A \code{\link{list}} of three elements
//'
//' \item{1.}{An \code{\link[base:vector]{integer vector}} or \code{\link{factor}} of unique subject
//'   identifications.}
//'
//' \item{2.}{A \code{\link{vector}} of class \code{\link{Date}} of unique timepoints in the study.}
//'
//' \item{3.}{An \code{\link[base:factor]{ordered factor}} of unique values for results of the study.}
//'
//' @examples
//'
//' \dontshow{
//' Blackmore <- transform(Blackmore, timepoint = as.Date("2000-01-01") + round(age * 365.25),
//'     result = cut(exercise, breaks = seq(0, 30, 2), labels = paste0("<=", seq(0, 30, 2)[-1]),
//'         include.lowest = TRUE, ordered_result = TRUE))
//' }
//'
//'  ## Continuing example from `add_transitions()`
//'   # subject, timepoint and result arguments all defaults and required types
//'   # (native subject is factor)
//' uniques(Blackmore)
//'   #
//' Blackmore <- transform(Blackmore, subject = as.integer(subject))
//'   # subject now as integer
//' Blackmore |> str()
//' uniques(Blackmore)
//'
//' rm(Blackmore)
//'
// [[Rcpp::export]]
List uniques(DataFrame object, const char* subject = "subject", const char* timepoint = "timepoint", const char* result = "result")
{
//	cout << "——Rcpp::export——uniques(DataFrame)\n";
	try {
		Transitiondata td(object, colpos(object, subject), colpos(object, timepoint), colpos(object, result));
		IntegerVector subvec(wrap(td.unique_sub()));
		RObject subcol{ object[colpos(object, subject)] };
		if (subcol.inherits("factor")) {
			subvec.attr("class") = string { "factor" };			
			subvec.attr("levels") = subcol.attr("levels");
		}
		IntegerVector rltvec(wrap(td.unique_test()));
		rltvec.attr("class") = CharacterVector::create("factor", "ordered");
		rltvec.attr("levels") = (RObject { object[colpos(object, result)] }).attr("levels");
		return List::create(_[subject] = subvec, _[timepoint] = td.unique_date(), _[result] = rltvec);
	} catch (exception& e) {
		Rcerr << "Error in uniques(): " << e.what() << '\n';
	} catch (std::invalid_argument& iva) {
		Rcerr << "Error invalid argument: " << iva.what() << '\n';
	}
	return List::create();
}
