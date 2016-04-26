#pragma once
#define BOOST_LOG_DYN_LINK 1

#include <stdexcept>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <functional>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

//using boost::shared_ptr;
typedef sinks::text_ostream_backend backend_t;
typedef sinks::asynchronous_sink<
            backend_t,
            sinks::unbounded_ordering_queue<
                logging::attribute_value_ordering< unsigned int, std::less< unsigned int > >
            >
        > sink_t;
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};



  /*severitymap["normal"] = normal;
  severitymap.insert(map<std::string, severity_level>::value_type("normal",normal));
  severitymap.insert(map<std::string, severity_level>::value_type("notification",notification));
  severitymap.insert(map<std::string, severity_level>::value_type("warning",warning));
  severitymap.insert(map<std::string, severity_level>::value_type("error",error));
  severitymap.insert(map<std::string, severity_level>::value_type("critical",critical));
*/

// The formatting logic for the severity level
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
{
    static const char* const str[] =
    {
        "normal",
        "notification",
        "warning",
        "error",
        "critical"
    };
    if (static_cast< std::size_t >(lvl) < (sizeof(str) / sizeof(*str)))
        strm << str[lvl];
    else
        strm << static_cast< int >(lvl);
    return strm;
}



typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(test_lg, src::logger_mt)

enum { LOG_RECORDS_TO_WRITE = 10000 };

//boost::shared_ptr< sink_t > initlog();
boost::shared_ptr< file_sink > initlog();


std::string&   replace_all(std::string&   str,const   std::string&   old_value,const   std::string&   new_value);     
std::string&   replace_all_distinct(std::string&   str,const   std::string&   old_value,const   std::string& new_value);  
