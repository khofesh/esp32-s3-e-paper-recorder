# Runs on every build (cmake -P). Writes OUT with the host's local time.
string(TIMESTAMP ID "%s" UTC)
string(TIMESTAMP Y "%Y")
string(TIMESTAMP MO "%m")
string(TIMESTAMP D "%d")
string(TIMESTAMP H "%H")
string(TIMESTAMP MI "%M")
string(TIMESTAMP S "%S")
foreach(v MO D H MI S)
    string(REGEX REPLACE "^0([0-9])$" "\\1" ${v} "${${v}}")
endforeach()
file(WRITE "${OUT}.tmp"
"#ifndef BUILD_TIME_H
#define BUILD_TIME_H
#define BUILD_TIME_ID ${ID}u
#define BUILD_TIME_YEAR ${Y}
#define BUILD_TIME_MONTH ${MO}
#define BUILD_TIME_DAY ${D}
#define BUILD_TIME_HOUR ${H}
#define BUILD_TIME_MIN ${MI}
#define BUILD_TIME_SEC ${S}
#define BUILD_RTC_SYNC ${RTC_SYNC}
#define BUILD_RTC_SYNC_OFFSET_S ${RTC_SYNC_OFFSET_S}
#endif
")
file(RENAME "${OUT}.tmp" "${OUT}")
