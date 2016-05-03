
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
			nfspaths = pt.get<std::string>("nfs.path");
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
		this_thread::sleep_for(chrono::seconds(2));



{
	//test wss
	WssClient client("localhost:8081/upload", false);
    client.onmessage = [&client](shared_ptr<WssClient::Message> message) {
			stringstream data_ss;
			data_ss << message->data.rdbuf();
			cout << "Client: Message received: \"" << data_ss.str() << "\"" << endl;
			// cout << "Client: Sending close connection" << endl;
			// client.send_close(1000);

			ptree pt;
			read_json(data_ss, pt);
			string phase = pt.get<string>("phase");
			if (phase.compare("TRANSFER") == 0)
			{
				cout << "Client: Sending close connection" << endl;
				client.send_close(1000);
			}
			else
			{
				stringstream sss;
			
				sss << "sffafasfdaf";
				cout << "Client: Sending message: \"" << sss.str() << "\"" << endl;
				client.send(sss);
			}
			

			

			
		};

		client.onopen = [&client]() {
			cout << __LINE__<<" Client: Opened connection" << endl;

			stringstream ss;
			
			ss << "#PREPARE#{\
				\"action\": \"UPLOAD_FILE\",\
				\"transfer\" : \"base64\",\
				\"data\" : {\
				\"file\":\"/1/test.pdf\",\
				\"size\" : 11,\
				\"overwrite\" : true},\
			    \"ts\" : \"2015 - 07 - 31 08 : 00 : 00\"}";
			
			cout << "Client: Sending message: \"" << ss.str() << "\"" << endl;
			client.send(ss);
			
		};

		client.onclose = [](int status, const string& reason) {
			cout << "Client: Closed connection with status code " << status << endl;
		};

		//See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
		client.onerror = [](const boost::system::error_code& ec) {
			cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
		};

		client.start();
}


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
