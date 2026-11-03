#ifndef ELOG_CFG_H
#define ELOG_CFG_H

#define ELOG_OUTPUT_ENABLE
#define ELOG_OUTPUT_LVL                 ELOG_LVL_DEBUG
#define ELOG_ASSERT_ENABLE

#define ELOG_LINE_BUF_SIZE              512
#define ELOG_LINE_NUM_MAX_LEN           5
#define ELOG_FILTER_TAG_MAX_LEN         24
#define ELOG_FILTER_KW_MAX_LEN          16
#define ELOG_FILTER_TAG_LVL_MAX_NUM     5
#define ELOG_NEWLINE_SIGN               "\r\n"

/* No color: the same bytes go to UART and a file. */
/* #define ELOG_COLOR_ENABLE */

/* Keep the log line compact; tag/level/time are added by EasyLogger. */
/* #define ELOG_FMT_USING_FUNC */
/* #define ELOG_FMT_USING_DIR */
/* #define ELOG_FMT_USING_LINE */

/* The RAM ring / log task provides asynchronous behaviour; do not use the
 * pthread-based async/buffer implementations. */
/* #define ELOG_ASYNC_OUTPUT_ENABLE */
/* #define ELOG_ASYNC_OUTPUT_USING_PTHREAD */
/* #define ELOG_BUF_OUTPUT_ENABLE */

#endif /* ELOG_CFG_H */
