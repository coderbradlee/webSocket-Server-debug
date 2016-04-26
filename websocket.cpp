
#include "serverResource.hpp"
#include "serverResources.hpp"

int main() {
	try
	{
		unsigned short port,ports;
		size_t threads;
		string upload_url;
		string rename_url;
		string delete_url;
		string list_url;
		string server_crt;
		string server_key;
		{
			string fullpath = boost::filesystem::initial_path<boost::filesystem::path>().string();
			boost::property_tree::ptree pt;
			boost::property_tree::ini_parser::read_ini(fullpath+"/config.ini", pt);
			cout << fullpath << endl;
			port = boost::lexical_cast<unsigned short>(pt.get<std::string>("webserver.port"));
			ports = boost::lexical_cast<unsigned short>(pt.get<std::string>("webserver.ports"));
			threads = boost::lexical_cast<size_t>(pt.get<std::string>("webserver.threads"));
			server_crt=pt.get<std::string>("webserver.server_crt");
			server_key=pt.get<std::string>("webserver.server_key");		
				
			upload_url = "^/" + pt.get<std::string>("webserver.upload_url") + "/?$";
			rename_url = "^/" + pt.get<std::string>("webserver.rename_url") + "/?$";
			delete_url = "^/" + pt.get<std::string>("webserver.delete_url") + "/?$";
			list_url = "^/" + pt.get<std::string>("webserver.list_url") + "/?$";
			nfspath = pt.get<std::string>("nfs.path");
		}

		cout << port << endl;;
		cout << threads << endl;;
		cout << upload_url << endl;;
		cout << rename_url << endl;;
		cout << delete_url << endl;;
		cout << list_url << endl;;
		cout << nfspath << endl;;

		if (!boost::filesystem::exists(nfspath))
		{
			cout << "\nNot found: " << nfspath << endl;
			if (create_dir(nfspath))
			{
				cout << "create path: " << nfspath << endl;
			}
			else
			{
				cout << "create path failed: " << nfspath << endl;
			}
		}

		//WebSocket (WS)-server at port 8080 using 4 threads
		WsServer server(port, threads);
		serverResourceUpload(server, upload_url);
		serverResourceRename(server, rename_url);
		serverResourceDelete(server, delete_url);
		serverResourceList(server, list_url);
		thread server_thread([&server](){
			//Start WS-server
			server.start();
		});
		this_thread::sleep_for(chrono::seconds(1));

		WssServer servers(ports, threads, server_crt, server_key);
		serverResourceUploads(servers, upload_url);
		serverResourceRenames(servers, rename_url);
		serverResourceDeletes(servers, delete_url);
		serverResourceLists(servers, list_url);
		thread server_threads([&servers](){
			//Start WS-server
			servers.start();
		});
		this_thread::sleep_for(chrono::seconds(1));



		server_threads.join();
		server_thread.join();
	}
	catch (json_parser_error& e)
	{
		cout << "json read or write error" << endl;
		return -1;
	}
	catch (exception& e) {
		cout << e.what()<< endl;
		return -1;
	}
	catch (...) {
		cout << "unknown error" << endl;
		return -1;
	}
    return 0;
}
