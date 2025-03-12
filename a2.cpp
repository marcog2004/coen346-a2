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

mutex process_mtx, scheduler_mtx;

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

void scheduleProcesses(vector<Process>& Processes, vector<char>& Users, int quantum) {
	ofstream outputFile("output.txt");

	//organize processes by user
	map<char, vector<Process*>> userProcesses;
	for (auto& process : Processes) {
		userProcesses[process.userID].push_back(&process);
	}

	int currentTime = 1;
	bool processesRemaining = true;

	// keep track of the next process to run for each user
	map<char, int> userNextProcessIndex;
	for (char user : Users) {
		userNextProcessIndex[user] = 0;
	}

	while (processesRemaining) {
		processesRemaining = false;

		// Find users with ready processes at this time
		vector<char> activeUsers;
		for (char user : Users) {
			bool hasReadyProcess = false;
			for (auto* proc : userProcesses[user]) {
				if (proc->readyTime <= currentTime && proc->remainingTime > 0) {
					hasReadyProcess = true;
					processesRemaining = true;
					break;
				}
			}
			if (hasReadyProcess) {
				activeUsers.push_back(user);
			}
		}

		// check if users are empty
		if (activeUsers.empty()) {
			int nextReadyTime = INT_MAX;
			for (auto& proc : Processes) {
				if (proc.remainingTime > 0 && proc.readyTime > currentTime && proc.readyTime < nextReadyTime) {
					nextReadyTime = proc.readyTime;
				}
			}

			if (nextReadyTime != INT_MAX) {
				currentTime = nextReadyTime;
				continue;
			}
			else {
				break; // no more processes
			}
		}

		// quantum should be shared equally
		int userTimeSlice = quantum / activeUsers.size();

		// Run one process from each active user
		for (char userId : activeUsers) {
			// Find the first ready process for this user
			Process* selectedProc = nullptr;

			for (auto* proc : userProcesses[userId]) {
				if (proc->readyTime <= currentTime && proc->remainingTime > 0) {
					selectedProc = proc;
					break;
				}
			}

			if (selectedProc) {
				// Calculate actual execution time
				int executedTime = min(userTimeSlice, selectedProc->remainingTime);

				// Create a thread to run this process
				int processCurrentTime = currentTime;
				thread t([selectedProc, userTimeSlice, processCurrentTime, &outputFile]() {
					lock_guard<mutex> lock(scheduler_mtx); // one process at a time
					selectedProc->runProcess(userTimeSlice, processCurrentTime, outputFile);
					});

				t.join(); // wait for process to finish

				// update current time
				currentTime += executedTime;
			}
		}
	}

	outputFile.close();
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

	thread schedulingThread(scheduleProcesses, std::ref(Processes), std::ref(Users), quantum);
	schedulingThread.join();

}