### R code from vignette source 'convertDate.Rnw'

###################################################
### code chunk number 1: convertDate.Rnw:27-30
###################################################
library(Transition)
options(width = 80, continue = "  ",
        try.outFile = stdout())


###################################################
### code chunk number 2: convertDate.Rnw:59-64
###################################################
(df <- data.frame(
        subject = rep(1001:1003),
        timepoint = rep(2018:2025, each = 3),
        result = gl(3, 4, lab = c("good", "bad", "ugly"), ordered = TRUE)
    ))


###################################################
### code chunk number 3: convertDate.Rnw:73-77
###################################################
(df <- transform(
        df,
        timepoint = as.Date(paste(timepoint, "01", "01", sep = "-"))
    ))


###################################################
### code chunk number 4: convertDate.Rnw:84-85
###################################################
(df <- add_prev_result(df))


###################################################
### code chunk number 5: convertDate.Rnw:92-93
###################################################
transform(df, timepoint = format(timepoint, "%Y"))


###################################################
### code chunk number 6: convertDate.Rnw:103-109
###################################################
(df <- data.frame(
        subject = 1001:1002,
        year = rep(2024:2025, each = 12),
        month = rep(c(7:12, 1:6), each = 2),
        result = gl(2, 3, lab = c("low", "high"), ordered = TRUE)
    ))


###################################################
### code chunk number 7: convertDate.Rnw:117-123
###################################################
(df <- transform(
        df,
        timepoint = as.Date(paste(year, month, "01", sep = "-")),
        year = NULL,
        month = NULL
    ))


###################################################
### code chunk number 8: convertDate.Rnw:130-131
###################################################
(df <- add_transitions(df))


###################################################
### code chunk number 9: convertDate.Rnw:138-139
###################################################
transform(df, timepoint = format(timepoint, "%b-%Y"))


