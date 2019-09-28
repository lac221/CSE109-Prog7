/*
CSE 109: Fall 2018
Lydia Cornwell
LAC221
client that interacts with the server provided. Reads a maze and uses an algorithm to solve it.
Program #7
*/

#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<string>
#include<cerrno>
#include<cstring>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<signal.h>
#include<algorithm>
#include<netdb.h>
#include<thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

void readFromServer(int fd, void* buf, size_t num);
char  mazeSolver(int, int, int, int, int fd, char);
void writeToServer(int fd, const void* buf, size_t num);

int main(int argc, char** argv)
{
	//get command line arguments for Load
	size_t seed = 0;
	size_t rows = 0;
	size_t cols = 0;
	int defaultFlag = 1;
	int loadFlag = 1;

	if(argc == 4)
	{
		defaultFlag = 0;
		seed = atoi(argv[1]);
		rows = atoi(argv[2]);
		cols = atoi(argv[3]);
		if(rows > 5000 || cols > 5000)
		{
			defaultFlag = 1;
			loadFlag = 0;
		}
	}

	//read file and use information to connect to server
	short portnum;

	int file = open("/home/merle/CSE109/prog7student/connection.txt", O_RDONLY);
	if(file < 0)
	{
		return -1;
	}

	int retval = read(file, &portnum, 2);
	if(retval <= 0)
	{
		close(file);
		return -1;
	}

	size_t hostnameLen;

	retval = read(file, &hostnameLen, 8);
	if(retval <= 0)
	{
		close(file);
		return -1;
	}

	char* machine = (char*)malloc((hostnameLen + 1) * sizeof(char));

	retval = read(file, machine, hostnameLen);
	if(retval <= 0)
	{
		close(file);
		return -1;
	}

	int mySocket;
	if((mySocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Socket failed");
		return 1;
	}

	sockaddr_in client;
	memset(&client, 0, sizeof(sockaddr_in));

	client.sin_family = AF_INET;
	client.sin_port = htons(portnum);

	hostent* host = gethostbyname(machine);
	if(host == NULL)
	{
		perror("Could not reslive hostname");
		return 1;
	}

	memmove(&(client.sin_addr.s_addr), host->h_addr_list[0], 4);
	int conn = connect(mySocket, (sockaddr*)&client, sizeof(sockaddr_in));
	if(conn == -1)
	{
		perror("could not connect");
		free(machine);
		return 1;
	}


	free(machine);

	//flags for directions you can go
	int nFlag = 0;
	int sFlag = 0;
	int eFlag = 0;
	int wFlag = 0;
	//char item;
	char face = 'e';
	while(1)
	{
		//read lines from server, depending on what they say do different things
		size_t toRead;
		readFromServer(mySocket, &toRead, sizeof(size_t));

		char buffer[toRead + 1];
		readFromServer(mySocket, buffer, toRead);
		buffer[toRead] = '\0';
		cout<< buffer;
		cout << flush;

		string bufferString(buffer);

		if(bufferString.find("You can go 'N'orth.") != string::npos)
		{
			nFlag = 1;
		}
		if(bufferString.find("You can go 'S'outh.") != string::npos)
		{
			sFlag = 1;
		}
		if(bufferString.find("You can go 'E'ast.") != string::npos)
		{
			eFlag = 1;
		}
		if(bufferString.find("You can go 'W'est.") != string::npos)
		{
			wFlag = 1;
		}
		//if(bufferString.find("You have found a") != string::npos)
		//{
			
	//	}

		if(bufferString.find("command> ") != string::npos)
		{
			//if its the first command and you are not supposed to default, then send load command 
			if(loadFlag == 1 && defaultFlag == 0)
			{
				char* loadCommand = (char*)malloc(22*sizeof(char));
				sprintf(loadCommand, "L %ld %ld %ld", seed, rows, cols);
				
				size_t toWrite = strlen(loadCommand);
				writeToServer(mySocket, &toWrite, sizeof(size_t));
				writeToServer(mySocket, loadCommand, toWrite);
				loadFlag = 0;
			}
			else
			{
				//call algorithm to solve maze
				face = mazeSolver(nFlag, sFlag, eFlag, wFlag, mySocket, face);
				nFlag = 0;
				sFlag = 0;
				eFlag = 0;
				wFlag = 0;
			}
			cout << endl;
		}
	}
	return 1;
}

void readFromServer(int fd, void* buf, size_t num)
{
	size_t remaining = num;
	ssize_t sofar;
	char* bufp = (char*)buf;
	while(remaining > 0)
	{
		if((sofar = read(fd, bufp, remaining)) < 0)
		{
			cerr << " Failed to read file";
			close(fd);
			exit(1);
		}
		if(sofar == 0)
		{
			close(fd);
			exit(1);
		}
		bufp += sofar;
		remaining -= sofar;
	}
}

char mazeSolver(int nFlag, int sFlag, int eFlag, int wFlag, int fd, char face)
{
	/*
	i) Face East
	ii) Turn Left
	iii) If you face a wall, turn right
	iv) If you still face a wall, jump back to iii
	v) Move forward
	vi) Jump back to step ii
	*/

	size_t toWrite = 2;

	if(face == 'e')
	{
		if(nFlag ==1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "N\n", 2);
			face = 'n';
			return face;
		}
		else if(eFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "E\n", 2);
			face = 'e';
			return face;

		}
		else if(sFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "S\n", 2);
			face = 's';
			return face;

		}
		else if(wFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "W\n", 2);
			face = 'w';
			return face;
		}
//		else
//		{
//			cerr << "Im stuck" << endl;
//		}
	}

	if(face == 's')
	{
		if(eFlag ==1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "E\n", 2);
			face = 'e';
			return face;
		}
		else if(sFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "S\n", 2);
			face = 's';
			return face;

		}
		else if(wFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "W\n", 2);
			face = 'w';
			return face;

		}
		else if(nFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "N\n", 2);
			face = 'n';
			return face;
		}
		//else
		//{
	//		cerr << "Im stuck" << endl;
		//}
	}

	if(face == 'w')
	{
		if(sFlag ==1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "S\n", 2);
			face = 's';
			return face;
		}
		else if(wFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "W\n", 2);
			face = 'w';
			return face;

		}
		else if(nFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "N\n", 2);
			face = 'n';
			return face;

		}
		else if(eFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "E\n", 2);
			face = 'e';
			return face;
		}
//		else
//		{
//			cerr << "Im stuck" << endl;
//		}
	}

	if(face == 'n')
	{
		if(wFlag ==1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "W\n", 2);
			face = 'w';
			return face;
		}
		else if(nFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "N\n", 2);
			face = 'n';
			return face;

		}
		else if(eFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "E\n", 2);
			face = 'e';
			return face;

		}
		else if(sFlag == 1)
		{
			writeToServer(fd, &toWrite, sizeof(size_t));
			writeToServer(fd, "S\n", 2);
			face = 's';
			return face;
		}
//		else
//		{
//			cerr << "Im stuck" << endl;
//		}
	}
	return face;
}

void writeToServer(int fd, const void* buf, size_t num)
{
	size_t remaining = num;
	ssize_t sofar;
	char* bufp = (char*)buf;

	while(remaining > 0)
	{
		if((sofar = write(fd, bufp, remaining)) <= 0)
		{
			sofar = 0;
		}
		remaining -= sofar;
		bufp += sofar;
	}

}


