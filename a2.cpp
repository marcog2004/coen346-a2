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
		lock_guard<mutex> lock(process_mtx); // activates the mutex and will automatically release once process is done
		if (isStarted == false) { // checks if the process has already started executing. If not, states that it has started and sets flag to true
			outFile << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Started\n";
			cout << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Started\n";
			isStarted = true;
		}

		// States that the process has resumed execution
		outFile << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Resumed\n";
		cout << "Time " << currentTime << ", User " << userID << ", Process " << processNumber << ", Resumed\n";
		

		int cpuTime = min(timeSlice, remainingTime); // Finds the minimum between the time slice it is allotted and the remaining time the process has to execute
		this_thread::sleep_for(seconds(cpuTime)); // Process sleeps to simulate execution for its allotted time
		remainingTime -= cpuTime; // Subtracts the time that was just used from the remaining time to find the time left after this execution.

		// Declares that the process has paused
		outFile << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Paused\n";
		cout << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Paused\n";

		if (remainingTime == 0) { // If the process has no time left, the process has completed. The process states that it has finished.
			outFile << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Finished\n";
			cout << "Time " << (currentTime + cpuTime) << ", User " << userID << ", Process " << processNumber << ", Finished\n";
		}
	}
};

void scheduleProcesses(vector<Process>& Processes, vector<char>& Users, int quantum) {
	ofstream outputFile("output.txt");

	//associatates processes to users
	map<char, vector<Process*>> userProcesses;
	for (auto& process : Processes) {
		userProcesses[process.userID].push_back(&process);
	}

	int currentTime = 1;
	bool processesRemaining = true;

	// keeps track of the next process to run for each user
	map<char, int> userNextProcessIndex;
	for (char user : Users) {
		userNextProcessIndex[user] = 0;
	}

	while (processesRemaining) {
		processesRemaining = false;

		// checks if users have available processes (readyTime <= currentTime, still has remainingTime left so > 0)
		vector<char> activeUsers;
		for (char user : Users) {
			bool hasReadyProcess = false;
			for (auto* process : userProcesses[user]) {
				if (process->readyTime <= currentTime && process->remainingTime > 0) {
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
			for (auto& process : Processes) {
				if (process.remainingTime > 0 && process.readyTime > currentTime && process.readyTime < nextReadyTime) {
					nextReadyTime = process.readyTime;
				}
			}

			if (nextReadyTime != INT_MAX) {
				currentTime = nextReadyTime;
				continue;
			}
			else {
				break; // no more processes if there are no users
			}
		}

		// quantum should be shared equally
		int userTimeSlice = quantum / activeUsers.size();

		// run a process from all active users
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
				// execution time is the min value of usertTimeSlice and processes remaining time
				// prevents execution time to go past the user's remaining time
				int executedTime = min(userTimeSlice, selectedProc->remainingTime);

				// thread creation with all available info
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
