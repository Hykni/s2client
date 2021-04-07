#include "httpclient.hpp"

#include <sstream>
#include <ios>

namespace network {
	string httpclient::getheader(const string& name, const string& d) {
		auto it = mHeaders.find(name);
		if (it == mHeaders.end())
			return d;
		return it->second;
	}
	httpresponse httpclient::parseresponse(string&& rsp) {
		httpresponse response;
		std::istringstream str(rsp);
		string http;
		string statusdesc;
		str >> http;
		str >> response.status;
		str >> statusdesc;
		str.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		for (string line; std::getline(str, line);) {
			auto idx = line.find(':');
			if (idx != string::npos) {
				string name = line.substr(0, idx);
				string val = line.substr(idx + 1);
				response.headers[name] = val;
			}
			if (line.length() == 1 && line[0] == '\r') {
				break;
			}
		}
		response.data = str.str().substr(static_cast<unsigned int>(str.tellg()));
		return response;
	}
	string httpclient::query(string_view path, string_view method, string_view data)
	{
		std::stringstream request;
		request << method << " " << path << " HTTP/1.1\r\n";
		request << "Host: " << mHost << "\r\n";
		request << "User-Agent: " << mHeaders["User-Agent"] << "\r\n";
		if (data.size() > 0) {
			if (mHeaders.find("Content-Type") != mHeaders.end()) {
				request << "Content-Type: " << mHeaders["Content-Type"] << "\r\n";
			}
			else {
				request << "Content-Type: application/x-www-form-urlencoded\r\n";
			}
			request << "Content-Length: " << data.size() << "\r\n";
		}
		request << "Connection: " << getheader("Connection", "close") << "\r\n";
		request << "\r\n";
		request << data;
		auto reqstr = request.str();
		send(std::move(reqstr));
		
		string rspdata;
		recvstring(&rspdata);
		auto response = parseresponse(std::move(rspdata));
		assert(response.status == 200);
		return response.data;
	}
	httpclient::httpclient() {
	}
	bool httpclient::connect(string_view hostname)
	{
		mHost = hostname;
		return tcpclient::connect(mHost.c_str(), 80);
	}
	void httpclient::disconnect() {
		network::tcpclient::disconnect();
	}
	void httpclient::header(string name, string val) {
		mHeaders[name] = val;
	}
	string httpclient::get(string_view hostname, string_view path) {
		if (!connect(hostname))
			return "";
		string result = get(hostname, path);
		disconnect();
		return result;
	}
	string httpclient::post(string_view hostname, string_view path, string_view data) {
		if (!connect(hostname))
			return "";
		string result = post(path, data);
		disconnect();
		return result;
	}
	string httpclient::get(string_view path) {
		return query(path, "GET");
	}
	string httpclient::post(string_view path, string_view data) {
		return query(path, "POST", data);
	}
}
