#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>
#include <mutex>
#include <thread>
#include <map>


using namespace std;

class Process {
public:
	char userID;
	int processNumber;
	int readyTime;
	int serviceTime;
	bool isRunning;
	int remainingTime;
	mutex mtx;

	Process(char iuserID, int iprocessNumber, int ireadyTime, int iserviceTime) {
		userID = iuserID;
		processNumber = iprocessNumber;
		readyTime = ireadyTime;
		serviceTime = iserviceTime;
		remainingTime = iserviceTime;
		isRunning = false;
	}
	
	Process(Process&& other)
		: userID(other.userID), processNumber(other.processNumber),
		readyTime(other.readyTime), serviceTime(other.serviceTime),
		isRunning(other.isRunning), remainingTime(other.remainingTime) {
	}

	Process& operator=(Process&& other){
		if (this != &other) {
			userID = other.userID;
			processNumber = other.processNumber;
			readyTime = other.readyTime;
			serviceTime = other.serviceTime;
			isRunning = other.isRunning;
			remainingTime = other.remainingTime;
		}
		return *this;
	}

	void running(int timeSlice, int currentTime, ofstream& outFile) {
		lock_guard<mutex> lock(mtx);
		isRunning = true;
		outFile << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Resumed";
		cout << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Resumed";

		int executionTime = min(timeSlice, remainingTime);
		this_thread::sleep_for(chrono::seconds(executionTime));
		remainingTime -= executionTime;

		outFile << "Time " << (currentTime + executionTime) << ", User " << userID << ", Process " << processNumber << ", Paused\n";
		cout << "Time " << (currentTime + executionTime) << ", User " << userID << ", Process " << processNumber << ", Paused\n";
		isRunning = false;
	}
};

int main() {

	ifstream inputFile("input.txt");

	if (!inputFile) {
		cerr << "Error opening input file";
		return 1;
	}

	int quantum;

	vector<Process> Processes;

	string value;
	char valChar;

	string amountProcessesString = "0";
	int amountProcesses = 0;

	string readyTimeString;
	int readyTime;

	string serviceTimeString;
	int serviceTime;

	inputFile >> quantum;

	cout << "Quantum: " << quantum << "\n";

	char userID = NULL;

	while (inputFile >> value) {
			valChar = value[0];
			if (static_cast<int>(valChar) >= 65 && static_cast<int>(valChar) <= 90) {
				userID = valChar;
				inputFile >> amountProcessesString;
				amountProcesses = stoi(amountProcessesString);
				for (int i = 0; i < amountProcesses; i++) {
					for (int j = 0; j <= 2; j++) {
						if (j == 0) {
							inputFile >> readyTimeString;
							readyTime = stoi(readyTimeString);
						}
						else if (j == 1) {
							inputFile >> serviceTimeString;
							serviceTime = stoi(serviceTimeString);
						}
						else {
							Process process(userID, i, readyTime, serviceTime);
							Processes.push_back(move(process));
						}
					}
				}
			}

	}

	for (int i = 0; i < Processes.size(); i++) {
		cout << "User: " << Processes[i].userID << " | " << "Number: " << Processes[i].processNumber << " | " << "Ready Time: " << Processes[i].readyTime << " | " << "Service Time: " << Processes[i].serviceTime << "\n";
	}

}