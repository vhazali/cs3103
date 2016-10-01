//==============================================//
// Name			: HazaliCrawler.cpp				//
// Author		: Victor Hazali 				//
// Description	: Simple parallel web crawler 	//
//==============================================//

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0x0 //Don't request NOSIGNAL on systems where this is not implemented.
#endif

#include <iostream>
#include <string>
#include <fstream>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Using boost to do parsing
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

const int DELAY = 5;				// time delay between HTTP GET requests
const int MAXRECV = 200 * 1024;		// max size of received file

// Setting this flag to true allows the code to show a lot (you have been warned)
// of debug information. This can help to better understand what's happening in the code.
const bool DEBUG_MODE = true;

// Method to generate a GET HTTP request to be sent to server
// host is the hostname of the server we're accessing
// path is the path to the file we're requesting
string http_request(string host, string path) {

	// Generating get request line
	string request = "GET ";
	request.append(path);
	request.append(" HTTP/1.1\r\n");

	// Host header field
	request.append("Host: ");
	request.append(host);
	request.append("\r\n");
	// Accept header field
	request.append("Accept: text/html\r\n");
	// User Agent header field
	request.append("User-Agent: HazaliCrawler (github.com/vhazali/cs3103/assignment2)\r\n");
	// Connection header field
	request.append("Connection: close\r\n");
	// End of request
	request.append("\r\n");
	return request;
}

// Method to combine the hostname and path to get the correct fullpath
// Method takes in two strings, the hostname and path respectively
// Returns a string containing the fullpath
string get_full_path(const string host, const string path) {
	string full_url = host;
	full_url.append("/");
	full_url.append(path);
	const boost::regex rmv_all("[^a-zA-Z0-9]");
	const string full_path = boost::regex_replace(full_url, rmv_all, "_");

	//cout << "full path: " + full_path << endl;	//debug line

	return full_path;
}

// String formatter, applies format specified to the string
string string_format (const string &fmt, ...) {
	int size = 255;
	string str;
	va_list ap;

	while(1) {
		str.resize(size);
		va_start(ap, fmt);
		int n = vsnprintf((char*) str.c_str(), size, fmt.c_str(), ap);
		va_end(ap);
		if (n > -1 && n < size) {
			str.resize(n);
			return str;
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;
	}
	return str;
}

// Method performs address resolution from hostname to IP Address
// Method takes in a string containing the hostname
// Method returns a string containing the IPV4 address in the format
// ddd.ddd.ddd.ddd representing the IP address of the provided hostname
// If hostname cannot be resolved, returns an empty string instead
string getIPfromHostname(const string hostname) {
	try {
		hostent* h = gethostbyname(hostname.c_str());
		if (h) {
			return std::string(inet_ntoa(**(in_addr**)h->h_addr_list));
		}
	} catch (std::exception const &exc) {
		if (DEBUG_MODE) {
			cout <<"Error in resolving IP address.\n Error is: "<<exc.what()<<endl;
			return "";
		}
	}
	return "";
}

// class HTMLpage{
// public:
// 	string hostname;
// 	string page;

// 	// Default Constructor
// 	// Initialises hostname and page to empty strings
// 	HTMLpage() {
// 		hostname = "";
// 		page = "";
// 	}

// };
// take in one url string
// check against dictionary, if non existent, add to dictonary and queue
// if existent do nothing
void splitToHostNameAndPage(string url) {
	boost::regex expr("^.*://(?:[wW]{3}\.)?([^:/]*)/(.*)$", boost::regex::perl);
	boost::smatch what;

	if (boost::regex_search (url, what, expr)){
		for(int i=0;i<what.size();i++) {
			if(DEBUG_MODE){
				cout<<i<<	": "<<what[i]<<endl;
			}
		}
			// TODO: Check against dictionary
	}
}

void parseHTMLpage(string htmlPage){
	const boost::regex rmv_cr("[\\r|\\n]");
	const string htmlWithoutCarriage = boost::regex_replace(htmlPage, rmv_cr, "");

	if(DEBUG_MODE) {
		cout<<"html page after removing carriage return:\n===============================\n"
			<<htmlWithoutCarriage<<"===============================\n";
	}

	boost::regex getURLregex("(?:(?:https?)://|www\.)(?:\([-a-zA-Z0-9+&@#\/%=~_|$?!:,.]*\)|[-a-zA-Z0-9+&@#\/%=~_|$?!:,.])*", boost::regex::perl);
	boost::sregex_token_iterator iter(htmlWithoutCarriage.begin(), htmlWithoutCarriage.end(), getURLregex, 0);
	boost::sregex_token_iterator end;

	// TODO: put into queue 
	for(int counter=1; iter!=end; ++iter, ++counter) {
		if(DEBUG_MODE){
				cout<<counter<<": "<<*iter<<endl;
		}
		splitToHostNameAndPage(*iter);
	}
}

int connect(const string host, const string path) {
	const int PORT = 80;

	// setting up socket
	int m_sock;
	sockaddr_in m_addr;
	memset(&m_addr, 0, sizeof(m_addr));
	m_sock = socket(AF_INET, SOCK_STREAM, 0);

	if(DEBUG_MODE) {
		cout<<"socket done"<<endl;	// debug line
	}

	int on = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1) {
		if (DEBUG_MODE) {
			cout << "Error in setsockopt function" << endl;	//debug line
		}
		return false;
	}
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(PORT);

	string ipAdd = getIPfromHostname(host);
	if (DEBUG_MODE) {
		cout<< "ip Add: " << ipAdd << endl;
	}
	int status = inet_pton(AF_INET, ipAdd.c_str(), &m_addr.sin_addr);

	if (errno == EAFNOSUPPORT) {
		if (DEBUG_MODE) {
			cout << "Error, earnosupport" << endl;	//debug line
		}
		return false;
	}

	string req = http_request(host, path);
	// Connecting
	status = ::connect(m_sock, (sockaddr *) &m_addr, sizeof(m_addr));
	if (DEBUG_MODE) {
		cout<<"connected"<<endl;	// debug line
	}
	// Sending request
	status = ::send(m_sock, req.c_str(), req.size(), MSG_NOSIGNAL);
	cout<<"Request:\n"<<"=========================="<<endl<<req<<endl<<"=========================="<<endl;

	if (DEBUG_MODE && status == -1) {
		cout<<"Error in sending"<<endl;
		cout<<"Error is :"<<strerror(errno)<<endl;
	}

	// Receiving Reply
	char buf[MAXRECV];
	string recv = "";
	while (status != 0) {
		memset(buf, 0, MAXRECV);
		status = ::recv(m_sock, buf, MAXRECV, 0);
		recv.append(buf);
	}
	if (DEBUG_MODE){
		cout<<"Response: "<<recv<<endl;
		cout<<"=========================="<<endl;
	}

	// Parsing the data received
	try {
		parseHTMLpage(recv);
	} catch (boost::regex_error& e) {
		cout << "Regex Error: " << e.what() << endl;
	}
	return 1;
}

int main() {
	cout << "Program starting." << endl;
	// This is my personal website which contains a list of seed urls
	connect("www.comp.nus.edu.sg","/~vhazali/");
	cout << "Program finished." << endl;
	return 0;
}