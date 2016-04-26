#ifndef SERVER_RESOURCES_HPP
#define	SERVER_RESOURCES_HPP

#define BOOST_SPIRIT_THREADSAFE
#define BOOST_FILESYSTEM_VERSION 3
//
//  As an example program, we don't want to use any deprecated features
#ifndef BOOST_FILESYSTEM_NO_DEPRECATED 
#  define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_SYSTEM_NO_DEPRECATED 
#  define BOOST_SYSTEM_NO_DEPRECATED
#endif

#include "server_wss.hpp"
#include "client_wss.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
using namespace std;
using namespace boost::property_tree;
using namespace SimpleWeb;
using namespace boost::posix_time;
namespace fs = boost::filesystem;
typedef SimpleWeb::SocketServer<SimpleWeb::WSS> WssServer;
typedef SimpleWeb::SocketClient<SimpleWeb::WSS> WssClient;
map<size_t, FileUpload> uploadss;
string nfspaths = "";
std::mutex conLock_s;
std::mutex directory_is_empty_locks;
bool create_dirs(std::string dir)
{
	try
	{
		std::unique_lock<std::mutex> locker(conLock_s);
		return boost::filesystem::create_directories(dir);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return false;
	}
	catch (...)
	{
		std::cout << "unknown error" << std::endl;
		return false;
	}
}
string uploads_inits(size_t userid, string filename, int size, bool bin = false)
{
	std::unique_lock<std::mutex> locker(conLock_s);
	//uploadsinit((size_t)connection.get(),filename,size,true);
	string retString;
	ptime now = second_clock::local_time();
	string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
	try
	{
		string path = nfspaths+ filename;
		
		fs::path p(path);
		p.remove_filename();
		std::cout << p.string() << std::endl;
		if (!fs::exists(p))
		{
			if (!boost::filesystem::create_directories(p.string()))
			{
				retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":-9001,\"message\":\"create directory failed\",\"phase\":\"PREPARE\",\"ts\":\"" + now_str + "\"}";
				return retString;
			}
		}
		auto ret = uploadss.insert(make_pair(userid, FileUpload(path, size, bin)));
		if (ret.second)
		{
			cout << "map insert succeeded" << endl;
			//retString = "{\"type\":\"STOR\",\"message\":\"Upload initialized. Wait for data\",\"code\": 200}";
			retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\": 200,\"message\":\"Upload initialized. Wait for data\",\"phase\":\"PREPARE\",\"ts\":\"" + now_str + "\"}";
		}
		else
		{
			cout << "map insert failed" << endl;
			retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":-9001,\"message\":\"map insert failed\",\"phase\":\"PREPARE\",\"ts\":\"" + now_str + "\"}";
		}

		return retString;
	}
	catch (exception& e)
	{
		//retString = "{\"type\":\"STOR\",\"message\":\"init file failed\",\"code\": 500}";
		retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":-9001,\"message\":\"init exception\",\"phase\":\"PREPARE\",\"ts\":\"" + now_str + "\"}";
		return retString;
	}
}
string write_datas(size_t userid, string data)
{
	std::unique_lock<std::mutex> locker(conLock_s);
	//write_data((size_t)connection.get(),output_str);
	string retString;
	ptime now = second_clock::local_time();
	string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
	try
	{
		//uploadss[userid].addData(data);
		auto iter = uploadss.find(userid);
		if (iter != uploadss.end())
		{
			//cout << iter->second << endl;
			iter->second.addData(data);
		}
		else
		{
			cout << "cannot open file when add data" << endl;
			throw exception();
		}
		//cout << "add data succeeded" << endl;

		//retString = "{\"type\":\"DATA\",\"code\": 200,\"bytesRead\":" + boost::lexical_cast<string>(data.length()) + "}";
		
		retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":200,\"message\":\"write slice ok\",\"phase\":\"TRANSFER\",\"bytesRead\":" + boost::lexical_cast<string>(data.length()) + ",\"ts\":\"" + now_str + "\"}";
		return retString;
	}
	catch (exception& e)
	{
		//retString = "{\"type\":\"DATA\",\"code\": 500,\"bytesRead\":0}";
		retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":-9001,\"message\":\"" + string(e.what()) + "\",\"phase\":\"TRANSFER\",\"bytesRead\":0,\"ts\":\"" + now_str + "\"}";

		return retString;
	}
}
void finish_uploads(size_t userid)
{
	std::unique_lock<std::mutex> locker(conLock_s);
	auto iter = uploadss.find(userid);
	if (iter != uploadss.end())
	{
		iter->second.close();
		uploadss.erase(userid);
	}
	//cout << "double free" << endl;
}


