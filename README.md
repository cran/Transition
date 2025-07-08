# Transition
### Characterise Transitions in Test Result Status in Longitudinal Studies

Analyse data from longitudinal studies to characterise changes in values of semi-quantitative
outcome variables within individual subjects, using high performance C++ code to enable rapid
processing of large datasets. A flexible methodology is available for codifying these state transitions.

## Installation

You can install the currently-released version from CRAN with this R
command:

``` r
install.packages("Transition")
```

Alternatively, you can install the latest development version of Transition
from [GitHub](https://github.com/) with:
      
``` r
# install.packages("devtools")
devtools::install_github("Mark-Eis/Transition")
```
---

**Authors:** Mark C. Eisler and Ana V. Rabaza

**eMail:** Mark.Eisler@bristol.ac.uk, arabaza@pasteur.edu.uy

**ORCID** = [0000-0001-6843-3345](https://orcid.org/0000-0001-6843-3345), 
[0000-0002-9713-0797](https://orcid.org/0000-0002-9713-0797)

### Transition Package Overview: –

* Identify temporal transitions in test results for individual subjects in a longitudinal study with
    `get_transitions()`.

* Interpolate these transitions into a data frame for further analysis with `add_transitions()`.

* Identify the previous test result for individual subjects and timepoints in a longitudinal study
    with `get_prev_result()`.

* Interpolate these previous test results into a data frame for further analysis with
    `add_prev_result()`.

* Identify the previous test date for individual subjects and timepoints in a longitudinal study
    `get_prev_date()`.

* Interpolate these previous test dates into a data frame for further analysis with `add_prev_date()`.

* Identify unique values for subjects, timepoints and test results in longitudinal study data with
    `uniques()`.

#### Methodology  

*Transition* uses high performance C++ code seamlessly integrated into R using
[`Rcpp`](https://www.rcpp.org) to enable rapid processing of large longitudinal
study datasets.

#### Disclaimer

While every effort is made to ensure this package functions as expected, the
authors accept no responsibility for the consequences of errors.