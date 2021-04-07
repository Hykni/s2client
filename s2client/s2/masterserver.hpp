#pragma once

#include <core/prerequisites.hpp>
#include <network/httpclient.hpp>

namespace s2 {
	struct ms_login_info {
		bool success;
		string nickname;
		string cookie;
		int accountid;
	};
	struct ms_server_info {
		string name;
		string description;
		string ip;
		int port;
		int numplayers, maxplayers;
	};
	class masterserver {
	private:
		network::httpclient mHttp;
	
		struct node {
			enum NodeType : uint8_t {
				NTNone,
				NTInteger,
				NTArray,
				NTString,
				NTStringMap
			} type;
			node* parent;
			struct {
				int val;
			} i;
			struct {
				vector<node> elems;
				int cnt;
			} a;
			struct {
				map<string, string> m;
			} sm;
			struct {
				string str;
			} s;
		};
		node parse_ms_response(const string& data);
	public:
		masterserver();

		vector<ms_server_info> getserverlist();
		ms_login_info login(string_view username, string_view password);
	};
}
