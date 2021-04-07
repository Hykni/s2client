#include "masterserver.hpp"
#include <core/io/logger.hpp>

namespace s2 {
	masterserver::node masterserver::parse_ms_response(const string& data)
	{
		masterserver::node root;
		deque<string> tokens;
		string token = "";
		{
			for (size_t idx = 0; idx < data.length(); idx++) {
				auto c = data.at(idx);
				if (c == ';' || c == ':')
				{
					tokens.push_back(token);
					token.clear();
				}
				else if (c == '{' || c == '}') {
					tokens.push_back(string(1, c));
					token.clear();
				}
				else {
					token += c;
				}
			}
		}

		root.parent = nullptr;
		root.type = node::NTArray;
		node* current = &root;
		string type = "", value = "";
		while (!tokens.empty()) {
			token = tokens.front();
			tokens.pop_front();
			if (type.empty()) {
				type = token;
				if (type == "}") {
					assert(current->a.elems.size() == (2*current->a.cnt));
					if (current->a.cnt > 0) {
						auto& arr = current->a.elems;
						if (arr[0].type == node::NTString && arr[1].type == node::NTString) {
							current->type = node::NTStringMap;
							while (!arr.empty()) {
								auto val = arr.back();
								arr.pop_back();
								auto key = arr.back();
								arr.pop_back();
								current->sm.m[key.s.str] = val.s.str;
							}
						}
					}
					if (current->parent == nullptr)
						break;
					current = current->parent;
					type.clear();
				}
			}
			else {
				value = token;
				current->a.elems.push_back(node{});
				node& nn = current->a.elems.back();
				nn.parent = current;
				switch (type[0]) {
				case 'N':
					break;
				case 'a':
					nn.type = node::NTArray;
					nn.a.cnt = std::stoi(value);
					nn.a.elems.reserve(nn.a.cnt);
					current = &nn;
					assert(tokens[0] == "{");
					tokens.pop_front();
					break;
				case 'i':
					nn.type = node::NTInteger;
					nn.i.val = std::stoi(value);
					break;
				case 's':
					auto slen = std::stoi(value);
					value = tokens.front();
					assert(value.front() == '"' && value.back() == '"');
					value = value.substr(1, value.length() - 2);
					assert(value.length() == slen);
					nn.type = node::NTString;
					nn.s.str = value;
					tokens.pop_front();
					break;
				}
				type.clear();
			}
		}

		return root;
	}
	masterserver::masterserver() {
	}

	vector<ms_server_info> masterserver::getserverlist() {
		vector<ms_server_info> servers;
		auto response = mHttp.post("masterserver1.talesofnewerth.com", "/irc_updater/svr_request_pub.php", "f=get_online");
		auto data = parse_ms_response(response);
		auto& srvrList = data.a.elems[0].a.elems;
		for (size_t i = 0; i < srvrList.size(); i += 2) {
			int id = srvrList[i].i.val;
			auto& info = srvrList[i + 1].sm.m;
			
			servers.push_back({
				.name = info["name"],
				.description = info["description"],
				.ip = info["ip"],
				.port = std::stoi(info["port"]),
				.numplayers = std::stoi(info["num_conn"]),
				.maxplayers = std::stoi(info["max_conn"]),
			});
		}
		return servers;
	}
	ms_login_info masterserver::login(string_view username, string_view password) {
		auto authdata = core::format("f=auth&email=%s&password=%s", username, password);
		auto response = mHttp.post("masterserver1.talesofnewerth.com", "/irc_updater/irc_requester.php", authdata);
		auto logindata = parse_ms_response(response);
		
		if (logindata.a.elems.size() < 1 || logindata.a.elems[0].type != node::NTStringMap)
			return { .success = false };

		auto& m = logindata.a.elems[0].sm.m;

		if (m.find("cookie") == m.end() || m["cookie"].length() < 1)
			return { .success = false };

		ms_login_info logininfo{
			.success = true,
			.nickname = m["nickname"],
			.cookie = m["cookie"],
			.accountid = std::stoi(m["account_id"])
		};
		return logininfo;
	}
}
