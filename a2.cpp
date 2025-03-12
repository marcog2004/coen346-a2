#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>

using namespace std;

class Process {
public:
	string userID;
	int processNumber;
	int readyTime;
	int serviceTime;
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
							Process process;
							process.userID = userID;
							process.processNumber = i;
							process.readyTime = readyTime;
							process.serviceTime = serviceTime;
							Processes.push_back(process);
						}
					}
				}
			}
		
	}

	for (int i = 0; i < Processes.size(); i++) {
		cout << "User: " << Processes[i].userID << " | " << "Number: " << Processes[i].processNumber << " | " << "Ready Time: " << Processes[i].readyTime << " | " << "Service Time: " << Processes[i].serviceTime << "\n";
	}

}