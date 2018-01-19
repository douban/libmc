#pragma once
#include <plog/Logger.h>
#include <plog/Formatters/TxtFormatter.h>
#include <cstring>

namespace plog
{
    //////////////////////////////////////////////////////////////////////////
    // Empty initializer / one appender

    template<int instance>
    inline Logger<instance>& init(Severity maxSeverity = none, IAppender* appender = NULL)
    {
        static Logger<instance> logger(maxSeverity);
        return appender ? logger.addAppender(appender) : logger;
    }

    inline Logger<PLOG_DEFAULT_INSTANCE>& init(Severity maxSeverity = none, IAppender* appender = NULL)
    {
        return init<PLOG_DEFAULT_INSTANCE>(maxSeverity, appender);
    }

    //////////////////////////////////////////////////////////////////////////
    // CHAR variants for Windows

#ifdef _WIN32
#endif
}
