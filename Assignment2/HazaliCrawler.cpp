//==============================================//
// Name			: HazaliCrawler.cpp				//
// Author		: Victor Hazali 				//
// Description	: Simple parallel web crawler 	//
//==============================================//


#include <iostream>
#include <string>
#include <fstream>

// Using boost to do parsing
#include <boost/regex.hpp>

using namespace std;

const int DELAY = 10;		//time delay between HTTP GET requests
const int MAXRECV = 140 * 1024;

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

	// Connection header field
	request.append("Connection: close\r\n");

	// User Agent header field
	request.append("User-Agent: HazaliCrawler (github.com/vhazali/cs3103/assignment2)\r\n");

	// Accept header field
	request.append("Accept: text/html\r\n");
	
	// End of request
	request.append("\r\n");

	return request;
}

// Method to combine the hostname and path to get the correct fullpath
// Used to save full path
// Function takes in two strings, the hostname and path respectively
// Returns a string containing the fullpath
string get_full_path(const string host, const string path) {
	string full_url = host;
	full_url.append("/");
	full_url.append(path);
	const boost::regex rmv_all("[^a-zA-Z0-9]");
	const string full_path = boost::regex_replace(full_url, rmv_all, "_");

	//cout << "full path for saving: " + full_path << endl;	//debug line

	return full_path;
}

int connect(const string host, const string path) {
	const int PORT = 80;

	// setting up msock
	int m_sock;
	sockaddr_in m_addr;
	memset(&m_addr, 0, sizeof(m_addr));
	m_sock = socket(AF_INET, SOCK_STREAM, 0);

	int on = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1) {
		return false;
	}

	// Connecting
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(PORT);
	int status = inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);

	if (errno == EAFNOSUPPORT) {
		return false;
	}

	status = ::connect(m_sock, (sockaddr *) &m_addr, sizeof(m_addr));
	
	// Sending request
	string req = request(host, path);
	status = ::send(m_sock, req.c_str(), req.size(), MSG_NOSIGNAL);
	char buf[MAXRECV];
	cout<<"Request: "<<req<<endl;
	cout<<"=========================="<<endl;

	// Receiving Reply
	string recv = "";
	while (status != 0) {
		memset(buf, 0, MAXRECV);
		status = ::recv(m_sock, buf, MAXRECV, 0);
		recv.append(buf);
	}
	cout<<"Response: "<<recv<<endl;
	cout<<"=========================="<<endl;

	// Parsing the data received
}

class HTMLpage{
public:
	string hostname;
	string page;

	// Default Constructor
	// Initialises hostname and page to empty strings
	HTMLpage() {
		hostname = "";
		page ="";
	}

	// Method to parse the html file to look for hostname
	// Takes in the html file in the form of a string
	// Returns either the hostname if found or an emptry string otherwise
	string parseHTTP(const string str) {
		//TODO: instead of (.*), regex is probably more accurate with ([a-zA-Z0-9]*)
		//TODO: ?i doesn't seem to be correct
		const boost::regex re("(?i)http://(.*)/?(.*)");
		boost::smatch what;
		if (boost::regex_match(str, what, re)) {
			string host = what[1];
			boost:allgorithm::to_lower(host);
			return host;
		}
		return "";
	}

	// Method to parse the html file to look for hostname and page
	// Takes in the html file in the form of a string and a string to denote the original host
	// Returns either the page and the hostname if both are found
	// Returns an empty string for page if page cannot be resolved
	void parseHref(const string orig_host, string str) {
		//TODO: same changes as above
		const boost::regex re("(?i)http://(.*)/(.*)");
		boost::smatch what;

		if(boost::regex_match(str, what, re)) {
			//found a full URL, parse out hostname and parse page
			host = what[1];
			boost::allgorithm::to_lower(host);
			page = what[2];
		} else {
			//cannot find page but can build hostname
			host = orig_host;
			page="";
		}
	}

	// Method to parse 
	void parse(const string orig_host, const string hrf) {
		const string host = parseHTTP(hrf);

		if (!host.empty()) {
			parseHref(host, hrf);
		} else {
			hostname = orig_host;
			page = hrf;
		}

		if (page.length() == 0) {
			page ="/";
		}
	}
}

int main() {
	cout << "Program starting." << endl;
	cout << "Program finished." << endl;
	return 0;
}