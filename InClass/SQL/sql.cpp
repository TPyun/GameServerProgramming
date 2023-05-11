#include <sqlext.h>

RETCODE rc;
HENV henv;
HDBC hdbc;
HSTMT hstmt;

SQLAllocEnv(&henv);