void server_send_datas(WssServer& server, shared_ptr<WssServer::Connection> connection, string retString, string action)
{
	try
	{
		stringstream sendtoclient;
		sendtoclient << retString;
		server.send(connection, sendtoclient, [&](const boost::system::error_code& ec){
			if (ec) {
				cout << action << " Server: Error sending message. " <<
					//See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
					"Error: " << ec << ", error message: " << ec.message() << endl;
				//************************************************
				BOOST_LOG_SEV(server.slg, error) << action << " Server: Error in connection " << (size_t)connection.get() << ". " <<
					"Error: " << ec << ", error message: " << ec.message();
				server.initsink->flush();
				//************************************************
			}
		});
	}
	catch (std::exception& e)
	{
		//************************************************
		BOOST_LOG_SEV(server.slg, error) << action << " Server send: exception in connection:" << (size_t)connection.get();
		server.initsink->flush();
		//************************************************

	}
	catch (...)
	{
		//************************************************
		BOOST_LOG_SEV(server.slg, error) << action << " Server send: exception in connection:" << (size_t)connection.get();
		server.initsink->flush();
		//************************************************

	}
}
void server_sends(WssServer& server, shared_ptr<WssServer::Connection> connection, string retString, string action)
{
	cout << action << " Server: Sending message \"" << retString << "\" to " << (size_t)connection.get() << endl;
	//************************************************
	BOOST_LOG_SEV(server.slg, notification) << action << " Server: Sending message \"" << retString << "\" to " << (size_t)connection.get();
	server.initsink->flush();
	//************************************************
	server_send_datas(server, connection, retString, action);

}

string rename_files(string data_oldFile, string data_newFile)
{
	std::unique_lock<std::mutex> locker(conLock_s);
	//uploadssinit((size_t)connection.get(),filename,size,true);
	string retString;
	ptime now = second_clock::local_time();
	string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());

	try
	{
		/*cout << nfspaths + data_oldFile << endl;
		cout << nfspaths + data_newFile << endl;*/
		fs::path oldpath(nfspaths + data_oldFile);
		fs::path newpath(nfspaths + data_newFile);
		fs::rename(oldpath, newpath);
		
		cout << "rename success" << endl;
		retString = "{\"errorCode\":200,\"message\":\"rename success\",\"ts\":\"" + now_str + "\"}";

		return retString;
	}
	catch (fs::filesystem_error e)
	{
		cout << e.what()<< endl;
		retString = "{\"errorCode\":-9001,\"message\":\"" + string(e.what()) + "\",\"ts\":\"" + now_str + "\"}";
		return retString;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
		retString = "{\"errorCode\":-9001,\"message\":\"" + string(e.what()) + "\",\"ts\":\"" + now_str + "\"}";
		return retString;
	}
}
bool directory_is_emptys(fs::path p)
{
	std::unique_lock<std::mutex> locker(directory_is_empty_locks);
	//cout << "directory_is_empty" << endl;
	try
	{
		unsigned long file_count = 0;
		unsigned long dir_count = 0;
		unsigned long other_count = 0;
		unsigned long err_count = 0;

		if (!fs::exists(p))
		{
			return false;
		}
		//cout << "dirctory exists" << endl;
		if (fs::is_directory(p))
		{
			//std::cout << "\nIn directory: " << p << "\n\n";
			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_itr(p);
				dir_itr != end_iter;
				++dir_itr)
			{
				try
				{
					if (fs::is_directory(dir_itr->status()))
					{
						++dir_count;
						//std::cout << dir_itr->path().filename() << " [directory]\n";
					}
					else if (fs::is_regular_file(dir_itr->status()))
					{
						++file_count;
						//std::cout << dir_itr->path().filename() << "\n";
					}
					else
					{
						++other_count;
						//std::cout << dir_itr->path().filename() << " [other]\n";
					}

				}
				catch (const std::exception & ex)
				{
					++err_count;
					//std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
				}
			}
			std::cout << "\n" << file_count << " files\n"
				<< dir_count << " directories\n"
				<< other_count << " others\n"
				<< err_count << " errors\n";
			if (file_count + dir_count + other_count + err_count>0)
				return false;
			else
				return true;
		}
		else // must be a file
		{
			return false;
		}
	}
	catch (fs::filesystem_error e)
	{
		return false;
	}
	catch (exception& e)
	{
		return false;
	}
}
bool delete_files(string file,bool delete_empty)
{
	std::unique_lock<std::mutex> locker(conLock_s);
	
	try
	{
		//cout <<"delete_empty:"<< delete_empty << endl;
		fs::path p(file);
		fs::remove(file);
		if (delete_empty)
		{
			p.remove_filename();
			//cout << "delete_empty"<< endl;
			if (directory_is_emptys(p))
			{
				cout << "directory_is_empty"<< endl;
				remove_all(p);
			}
			else
			{
				cout << "directory is not empty" << endl;
				//return false;
			}
			
		}
		
		
		return true;
	}
	catch (fs::filesystem_error e)
	{
		return false;
	}
	catch (exception& e)
	{
		return false;
	}
}

