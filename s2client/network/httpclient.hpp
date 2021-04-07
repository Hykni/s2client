#pragma once

#include <core/prerequisites.hpp>
#include <network/tcpclient.hpp>

namespace network {
	struct httpresponse {
		int status;
		map<string, string> headers;
		string data;
	};
	class httpclient : protected tcpclient {
	private:
		string mHost;
		map<string, string> mHeaders = {
			{ "User-Agent", "PHP Script" }
		};

		string getheader(const string& name, const string& d="");
	
		httpresponse parseresponse(string&& rsp);
		string query(string_view path, string_view method, string_view data="");
	public:
		httpclient();

		bool connect(string_view hostname);
		void disconnect();

		void header(string name, string val);

		string get(string_view hostname, string_view path);
		string post(string_view hostname, string_view path, string_view data);
		string get(string_view path);
		string post(string_view path, string_view data);
	};
}
