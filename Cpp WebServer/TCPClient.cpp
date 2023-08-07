#include "TCPClient.hpp"
#include <string>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif
/*
��������� �����������
*/
TCPClient::TCPClient(){}

/*
����������� ������
*/
TCPClient::TCPClient(int socket, sockaddr_in addr) :TCPSocket(socket,addr){}

TCPClient::TCPClient(TCPSocket& sock): TCPSocket(sock){}

TCPClient::~TCPClient()
{
    this->~TCPSocket();
	state = Disconnected;
}

/*
���������� ��������� ������
*/
TCPClient::State TCPClient::getState()
{
	return state;
}

/*
���������� ��������� �� �����
std::string message - ������, ������� ���������� ���������
��� ������� �������� ������ ���������� ���������� ������������ ����
��� ������ ���������� ��������� ��� ������
*/
size_t TCPClient::send(std::string message)
{
	size_t sended_total = 0;
	do
	{
		size_t size = (message.size() - sended_total > packet_size) ? packet_size : (message.size() - sended_total);
		int s = ::send(socket, &message[sended_total], size, 0);
		if (s == -1)
		{
			int code =
#ifdef _WIN32
				WSAGetLastError()
#else
				errno
#endif
				;
			if (
				code !=
#ifdef _WIN32
				WSAEWOULDBLOCK//����� �� �������� ��������� �����
#else
				EAGAIN
#endif
				)
			{
				state = Error;
				throw std::string("Error during sending: "+std::to_string(code));
			}
		}
		if (s == 0)
		{
			state = Disconnected;
			return 0;
		}
		if (s > 0)
			sended_total += (size_t)s;
	} while (sended_total != message.size());
	state = Success;
	return sended_total;
}

/*
������ ��������� �� ������
��� �������� �������� ���������� �������� ������
��� ������ ���������� ������ ������
*/
std::string TCPClient::recv()
{
	char buffer[packet_size];
	std::string returned;
	int recieved_total = 0;
	do
	{
		int r = ::recv(socket, buffer, packet_size, 0);
		if (r == -1)
		{
			int code =
#ifdef _WIN32
				WSAGetLastError()
#else
				errno
#endif
				;
			state = Error;
			throw std::string("Error during reading: "+std::to_string(code));
			//				if (
			//					code ==
			//#ifdef _WIN32
			//					WSAEWOULDBLOCK//������ �� �����
			//#else
			//					EAGAIN
			//#endif
			//					)
			//				{
			//					state=Error;
			//					return "";
			//				}
#ifdef _WIN32
			if (code == WSAECONNRESET)
			{
				state = Disconnected;
				return "";
			}
#endif
		}
		if (r == 0)
		{
			state = Disconnected;
			return "";
		}
		if (r > 0)
		{
			recieved_total += r;
			returned.insert(returned.end(), buffer, buffer + r);
		}
	} while (recieved_total%packet_size == 0);
	state = Success;
	return returned;
}
