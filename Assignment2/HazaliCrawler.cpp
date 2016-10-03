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
#include <queue>
#include <unordered_map>
#include <sys/time.h>

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
const bool DEBUG_MODE = false;
const int SERVER_COUNT = 50;

// Method to get time
long long wall_clock_time()
{
#ifdef __linux__
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return (long long)(tp.tv_nsec + (long long)tp.tv_sec * 1000000000ll);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)(tv.tv_usec * 1000 + (long long)tv.tv_sec * 1000000000ll);
#endif
}

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
			cout <<"Error in resolving IP address.\n Error is: "<<exc.what()<<endl<<endl;
			return "";
		}
	}
	return "";
}

// Method splits a url into its hostname and page
// Input url must have either http:// or https:// at the front
// Returns a pair of strings, first component being the hostname
// Second componenet being the page.
// Hostname is returned without trailing /
// Page is returned with a prefix /
// If parsing fails, returns emptry strings for both.
pair<string, string> splitToHostNameAndPage(string url) {
	boost::regex expr("^.*://((?:[wW]{3}\.)?[^:/]*)/(.*)$", boost::regex::perl);
	boost::smatch what;

	pair<string, string>result ={"",""};
	if (boost::regex_search (url, what, expr)){
		if (DEBUG_MODE) {
			cout<<"Splitting to Hostname and Page.\nSplit contents:\n";
			for(int i=0;i<what.size();i++) {
				cout<<i<<	": "<<what[i]<<endl;
			}
			cout<<endl;
		}
		if(what.size() < 3) {
			if(DEBUG_MODE) {
				cerr<<"Error, unable to parse hostname and page from url\n";
				cout<<"Contents of parsed url:\n";
				for(int i=0;i<what.size();i++) {
				cout<<i<<	": "<<what[i]<<endl;
				}
				cout<<endl;
			}
			return result;
		}
		result.first=what[1];
		result.second="/" + what[2];
	}
	return result;
}

// Method parses the HTML page to look for urls
// Urls that are accepted must satisfy the following conditions
// Method then returns an iterator for all the urls found
boost::sregex_token_iterator parseHTMLpage(string htmlPage){
	const boost::regex rmv_cr("[\\r|\\n]");
	const string htmlWithoutCarriage = boost::regex_replace(htmlPage, rmv_cr, "");

	if(DEBUG_MODE) {
		cout<<"html page after removing carriage return:\n===================================================\n"
			<<htmlWithoutCarriage<<"===================================================\n";
	}

	boost::regex getURLregex("(?:(?:https?)://|www\.)(?:\([-a-zA-Z0-9+&@#\/%=~_|$?!:,.]*\)|[-a-zA-Z0-9+&@#\/%=~_|$?!:,.])*", boost::regex::perl);
	boost::sregex_token_iterator iter(htmlWithoutCarriage.begin(), htmlWithoutCarriage.end(), getURLregex, 0);

	return iter;
}

// Method connects to a html page and takes note of the time taken for server to respond.
// Method returns the html page received in a string
string getHTMLpage(const string url, float &timeTaken) {
	if(DEBUG_MODE) {
		cout<<"Starting to get html page."<<endl;
	}
	pair<string, string> hostNameAndPage = splitToHostNameAndPage(url);
	string host = hostNameAndPage.first;
	string path = hostNameAndPage.second;
	if(DEBUG_MODE) {
		cout<<"url: "<<url<<endl;
		cout<<"host: "<<host<<endl;
		cout<<"path: "<<path<<endl;
		cout<<endl;
	}
	long long before, after;

	const int PORT = 80;
	string recv = "";
	int on = 1;

	// setting up socket
	int m_sock;
	sockaddr_in m_addr;
	memset(&m_addr, 0, sizeof(m_addr));
	m_sock = socket(AF_INET, SOCK_STREAM, 0);

	if(DEBUG_MODE) {
		cout<<"socket done\n\n";	// debug line
	}
	
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1) {
		if (DEBUG_MODE) {
			cout << "Error in setsockopt function" << endl;	//debug line
		}
		return recv;
	}
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(PORT);

	string ipAdd = getIPfromHostname(host);
	if (DEBUG_MODE) {
		cout<< "ip Add: " << ipAdd << endl<<endl;
	}
	int status = inet_pton(AF_INET, ipAdd.c_str(), &m_addr.sin_addr);

	if (errno == EAFNOSUPPORT) {
		if (DEBUG_MODE) {
			cerr << "Error, earnosupport" << endl;	//debug line
		}
		return recv;
	}

	string req = http_request(host, path);
	// Connecting
	status = ::connect(m_sock, (sockaddr *) &m_addr, sizeof(m_addr));
	if (DEBUG_MODE) {
		cout<<"connected\n\n";
	}
	// Sending request
	status = ::send(m_sock, req.c_str(), req.size(), MSG_NOSIGNAL);
	if (DEBUG_MODE) {
		cout<<"Request sent. Starting timer.\n\n";
	}
	before = wall_clock_time();

	cout<<"=========================="<<endl<<"Request:\n"<<"=========================="<<endl<<req;

	if (status == -1) {
		if(DEBUG_MODE) {
			cout<<"Error in sending"<<endl;
			cerr<<"Error is :"<<strerror(errno)<<endl<<endl;
		}
		return recv;
	}

	// Receiving Reply
	char buf[MAXRECV];
	while (status != 0) {
		memset(buf, 0, MAXRECV);
		status = ::recv(m_sock, buf, MAXRECV, 0);
		recv.append(buf);
	}
	after = wall_clock_time();
	timeTaken = ((float)(after - before))/1000000000;

	if (DEBUG_MODE){
		cout<<"Response received.\n";
		printf("Time taken for response: %6.4f seconds\n",timeTaken);
		cout<<"Response: "<<recv<<endl;
		cout<<"=========================="<<endl;
	}

	// Cleanup
	close(m_sock);
	return recv;
}

