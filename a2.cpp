#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>
#include <mutex>
#include <thread>
#include <map>


using namespace std;
using namespace chrono;

mutex process_mtx;

class Process {
public:
	char userID;
	int processNumber;
	int readyTime;
	int serviceTime;
	bool isStarted;
	int remainingTime;

	Process(char iuserID, int iprocessNumber, int ireadyTime, int iserviceTime) {
		userID = iuserID;
		processNumber = iprocessNumber;
		readyTime = ireadyTime;
		serviceTime = iserviceTime;
		remainingTime = iserviceTime;
		isStarted = false;
	}

	void runProcess(int timeSlice, int currentTime, ofstream& outFile) {
		lock_guard<mutex> lock(process_mtx);
		if (isStarted == false) {
			outFile << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Started\n";
			cout << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Started\n";
			isStarted = true;
		}
		
		outFile << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Resumed\n";
		cout << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Resumed\n";
		

		int cpuTime = min(timeSlice, remainingTime);
		this_thread::sleep_for(seconds(cpuTime));
		remainingTime -= cpuTime;

		outFile << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Paused\n";
		cout << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Paused\n";

		if (remainingTime == 0) {
			outFile << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Finished\n";
			cout << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Finished\n";
		}
	}
};

void scheduleProcesses(vector<Process> Processes, vector<char> Users) {
	ofstream outputFile("output.txt");
	Processes[1].runProcess(1, 1, outputFile);
}

int main() {

	ifstream inputFile("input.txt");

	if (!inputFile) {
		cerr << "Error opening input file";
		return 1;
	}

	int quantum;

	vector<Process> Processes;

	vector<char> Users;

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
				Users.push_back(userID);
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
							Processes.push_back(process);
						}
					}
				}
			}

	}

	for (int i = 0; i < Processes.size(); i++) {
		cout << "User: " << Processes[i].userID << " | " << "Number: " << Processes[i].processNumber << " | " << "Ready Time: " << Processes[i].readyTime << " | " << "Service Time: " << Processes[i].serviceTime << "\n";
	}

	thread schedulingThread(scheduleProcesses, Processes, Users);
	schedulingThread.join();

}