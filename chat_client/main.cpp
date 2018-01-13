#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include "sio_client.h"

using namespace sio;
using namespace std;

mutex _lock;

condition_variable_any _cond;

socket::ptr current_socket;

bool connect_finish = false;

int users = -1;

const string SERVER_URL = "http://128.199.204.33";  // 디지털오션
const string SERVER_PORT = "7711";


class Listener
{
private:
	sio::client& m_client;

public:
	Listener(sio::client& client) : m_client(client)
	{
	}

	void OnConnected()
	{
		_lock.lock();
		_cond.notify_all();
		connect_finish = true;
		cout << "서버 연결 성공" << endl;
		_lock.unlock();
	}

	void OnClose(client::close_reason const& reason)
	{
		cout << "서버 연결 종료" << endl;
		exit(0);
	}

	void OnFail()
	{
		cout << "서버 연결 실패" << endl;
		exit(0);
	}
};

void bind_events()
{
	current_socket->on("new message", sio::socket::event_listener_aux([&](string const&name, message::ptr const& data, bool isAck, message::list& ack_resp) {
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		string message = data->get_map()["message"]->get_string();
		cout << user << ": " << message << endl;
		_lock.unlock();
	}));

	current_socket->on("user joined", sio::socket::event_listener_aux([&](string const&name, message::ptr const& data, bool isAck, message::list& ack_resp) {
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		users = data->get_map()["numUsers"]->get_int();
		cout << user << "님이 채팅방에 참여했습니다. 채팅방의 현재 인원은 " << users << "명 입니다." << endl;
		_lock.unlock();
	}));

	current_socket->on("user left", sio::socket::event_listener_aux([&](string const&name, message::ptr const& data, bool isAck, message::list& ack_resp) {
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		users = data->get_map()["numUsers"]->get_int();
		cout << user << "님이 채팅방에서 떠났습니다. 채팅방의 현재 인원은 " << users << "명 입니다." << endl;
		_lock.unlock();
	}));
}

int main()
{
	sio::client client;
	Listener listener(client);

	client.set_open_listener(std::bind(&Listener::OnConnected, &listener));
	client.set_close_listener(std::bind(&Listener::OnClose, &listener, placeholders::_1));
	client.set_fail_listener(std::bind(&Listener::OnFail, &listener));

	client.connect(SERVER_URL + ":" + SERVER_PORT);

	_lock.lock();
	if (!connect_finish)
		_cond.wait(_lock);
	_lock.unlock();

	current_socket = client.socket();


	string nickname;

	while (nickname.length() == 0)
	{
		cout << "닉네임을 입력해주세요: ";
		getline(cin, nickname);
	}

	current_socket->on("login", sio::socket::event_listener_aux([&](string const&name, message::ptr const& data, bool isAck, message::list& ack_resp) {
		_lock.lock();
		users = data->get_map()["numUsers"]->get_int();
		cout << "채팅방에 참여했습니다. 채팅방의 현재 인원은 " << users << "명 입니다." << endl;
		_cond.notify_all();
		_lock.unlock();
		current_socket->off("login");
	}));

	current_socket->emit("add user", nickname);

	_lock.lock();
	if (users < 0)
		_cond.wait(_lock);
	_lock.unlock();

	bind_events();

	for (string line; getline(cin, line);)
	{
		if (line.length() > 0)
		{
			if (line == "exit")
				break;
		}	
		current_socket->emit("new message", line);

		_lock.lock();
		cout << nickname << ": " << line << endl;
		_lock.unlock();
	}


	cout << "서버와의 연결을 끊는 중 입니다.." << endl;

	client.sync_close();
	client.clear_con_listeners();

	return 1;
}