bool isSeen(string hostName, unordered_map<string,float> seenHosts) {
	if (seenHosts.find(hostName) == seenHosts.end()) {
		return false;
	}
	return true;
}

int work() {
	queue <string> urlQueue;
	unordered_map<string, float> seenHosts;
	string htmlPage, hostName, page, urlToVisit;
	boost::sregex_token_iterator urls;
	pair<string, string> hostNameAndPage={"",""};
	float timeTaken;
	bool seen = false;
	int siteCount = 0;

	// Seed urls
	urlQueue.push("http://www.comp.nus.edu.sg/~vhazali/");
	// urlQueue.push("http://www.jimdo.com/");
	urlQueue.push("http://websitesetup.org/");
	urlQueue.push("http://www.awwwards.com/websites/responsive-design/");
	seenHosts.insert({"www.comp.nus.edu.sg",0.0});
	// seenHosts.insert({"www.jimdo.com",0.0});
	seenHosts.insert({"websitesetup.org/",0.0});
	seenHosts.insert({"www.awwwards.com/",0.0});



	if(DEBUG_MODE) {
		cout<<"\nStarting on queue\n\n";
	}

	while(!urlQueue.empty() && siteCount++<SERVER_COUNT) {
		seen = false;
		urlToVisit = urlQueue.front();
		urlQueue.pop();
		if(DEBUG_MODE) {
			cout<<"Starting on queue with url: "<<urlToVisit<<endl<<endl;
		}
		// Add current page to seen
		hostNameAndPage = splitToHostNameAndPage(urlToVisit);		
		htmlPage = getHTMLpage(urlToVisit, timeTaken);
		if(htmlPage == "") {
			if(DEBUG_MODE) {
				cout<<"Empty string from "<<urlToVisit<<endl<<endl;
			}
			continue;
		}
		seenHosts[hostNameAndPage.first] = timeTaken;
		urls = parseHTMLpage(htmlPage);
		boost::sregex_token_iterator end;

		// Checking through all the urls
		for(; urls!=end; ++urls) {
			if(DEBUG_MODE) {
				cout<<"url found: "<<*urls<<endl<<endl;
			}
			hostNameAndPage = splitToHostNameAndPage(*urls);
			hostName = hostNameAndPage.first;
			page = hostNameAndPage.second;
			// Check if already seen
			if (isSeen(hostName, seenHosts)) {
				// Do nothing
				if(DEBUG_MODE) {
					cout<<hostName<<" is already seen.\n\n";
				}
				continue;
			} else {
				// Add to queue and mark as seen
				if(DEBUG_MODE) {
					cout<<hostName<<" has not been seen. Adding to queue\n\n";
				}
				urlQueue.push(*urls);
				seenHosts.insert({hostName,0.0});
			}
		}
		if(DEBUG_MODE) {
			if(urlQueue.empty()) {
				cout<<"Reached end of queue\n\n";
			}
		}
	}

	if(DEBUG_MODE) {
		for (auto it = seenHosts.begin(); it != seenHosts.end(); it++) {
			cout<<it->first<<": "<<it->second<<endl;
		}
	}
	// Write to file
	fstream fs;
	fs.open("output.txt",fstream::out);
	fs.width(50);
	fs<<left<<"Hostname";
	fs<<"Time taken in seconds\n";
	for(auto it = seenHosts.begin(); it != seenHosts.end(); it++) {
		fs.width(50);
		fs<<left<<it->first;
		fs<<it->second<<endl;
	}
	fs.close();
	return 0;
}

int main() {
	signal(SIGPIPE, SIG_IGN);
	cout << "Program starting." << endl;
	work();
	cout << "Program finished." << endl;
	return 0;
}