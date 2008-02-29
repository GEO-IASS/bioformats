//
// JVMLinkClient.cpp
//

/*
JVMLink client/server architecture for communicating between Java and
non-Java programs using sockets.
Copyright (c) 2008 Hidayath Ansari and Curtis Rueden. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the UW-Madison LOCI nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE UW-MADISON LOCI ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include "JVMLinkClient.h"
#include <Windows.h>

#define DEFAULT_PORT 20345
#define MAX_PACKET_SIZE 65536

//TODO: clear memory at appropriate points.

JVMLinkClient::JVMLinkClient(void)
{
}

JVMLinkClient::~JVMLinkClient(void)
{
}

// -- Public API methods --

void JVMLinkClient::startJava(int arg_port, CString classpath) {
	port = arg_port == NULL ? DEFAULT_PORT : arg_port;
	CString command;
	// NB: Toggle comments to control debugging output for the server.
	//command.Format("-cp %s loci.jvmlink.JVMLinkServer %d", classpath, port);
	command.Format("-cp %s loci.jvmlink.JVMLinkServer -debug %d", classpath, port);
	debug("java " << command);
	//ShellExecute(NULL, "open", "javaw.exe" , command, "", SW_SHOW);
	ShellExecute(NULL, "open", "java.exe" , command, "", SW_SHOW);
}

void JVMLinkClient::shutJava() {
	debug("Terminating JVMLink server");
	sendInt(EXIT_CMD);
}

JVMLinkClient::ConnectionCode JVMLinkClient::establishConnection() {
	WSADATA wsaData;
	struct hostent *hp;
	unsigned int addr;
	struct sockaddr_in server;
	CString servername = "127.0.0.1"; 

	int wsaret=WSAStartup(0x101,&wsaData);
	if (wsaret) return WINSOCK_ERR;
	debug("Initialized WinSock");

	conn=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (conn==INVALID_SOCKET) return SOCKET_ERR;
	debug("Socket created");

	if (inet_addr(servername)==INADDR_NONE)
	{
		hp=gethostbyname(servername);
	}
	else
	{
		addr=inet_addr(servername);
		hp=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	}
	if (hp == NULL)
	{
		closesocket(conn);
		debug("Could not resolve network address: " << servername);
		return RESOLVE_ERR;
	}
	debug("Network address resolved");

	server.sin_addr.s_addr=*((unsigned long*)hp->h_addr);
	server.sin_family=AF_INET;
	server.sin_port=htons(port);
	if (connect(conn,(struct sockaddr*)&server,sizeof(server)))
	{
		closesocket(conn);
		debug("No server response on port " << port);
		return RESPONSE_ERR;	
	}
	debug("Connected to server: " << servername);
	return CONNECTION_SUCCESS;
}

int JVMLinkClient::closeConnection() {
	debug("Closing connection");
	closesocket(conn);
	debug("Socket closed");
	WSACleanup();
	debug("De-initialized WinSock");
	return 0;
}

JVMLinkObject* JVMLinkClient::getVar(CString name) {
	debug("getVar: requesting " << name);
	JVMLinkObject* obj = new JVMLinkObject(name);
	sendInt(GETVAR_CMD);
	sendMessage(name);
	obj->type = (Type) readInt();
	if (obj->type == ARRAY_TYPE) {
		obj->insideType = (Type) readInt();
		obj->length = (Type) readInt();
		obj->size = readInt();
		obj->data = readMessage(obj->size * obj->length);
		debug("getVar: got array: length=" << obj->length << ", type=" << obj->insideType);
	}
	else if (obj->type == STRING_TYPE) {	
		int len = readInt();
		char* buff = new char[len+1];
		int size = recv(conn,buff,len,0);
		buff[len] = '\0';
		obj->data = buff;
		obj->size = len;
		debug("getVar: got string: length=" << len << ", value=" << buff);
	}
	else {
		int size = readInt();
		obj->data = readMessage(size);
		obj->size = size;
		obj->insideType = NULL_TYPE;
		debug("getVar: got object: type=" << obj->type << ", size=" << obj->size);
	}
	return obj;
}

void JVMLinkClient::exec(CString command) {
	debug("exec: " << command);
	sendInt(EXEC_CMD);
	sendMessage(command);
}

void JVMLinkClient::setVar(JVMLinkObject* obj) {
	debug("setVar: " << obj->name << " = " << obj);
	sendInt(SETVAR_CMD);
	sendMessage(obj->name);
	sendInt((int) obj->type);
	if (obj->type == ARRAY_TYPE) {
		sendInt((int) obj->insideType);
		sendInt(obj->length);
		sendInt(obj->size);

		int sentBytes = 0;
		int totalBytes = (obj->size)*(obj->length);
		char* dataPointer = (char*) obj->data;
		while (sentBytes < totalBytes) {
			char* buff = (char*) (dataPointer + sentBytes);
			int packetSize = MAX_PACKET_SIZE;
			if (sentBytes + MAX_PACKET_SIZE > totalBytes) {
				packetSize = totalBytes - sentBytes;
			}
			send(conn,buff,packetSize,0);
			sentBytes += packetSize;
		}
	}
}

void JVMLinkClient::setVar(CString argname, int obj) {
	debug("setVar: " << argname << " = " << obj << " (int)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(INT_TYPE);
	sendInt(obj);
}

void JVMLinkClient::setVar(CString argname, CString obj) {
	debug("setVar: " << argname << " = " << obj << " (string)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(STRING_TYPE);
	sendMessage(obj);
}

void JVMLinkClient::setVar(CString argname, char obj) {
	debug("setVar: " << argname << " = " << obj << " (char)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(CHAR_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,1,0);
}

void JVMLinkClient::setVar(CString argname, Byte obj) {
	debug("setVar: " << argname << " = " << obj.data << " (byte)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(BYTE_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,1,0);
}

void JVMLinkClient::setVar(CString argname, float obj) {
	debug("setVar: " << argname << " = " << obj << " (float)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(FLOAT_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,sizeof(obj),0);
}

void JVMLinkClient::setVar(CString argname, bool obj) {
	debug("setVar: " << argname << " = " << obj << " (bool)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(BOOL_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,sizeof(obj),0);
}

void JVMLinkClient::setVar(CString argname, double obj) {
	debug("setVar: " << argname << " = " << obj << " (double)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(DOUBLE_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,sizeof(obj),0);
}

void JVMLinkClient::setVar(CString argname, long long obj) {
	debug("setVar: " << argname << " = " << obj << " (long)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(LONG_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,sizeof(obj),0);
}

void JVMLinkClient::setVar(CString argname, short obj) {
	debug("setVar: " << argname << " = " << obj << " (short)");
	sendInt(SETVAR_CMD);
	sendMessage(argname);
	sendInt(SHORT_TYPE);

	char* buff = (char*) (&obj);
	send(conn,buff,sizeof(obj),0);
}

// -- Private methods --

int JVMLinkClient::sendMessage(CString message) {
    sendInt(message.GetLength());
	send(conn,(LPCTSTR)message,message.GetLength(),0);
	return 0;
}

int JVMLinkClient::sendInt(int message) {
	char* buff = (char*) (&message);
	send(conn,buff,4,0);
	return 0;
}

void* JVMLinkClient::readMessage(int size) {
	char* buff = (char*) malloc(size);
	recv(conn,buff,size,0); //TODO: check if everything is received.
	return (void*) buff;
}

int JVMLinkClient::readInt() {
	return *(int*) readMessage(4);
}