void listfiles(ptree& listfiletree, string path)
{
	std::unique_lock<std::mutex> locker(directory_is_empty_locks);
	//cout << "directory_is_empty" << endl;
	//ptree filetree;
	try
	{
		//cout << path << endl;
		fs::path p(path);
		if (!fs::exists(p))
		{
			listfiletree.put<std::string>("file", "");
			//listfiletree.push_back(std::make_pair("", filetree));
		}
		//cout << "dirctory exists" << endl;
		if (fs::is_directory(p))
		{
			//std::cout << "\nIn directory: " << p << "\n\n";
			fs::directory_iterator end_iter;
			for (fs::directory_iterator dir_itr(p);
				dir_itr != end_iter;
				++dir_itr)
			{
				try
				{
					if (fs::is_regular_file(dir_itr->status()))
					{
						ptree children;
						children.put<std::string>("file", dir_itr->path().filename().string());
						listfiletree.push_back(std::make_pair("", children));
						//std::cout << dir_itr->path().filename() << "\n";
					}
				}
				catch (const std::exception & ex)
				{
					listfiletree.put<std::string>("file", "");
				}
			}
			//listfiletree.push_back(std::make_pair("", filetree));
		}
	
	}
	catch (fs::filesystem_error e)
	{
		listfiletree.put<std::string>("file", "");
		//listfiletree.push_back(std::make_pair("", filetree));
	}
	catch (exception& e)
	{
		listfiletree.put<std::string>("file", "");
		//listfiletree.push_back(std::make_pair("", filetree));
	}

}
void serverResourceUploads(WssServer& server, string url)
{
	auto& websocket = server.endpoint[url];
	websocket.onmessage = [&server](shared_ptr<WssServer::Connection> connection, shared_ptr<WssServer::Message> message) {
		ptime now = second_clock::local_time();
		string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
		try
		{
			stringstream data_ss;
			message->data >> data_ss.rdbuf();
			string clientmessage = data_ss.str();
			ptree pt;
			string retString = "";
			if (clientmessage.compare(0, 9, "#PREPARE#") == 0)
			{
				//************************************************
				BOOST_LOG_SEV(server.slg, notification) << "upload client(" << (size_t)connection.get() << "):" << clientmessage;
				server.initsink->flush();
				//************************************************

				clientmessage.erase(0,9);
				cout << __LINE__ << ":" << clientmessage << endl;
				stringstream stream;
				stream << clientmessage;
				read_json(stream, pt);
				
				string action = pt.get<string>("action");
				string transfer = pt.get<string>("transfer");
				ptree pdata = pt.get_child("data");
				string file = pdata.get<string>("file");
				size_t size = pdata.get<size_t>("size");
				bool overwrite = pdata.get<bool>("overwrite");
				/*cout << "action:" << action << endl;
				cout << "transfer:" << transfer << endl;
				cout << "file:" << file << endl;
				cout << "size:" << size << endl;
				cout << "overwrite:" << overwrite << endl;*/
				if (overwrite)
				{
					retString = uploads_inits((size_t)connection.get(), file, size);
				}
				else
				{
					fs::path p(nfspaths+file);
					if (fs::exists(p))
					{
						retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\": -9001,\"message\":\"file exist\",\"phase\":\"PREPARE\",\"ts\":\"" + now_str + "\"}";
					}
					else
					{
						retString = uploads_inits((size_t)connection.get(), file, size);
					}
				}

				if (retString.length() == 0)
				{
					retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\": -9001,\"message\":\"return string empty\",\"phase\":\"PREPARE\",\"ts\":\"" + now_str + "\"}";
				}
				server_sends(server, connection, retString, "upload");
			}
			else
			{
				string output_str;

				bool x=Base64Decode(clientmessage, &output_str);
				//output_str = "sffafasfdaf";
				cout << "decode base64:" << x << endl;
				if (x)
				{
					retString = write_datas((size_t)connection.get(), output_str);
					cout << "(size_t)connection.get(): " << (size_t)connection.get() << endl;
					if (retString.length() == 0)
					{
						retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":-9001,\"message\":\"read slice error\",\"phase\":\"TRANSFER\",\"bytesRead\":0,\"ts\":\"" + now_str + "\"}";
					}
					if (output_str.length() < 1024 * 80)
					{
						server_sends(server, connection, retString, "upload");
					}
					else
					{
						server_send_datas(server, connection, retString, "upload");
					}
					
					
				}
				else
				{
					retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\":-9001,\"message\":\"base64 decode error\",\"phase\":\"TRANSFER\",\"bytesRead\":0,\"ts\":\"" + now_str + "\"}";
					
					//server_send_data(server, connection, retString, "upload");
					server_sends(server, connection, retString, "upload");
				}
				
			}
			
		}
		catch (std::exception& e)
		{
			string retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\": -9001,\"message\":\"exception\",\"phase\":\"\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "upload");
			
		}
		catch (...)
		{
			string retString = "{\"action\":\"UPLOAD_FILE\",\"errorCode\": -9001,\"message\":\"exception\",\"phase\":\"\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "upload");
		}
	};

	websocket.onopen = [&server](shared_ptr<WssServer::Connection> connection) {
		cout << "upload Server: Opened connection " << (size_t)connection.get() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "upload Server: Opened connection " << (size_t)connection.get();
		server.initsink->flush();
		//************************************************
	};

	//See RFC 6455 7.4.1. for status codes
	websocket.onclose = [&server](shared_ptr<WssServer::Connection> connection, int status, const string& reason) {
		finish_uploads((size_t)connection.get());

		cout << "upload Server: Closed connection " << (size_t)connection.get() << " with status code:" << status <<",reason:"<<reason<< endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "upload Server: Closed connection " << (size_t)connection.get() <<" with status code : " << status <<",reason : "<<reason;
		server.initsink->flush();
		//************************************************
	};

	websocket.onerror = [&server](shared_ptr<WssServer::Connection> connection, const boost::system::error_code& ec) {
		cout << "upload Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, error) << "upload Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message();
		server.initsink->flush();
		//************************************************
	};
}
void serverResourceRenames(WssServer& server, string url)
{

	auto& websocket = server.endpoint[url];

	websocket.onmessage = [&server](shared_ptr<WssServer::Connection> connection, shared_ptr<WssServer::Message> message) {
		try
		{

			//To receive message from client as string (data_ss.str())
			stringstream data_ss;
			message->data >> data_ss.rdbuf();
			string clientmessage = data_ss.str();
			ptree pt;
			stringstream stream;
			stream << clientmessage;
			read_json(stream, pt);
			string action = pt.get<string>("action");
			string transfer = pt.get<string>("transfer");
			ptree pdata = pt.get_child("data");
			string data_oldFile = "";
			string data_newFile = "";
			//for (ptree::iterator it = pdata.begin(); it != pdata.end(); ++it)
			//{
			//	/*std::ostringstream buf;
			//	write_json(buf, (it->second), false);
			//	std::string json = buf.str();*/
			//	//cout<<json<<endl;
			//	data_oldFile = it->second.get<string>("oldFile");
			//	data_newFile= it->second.get<string>("newFile");
			//	//cout<<key<<endl;
			//}
			data_oldFile = pdata.get<string>("oldFile");
			data_newFile= pdata.get<string>("newFile");
			//cout << "action:"<<action << endl;
			string retString = "";
			if (action.compare("RENAME_FILE") == 0)
			{
				//************************************************
				BOOST_LOG_SEV(server.slg, notification) << "rename client(" << (size_t)connection.get() << "):" << clientmessage;
				server.initsink->flush();
				//************************************************			
				retString = rename_files(data_oldFile, data_newFile);
				server_sends(server, connection, retString, "rename");
			}
			else
			{
				ptime now = second_clock::local_time();
				string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
				retString = "{\"errorCode\":-9001,\"message\":\"action wrong\",\"ts\":\"" + now_str + "\"}";
				server_sends(server, connection, retString, "rename");
			}
			
		}
		catch (std::exception& e)
		{
			ptime now = second_clock::local_time();
			string now_str = to_iso_extended_string(now.date()) + " " +to_simple_string(now.time_of_day());
			//string retString = "{\"errorCode\":-9001,\"message\":\"exception\",\"ts\":\"" + now_str + "\"}";
			string retString = "{\"errorCode\":-9001,\"message\":\"" + string(e.what()) + "\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "rename");
			
		}
		catch (...)
		{
			ptime now = second_clock::local_time();
			string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
			string retString = "{\"errorCode\":-9001,\"message\":\"unknown exception\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "rename");
		}
	};

	websocket.onopen = [&server](shared_ptr<WssServer::Connection> connection) {
		cout << "rename Server: Opened connection: " << (size_t)connection.get() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "rename Server: Opened connection " << (size_t)connection.get();
		server.initsink->flush();
		//************************************************
	};

	//See RFC 6455 7.4.1. for status codes
	websocket.onclose = [&server](shared_ptr<WssServer::Connection> connection, int status, const string& reason) {
		finish_uploads((size_t)connection.get());

		cout << "rename Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "rename Server: Closed connection " << (size_t)connection.get() << " with status code " << status;
		server.initsink->flush();
		//************************************************
	};

	websocket.onerror = [&server](shared_ptr<WssServer::Connection> connection, const boost::system::error_code& ec) {
		cout << "rename Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, error) << "rename Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message();
		server.initsink->flush();
		//************************************************
	};
}
void serverResourceDeletes(WssServer& server, string url)
{

	auto& websocket = server.endpoint[url];

	websocket.onmessage = [&server](shared_ptr<WssServer::Connection> connection, shared_ptr<WssServer::Message> message) {
		try
		{

			//To receive message from client as string (data_ss.str())
			stringstream data_ss;
			message->data >> data_ss.rdbuf();
			string clientmessage = data_ss.str();
			//************************************************
			BOOST_LOG_SEV(server.slg, notification) << "delete client(" << (size_t)connection.get() << "):" << clientmessage;
			server.initsink->flush();
			//************************************************
			ptree pt;
			stringstream stream;
			stream << clientmessage;
			read_json(stream, pt);
			string action = pt.get<string>("action");
		
			string transfer = pt.get<string>("transfer");
			ptree pdata = pt.get_child("data");
			bool pdata_deleteDir = pdata.get<bool>("deleteDir");
			//cout << "pdata_deleteDir:" << pdata_deleteDir << endl;
			ptree pdata_files = pdata.get_child("files");
			cout << "action:" << action << endl;
			string retString = "";
			if (action.compare("DELETE_FILES") == 0)
			{
				int count = 0;
				for (ptree::iterator it = pdata_files.begin(); it != pdata_files.end(); ++it)
				{
					string file = it->second.get<string>("file");
					if (delete_files(nfspaths + file, pdata_deleteDir))
						count++;
				}
				
				ptime now = second_clock::local_time();
				string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
				if (count > 0)
				{
					retString = "{\"errorCode\":200,\"message\":\"delete " + boost::lexical_cast<string>(count)+" times(call delete function) success\",\"ts\":\"" + now_str + "\"}";
				}
				else
				{
					retString = "{\"errorCode\":-9001,\"message\":\"delete failed\",\"ts\":\"" + now_str + "\"}";
				}

				server_sends(server, connection, retString, "delete");
				
			}
			else
			{
				ptime now = second_clock::local_time();
				string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
				retString = "{\"errorCode\":-9001,\"message\":\"action wrong\",\"ts\":\"" + now_str + "\"}";
				server_sends(server, connection, retString, "delete");
			}

		}
		catch (std::exception& e)
		{
			ptime now = second_clock::local_time();
			string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
			string retString = "{\"errorCode\":-9001,\"message\":\"action wrong\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "delete");

		}
		catch (...)
		{
			ptime now = second_clock::local_time();
			string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
			string retString = "{\"errorCode\":-9001,\"message\":\"action wrong\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "delete");
		}
	};

	websocket.onopen = [&server](shared_ptr<WssServer::Connection> connection) {
		cout << "delete Server: Opened connection " << (size_t)connection.get() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "delete Server: Opened connection " << (size_t)connection.get();
		server.initsink->flush();
		//************************************************
	};

	//See RFC 6455 7.4.1. for status codes
	websocket.onclose = [&server](shared_ptr<WssServer::Connection> connection, int status, const string& reason) {
		finish_uploads((size_t)connection.get());

		cout << "delete Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "delete Server: Closed connection " << (size_t)connection.get() << " with status code " << status;
		server.initsink->flush();
		//************************************************
	};

	websocket.onerror = [&server](shared_ptr<WssServer::Connection> connection, const boost::system::error_code& ec) {
		cout << "delete Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, error) << "delete Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message();
		server.initsink->flush();
		//************************************************
	};
}
void serverResourceLists(WssServer& server, string url)
{

	auto& websocket = server.endpoint[url];

	websocket.onmessage = [&server](shared_ptr<WssServer::Connection> connection, shared_ptr<WssServer::Message> message) {
		try
		{
			//To receive message from client as string (data_ss.str())
			stringstream data_ss;
			message->data >> data_ss.rdbuf();
			string clientmessage = data_ss.str();
			//************************************************
			BOOST_LOG_SEV(server.slg, notification) << "list client(" << (size_t)connection.get() << "):" << clientmessage;
			server.initsink->flush();
			//************************************************
			ptree pt;
			stringstream stream;
			stream << clientmessage;
			read_json(stream, pt);
			string action = pt.get<string>("action");
		
			string transfer = pt.get<string>("transfer");
			ptree pdata = pt.get_child("data");
			string storagePath = pdata.get<string>("storagePath");
			//cout << "pdata_deleteDir:" << pdata_deleteDir << endl;
			ptree pdatainclude = pdata.get_child("include");
			ptree pdataexclude = pdata.get_child("exclude");
			cout << "action:" << action << endl;
			string retString = "";
			if (action.compare("LIST_FILES") == 0)
			{
				//
				ptree retJson, datachidren;
				retJson.put<int>("errorCode", 200);
				retJson.put<std::string>("message","list file success");
				
				datachidren.put<std::string>("storagePath", storagePath);
				//cout << storagePath << endl;
				int count = 0;
				ptree pinfiles;
				for (ptree::iterator it = pdatainclude.begin(); it != pdatainclude.end(); ++it)
				{
					ptree listfiletree;
					string type = it->second.get<string>("type");
					listfiles(listfiletree, nfspaths+"/"+storagePath+"/"+type);
					pinfiles.add_child(type, listfiletree);
				}
				datachidren.add_child("files", pinfiles);
				retJson.add_child("data", datachidren);

				ptime now = second_clock::local_time();
				string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
				////cout<<now_str<<endl;
				retJson.put<std::string>("ts", now_str);
				std::stringstream ss;
				write_json(ss, retJson);
				retString = ss.str();

				server_sends(server, connection, retString, "list");
				
			}
			else
			{
				ptime now = second_clock::local_time();
				string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
				retString = "{\"errorCode\":-9001,\"message\":\"action wrong\",\"ts\":\"" + now_str + "\"}";
				server_sends(server, connection, retString, "list");
			}

		}
		catch (std::exception& e)
		{
			ptime now = second_clock::local_time();
			string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
			string retString = "{\"errorCode\":-9001,\"message\":\"" + string(e.what()) + "\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "list");

		}
		catch (...)
		{
			ptime now = second_clock::local_time();
			string now_str = to_iso_extended_string(now.date()) + " " + to_simple_string(now.time_of_day());
			string retString = "{\"errorCode\":-9001,\"message\":\"unknown exception\",\"ts\":\"" + now_str + "\"}";
			server_sends(server, connection, retString, "list");
		}
	};

	websocket.onopen = [&server](shared_ptr<WssServer::Connection> connection) {
		cout << "list Server: Opened connection " << (size_t)connection.get() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "list Server: Opened connection " << (size_t)connection.get();
		server.initsink->flush();
		//************************************************
	};

	//See RFC 6455 7.4.1. for status codes
	websocket.onclose = [&server](shared_ptr<WssServer::Connection> connection, int status, const string& reason) {
		finish_uploads((size_t)connection.get());

		cout << "list Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, notification) << "list Server: Closed connection " << (size_t)connection.get() << " with status code " << status;
		server.initsink->flush();
		//************************************************
	};

	websocket.onerror = [&server](shared_ptr<WssServer::Connection> connection, const boost::system::error_code& ec) {
		cout << "list Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message() << endl;
		//************************************************
		BOOST_LOG_SEV(server.slg, error) << "list Server: Error in connection " << (size_t)connection.get() << ". " <<
			"Error: " << ec << ", error message: " << ec.message();
		server.initsink->flush();
		//************************************************
	};
}

#endif	