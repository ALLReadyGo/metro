#include "metro/utils/Logger.h"

namespace metro
{


Logger::Logger(SourceFile file, int line)
  : sourceFile_(file), fileLine_(line),
    level_(logLevel_())
{

}

Logger::Logger(Logger::SourceFile file, int line, LogLevel level)
  : sourceFile_(file), fileLine_(line),
    level_(level)
{

}

Logger::Logger(SourceFile file, int line, bool isSysErr)
  : sourceFile_(file), fileLine_(line),
{

}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
{

}

Logger::~Logger()
{
    
}

}