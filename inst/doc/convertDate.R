### R code from vignette source 'convertDate.Rnw'

###################################################
### code chunk number 1: convertDate.Rnw:27-30
###################################################
library(Transition)
options(width = 80, continue = "  ",
        try.outFile = stdout())


###################################################
### code chunk number 2: convertDate.Rnw:56-61
###################################################
(df <- data.frame(
        subject = rep(1001:1003),
        timepoint = rep(2018:2025, each = 3),
        result = gl(3, 4, lab = c("good", "bad", "ugly"), ordered = TRUE)
    ))


###################################################
### code chunk number 3: convertDate.Rnw:66-71
###################################################
(df <- data.frame(
        subject = rep(1001:1003),
        timepoint = rep(2018:2025, each = 3),
        result = gl(3, 4, lab = c("good", "bad", "ugly"), ordered = TRUE)
    ))


###################################################
### code chunk number 4: convertDate.Rnw:79-83
###################################################
(df <- transform(
        df,
        timepoint = as.Date(paste(timepoint, "01", "01", sep = "-"))
    ))


###################################################
### code chunk number 5: convertDate.Rnw:90-91
###################################################
(df <- add_prev_result(df))


###################################################
### code chunk number 6: convertDate.Rnw:98-99
###################################################
transform(df, timepoint = format(timepoint, "%Y"))


###################################################
### code chunk number 7: convertDate.Rnw:104-105
###################################################
transform(df, timepoint = format(timepoint, "%Y"))


###################################################
### code chunk number 8: convertDate.Rnw:114-120
###################################################
(df <- data.frame(
        subject = 1001:1002,
        year = rep(2024:2025, each = 12),
        month = rep(c(7:12, 1:6), each = 2),
        result = gl(2, 3, lab = c("low", "high"), ordered = TRUE)
    ))


###################################################
### code chunk number 9: convertDate.Rnw:127-133
###################################################
(df <- transform(
        df,
        timepoint = as.Date(paste(year, month, "01", sep = "-")),
        year = NULL,
        month = NULL
    ))


###################################################
### code chunk number 10: convertDate.Rnw:139-140
###################################################
(df <- add_transitions(df))


###################################################
### code chunk number 11: convertDate.Rnw:146-147
###################################################
transform(df, timepoint = format(timepoint, "%b-%Y"))


###################################################
### code chunk number 12: a
###################################################
head(Blackmore, 22)


###################################################
### code chunk number 13: >
###################################################
Blackmore <- transform(
        Blackmore,
        timepoint = as.Date("2000-01-01") + round(365.25 * age)
    )

head(Blackmore, 22)


###################################################
### code chunk number 14: >
###################################################
Blackmore <- transform(Blackmore, result = 0L)


###################################################
### code chunk number 15: convertDate.Rnw:193-194
###################################################
Blackmore <- add_prev_date(Blackmore)


###################################################
### code chunk number 16: convertDate.Rnw:199-200
###################################################
head(Blackmore, 22)


###################################################
### code chunk number 17: convertDate.Rnw:212-221
###################################################
Blackmore <- transform(
        Blackmore,
        prev_age = round(
                as.integer(prev_date - as.Date("2000-01-01")) / 365.25, 2
            ),
        timepoint = NULL, result = NULL, prev_date = NULL
    )

head(Blackmore, 22)


###################################################
### code chunk number 18: convertDate.Rnw:224-225
###################################################
rm(df, Blackmore)


