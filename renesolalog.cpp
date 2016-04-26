#include "renesolalog.h"

std::map<std::string,severity_level> severitymap={{ "normal", severity_level::normal },{ "notification", severity_level::notification },{ "warning", severity_level::warning},{ "error", severity_level::error },{ "critical", severity_level::critical}};

//boost::shared_ptr< sink_t > initlog()
boost::shared_ptr< file_sink > initlog()
{
   try
    {
		//*****************************************************************8
		//std::ifstream settings("settings.txt");
  //      if (!settings.is_open())
  //      {
  //          std::cout << "Could not open settings.txt file" << std::endl;
  //          return nullptr;
  //      }

  //      // Read the settings and initialize logging library
  //      logging::init_from_stream(settings);
		//*****************************************************************8

		//read config.ini
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini("config.ini", pt);
		std::string logName=pt.get<std::string>("log.name");
		std::string logLevel=pt.get<std::string>("log.level");
		//severity_level logLevel=pt.get<severity_level>("log.level");
        // Open a rotating text file
  //      boost::shared_ptr< std::ostream > strm(new std::ofstream(logName,std::ios::app));
		////strm
  //      if (!strm->good())
  //          throw std::runtime_error("Failed to open a text log file");

        // Create a text file sink
        /*typedef sinks::text_ostream_backend backend_t;
        typedef sinks::asynchronous_sink<
            backend_t,
            sinks::unbounded_ordering_queue<
                logging::attribute_value_ordering< unsigned int, std::less< unsigned int > >
            >
        > sink_t;*/
		
        //boost::shared_ptr< sink_t > sink(new sink_t(
        //    boost::make_shared< backend_t >(),
        //    // We'll apply record ordering to ensure that records from different threads go sequentially in the file
        //    keywords::order = logging::make_attr_ordering("RecordID", std::less< unsigned int >())));
		boost::shared_ptr< file_sink > sink(new file_sink(
            //keywords::file_name = logName+"_%Y%m%d_%H%M%S_%5N.log",      // file name pattern
			keywords::file_name = logName+"_%Y%m%d_%5N.log",
			//keywords::filter = expr::attr< severity_level >("Severity") >= logLevel,
            keywords::rotation_size = 16*1024 * 1024                     // rotation size, in characters
            ));

        // Set up where the rotated files will be stored
        sink->locked_backend()->set_file_collector(sinks::file::make_collector(
            keywords::target = "logs",                          // where to store rotated files
            keywords::max_size = 16*1024 * 1024,              // maximum total size of the stored files, in bytes
            keywords::min_free_space = 100 * 1024 * 1024        // minimum free space on the drive, in bytes
            ));

        // Upon restart, scan the target directory for files matching the file_name pattern
        sink->locked_backend()->scan_for_files();
        sink->set_formatter
        (
            expr::format("%1%: [%2%] [%3%] [%4%] - %5%")
                % expr::attr< unsigned int >("RecordID")
				% expr::attr< severity_level >("Severity")
                % expr::attr< boost::posix_time::ptime >("TimeStamp")
               // % expr::attr< boost::thread::id >("ThreadID")
				% expr::attr< attrs::current_thread_id::value_type >("ThreadID")
                % expr::smessage
        );
		//sink->set_filter(expr::attr< severity_level >("Severity").or_default(normal) >= logLevel);
		sink->set_filter(expr::attr< severity_level >("Severity").or_default(normal) >= severitymap[logLevel]);
        // Add it to the core
        logging::core::get()->add_sink(sink);

        // Add some attributes too
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());
		logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id());
       

        return sink;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return nullptr;
    }
}