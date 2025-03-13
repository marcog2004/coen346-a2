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

class Process { // process class
public: // public variables
	char userID; // associated user id (single letter)
	int processNumber; // associated process number (according to user)
	int readyTime; // time when process is ready to execute
	int serviceTime; // amount of time process needs with cpu
	bool isStarted; // flag to check if program is in progress or starting for the first time
	int remainingTime; // time remaining after process is executed partially

	Process(char iuserID, int iprocessNumber, int ireadyTime, int iserviceTime) { // default constructor
		userID = iuserID; // user id
		processNumber = iprocessNumber; // process number
		readyTime = ireadyTime; // ready time
		serviceTime = iserviceTime; // service time
		remainingTime = iserviceTime; // remaining time starts at maximum (which is service time)
		isStarted = false; // not started initially
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

	ifstream inputFile("input.txt"); // Open input file

	if (!inputFile) {	// If input file does not exist, show error
		cerr << "Error opening input file";
		return 1;
	}

	int quantum; // time quantum

	vector<Process> Processes; // vector of all existing processes

	vector<char> Users; // vector of all existing users

	string value; // value that will be read from input file (as string)
	char valChar; // value converted to character if string has length of 1

	string amountProcessesString = "0"; // read value of amount of processes (for each user) as string
	int amountProcesses = 0; // amount of processes converted to int

	string readyTimeString; // read value of ready time (for each process) as string
	int readyTime; // ready time converted to int

	string serviceTimeString; // read value of service time (for each process) as string
	int serviceTime; // service time converted to int

	inputFile >> quantum; // read quantum from input file (will always be first value if input file is set up properly)

	cout << "Quantum: " << quantum << "\n"; // display quantum

	char userID = NULL; // userID character


	// loop that reads processes from each user
	while (inputFile >> value) { // read value from file
		valChar = value[0]; // since first value is user ID as one letter, convert to char
		if (static_cast<int>(valChar) >= 65 && static_cast<int>(valChar) <= 90) { // verify the character is a letter
			userID = valChar; // store character in userID
			Users.push_back(userID); // add user to vector
			inputFile >> amountProcessesString; // read next value (amount of processes for user)
			amountProcesses = stoi(amountProcessesString); // convert to int and store
			for (int i = 0; i < amountProcesses; i++) { // loop through each process in order to store ready/service times
				for (int j = 0; j <= 2; j++) { // iterate 3 times per process to store ready/service times then store process
					if (j == 0) { // first store ready time
						inputFile >> readyTimeString; // read from file (as string)
						readyTime = stoi(readyTimeString); // convert to int and store
					}
					else if (j == 1) { // second store service time
						inputFile >> serviceTimeString; // read from file (as string)
						serviceTime = stoi(serviceTimeString); // convert to int and store
					}
					else { // third store process
						Process process(userID, i, readyTime, serviceTime); // create instance of process class with associated user id, process number, ready time, service time
						Processes.push_back(process); // store in process vector
					}
				}
			}
		}

	}

	inputFile.close(); // close input file when finished reading

	for (int i = 0; i < Processes.size(); i++) { // display what was read from input file
		cout << "User: " << Processes[i].userID << " | " << "Number: " << Processes[i].processNumber << " | " << "Ready Time: " << Processes[i].readyTime << " | " << "Service Time: " << Processes[i].serviceTime << "\n";
	}

	thread schedulingThread(scheduleProcesses, std::ref(Processes), std::ref(Users), quantum); // start the scheduler
	schedulingThread.join();

}
