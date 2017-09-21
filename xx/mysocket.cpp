
#include "Socket.h"

//发送图片前先转换为base64字符串
#include "base64.h"
//计时
#include <time.h>

//ofstream写文本文件
#include <iostream>
#include <fstream>
using namespace std;

//cv
#include "opencv2/opencv.hpp"


#pragma comment(lib,"WS2_32.LIB")

//
//UTF-8到GB2312的转换
char* U2G(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

HANDLE hMutex;
//------------------------------------------------------------------------------------
//函数名称:      Send(SOCKET sockClient)
/*函数功能:      发送数据
/*入口参数:      SOCKET sockClient				 
//出口参数:     
//全局变量引用:  
//调用模块:      无 
//------------------------------------------------------------------------------------*/
void Send(SOCKET sockClient)
{
	
	char sendBuf[MaxSize];  //image data
	int byte = 0;

	while(1)
	{
		WaitForSingleObject(hMutex, INFINITE);

		//
//		gets(sendBuf);
		byte= send(sockClient,sendBuf,strlen(sendBuf)+1,0);;//服务器从客户端接受数据
		if (byte<=0)
		{
			break;
		}	
		
		Sleep(1000);
		ReleaseMutex(hMutex);

	}
	closesocket(sockClient);//关闭socket,一次通信完毕
	
}
//------------------------------------------------------------------------------------
//函数名称:     Rec()
/*函数功能:     接收函数
/*入口参数:     SOCKET sockClient			 
//出口参数:     
//全局变量引用:  
//调用模块:      无 
//------------------------------------------------------------------------------------*/
void Rec(SOCKET sockClient)
{
	
	char revBuf[MaxSize];
	int byte = 0;

	while(1)
	{
		WaitForSingleObject(hMutex, INFINITE);

		byte= recv(sockClient,revBuf,strlen(revBuf)+1,0);//服务器从客户端接受数据
		if (byte<=0)
		{
			break;
		}	

		printf("%s\n",revBuf);

		Sleep(1000);
		ReleaseMutex(hMutex);

	}
	closesocket(sockClient);//关闭socket,一次通信完毕
	
}

extern SOCKET s;
extern  char *ServerIp ;
extern  int ServerPort ;
int SendImage(unsigned char* src, long length, std::string& result)
{
	int num;

	SOCKET s;
	WSADATA wsa;
	struct sockaddr_in serv;

	char sndBuf[1024], rcvBuf[2048];
	char *buffer = NULL;

	WSAStartup(MAKEWORD(2, 1), &wsa);

	if ((s = socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		perror("socket error!");
		exit(1);
	}

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(ServerPort);
	serv.sin_addr.S_un.S_addr = inet_addr(ServerIp);

	//
	double t = (double)cv::getTickCount();
	if ((connect(s, (struct sockaddr *)&serv, sizeof(serv)))<0)
	{
		perror("connet error!");
		exit(1);
	}
	t = double((cv::getTickCount() - t) * 1000 / cv::getTickFrequency());
	cout << "connect server:" << t << endl;

	//
	memset(sndBuf, 0, 1024);
	memset(rcvBuf, 0, sizeof(rcvBuf));

	t = (double)cv::getTickCount();
	std::string base64str = ZBase64::Encode(src, length);
	t = double((cv::getTickCount() - t) * 1000 / cv::getTickFrequency());
	cout << "base64 encode:" << t << endl;
  
	//头信息[例子]
	/*strcat(sndBuf, "POST ");
	strcat(sndBuf, "http://www.baidu.com");
	strcat(sndBuf, " HTTP/1.0\r\n");
	strcat(sndBuf, "Host: ");
	strcat(sndBuf, "baidu.com");
	strcat(sndBuf, "\r\n");
	strcat(sndBuf, "Connection: keep-alive\r\n");
	strcat(sndBuf, "\r\n");
	puts(sndBuf);*/
	/*char *header = "POST http://%s:%d HTTP/1.1\r\n"
		"Host: %s \r\n"
		"Content-Type: application/octet-stream\r\n"
		"Content-Length: %ld\r\n" // <-- substitute with the actual file size
		"Connection: keep-alive\r\n"
		"\r\n";*/

	const char*  dataformat1 = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><TX><TX_BODY><ENTITY><CUST_ID>1</CUST_ID><PICTURE>";
	const char*  dataformat2 = "</PICTURE></ENTITY></TX_BODY></TX>";
	int  len = strlen(base64str.c_str()) + strlen(dataformat1) + strlen(dataformat2); 
	char * databuf = new char[len];
	memset(databuf, 0, len);
	memcpy(databuf, dataformat1, strlen(dataformat1));
	memcpy(databuf + strlen(dataformat1),								base64str.c_str(), strlen(base64str.c_str()));
	memcpy(databuf + strlen(dataformat1) + strlen(base64str.c_str()),	dataformat2,		strlen(dataformat2));
	//sprintf(databuf, dataformat, base64str.c_str());

	length = len;
	const char* boundary = "---------------------------7da2137580612";
	
	sprintf(sndBuf, "POST http://172.16.32.188:8888/szyytface/tdrBfcm/multiFaceRecognize.action HTTP/1.1\r\n"
		"Host: 172.16.32.188\r\nConnection: keep-alive\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: application/xml; boundary=%s\r\n\r\n", length, boundary);
	int head_len = strlen(sndBuf);
	int packet_len = head_len + length;

	//join header + body
	char* packet = new char[packet_len ];
	memset(packet, 0, packet_len);

	memcpy(packet, sndBuf, head_len);
	memcpy(packet + head_len, databuf, length);
   
  delete databuf;

	t = cv::getTickCount();
	if ((num = send(s, packet, packet_len, 0))<0)
	{
		int errcode = GetLastError();
		printf("err:%d\n", errcode);
		perror("send error!");
		exit(1);
	}
	t = double((cv::getTickCount() - t) * 1000 / cv::getTickFrequency());
	cout << "send package:" << t << endl;

	delete packet;
//puts("send success!\n");

	
	result.clear();
	t = cv::getTickCount() - t;
//do {
		  if ((num = recv(s, rcvBuf, sizeof(rcvBuf), 0))<0)
      {
        int errcode = GetLastError();
        printf("err:%d\n", errcode);
        perror("recv error!");
        system("pause");
        exit(1);
      }
      else if (num>0)
      {
        //printf("%s\n", rcvBuf);

        /*	ofstream myfile;
        myfile.open("d:\\example_out.txt");
        myfile << rcvBuf;
        myfile.close();*/
        //result += string(rcvBuf);
      }	
      memset(rcvBuf, 0, sizeof(rcvBuf));
//} while (num>0);

		t = double((cv::getTickCount() - t) * 1000 / cv::getTickFrequency());
		cout << "recv package:" << t << endl;
	
//puts("\nread success!\n");

	closesocket(s);

//	system("pause");
	WSACleanup();

	return 0;

}

