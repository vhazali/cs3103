//==============================================//
// Name			: HazaliCrawler.cpp				//
// Author		: Victor Hazali 				//
// Description	: Simple parallel web crawler 	//
//==============================================//


#include <iostream>
#include <string>
#include <fstream>

using namespace std;

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

int main() {
	cout << "Program starting." << endl;
	cout << http_request("hazali.com", "index.html");
	cout << "Program finished." << endl;
	return 0;
}