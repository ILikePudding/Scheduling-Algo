#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>
#include <random>

struct Task {
    int id;
    int deadline;
    int executionTime;
    std::vector<int> dependencies;
};

struct CompareDeadline {
    bool operator()(const Task& t1, const Task& t2) {
        return t1.deadline > t2.deadline;
    }
};

struct Processor {
    int id;
    std::vector<int> availableTime;
    int totalExecutionTime = 0;
};

std::vector<int> computeUpwardRanks(const std::vector<Task>& tasks) {
    std::vector<int> upwardRanks(tasks.size());

    // Initialize upward ranks with execution times
    for (int i = 0; i < tasks.size(); i++) {
        upwardRanks[i] = tasks[i].executionTime;
    }

    // Traverse tasks and update upward ranks
    for (int i = 0; i < tasks.size(); i++) {
        for (int pred : tasks[i].dependencies) {
            upwardRanks[i] = std::max(upwardRanks[i], upwardRanks[pred] + tasks[i].executionTime);
        }
    }

    return upwardRanks;
}

std::vector<int> computeDownwardRanks(const std::vector<Task>& tasks) {
    std::vector<int> downwardRanks(tasks.size());

    // Initialize downward ranks with execution times
    for (int i = 0; i < tasks.size(); i++) {
        downwardRanks[i] = tasks[i].executionTime;
    }

    // Traverse tasks in reverse order and update downward ranks
    for (int i = tasks.size() - 1; i >= 0; i--) {
        for (int j = 0; j < tasks.size(); j++) {
            if (std::find(tasks[j].dependencies.begin(), tasks[j].dependencies.end(), i) != tasks[j].dependencies.end()) {
                downwardRanks[i] = std::max(downwardRanks[i], downwardRanks[j] + tasks[j].executionTime);
            }
        }
    }

    return downwardRanks;
}

int scheduleD_EDF(std::vector<Task>& tasks, int numProcessors, std::vector<Processor>& processors) {
    int successfullyScheduledTasks = 0;
    std::priority_queue<Task, std::vector<Task>, CompareDeadline> readyQueue;

    for (int i = 0; i < numProcessors; i++) {
        processors.push_back({i, std::vector<int>(tasks.size(), 0), 0});
    }

    int missedDeadlines = 0;
    bool useDM = false;

    // Sort tasks by deadline (for DM)
    std::sort(tasks.begin(), tasks.end(), [](const Task& t1, const Task& t2) {
        return t1.deadline < t2.deadline;
    });

    for (const Task& task : tasks) {
        if (useDM) {
            // Assign task to the processor with the earliest available time
            int processorIndex = std::min_element(processors.begin(), processors.end(), [](const Processor& p1, const Processor& p2) {
                return p1.availableTime[0] < p2.availableTime[0];
            }) - processors.begin();
            int finishTime = processors[processorIndex].availableTime[0] + task.executionTime;
            processors[processorIndex].availableTime[0] = finishTime;
            processors[processorIndex].totalExecutionTime += task.executionTime;
            //std::cout << "Task " << task.id << " assigned to Processor " << processorIndex << " (Finish Time: " << finishTime << ")" << std::endl;
            successfullyScheduledTasks++;
        } else {
            readyQueue.push(task);
            // Check if any processor is available
            int processorIndex = -1;
            int earliestAvailableTime = std::numeric_limits<int>::max();
            for (int i = 0; i < numProcessors; i++) {
                if (processors[i].availableTime[0] < earliestAvailableTime) {
                    processorIndex = i;
                    earliestAvailableTime = processors[i].availableTime[0];
                }
            }
            if (earliestAvailableTime <= task.deadline) {
                // Assign task to the selected processor
                int finishTime = processors[processorIndex].availableTime[0] + task.executionTime;
                processors[processorIndex].availableTime[0] = finishTime;
                processors[processorIndex].totalExecutionTime += task.executionTime;
                readyQueue.pop();
                //std::cout << "Task " << task.id << " assigned to Processor " << processorIndex << " (Finish Time: " << finishTime << ")" << std::endl;
                successfullyScheduledTasks++;
            } else {
                // Increment missed deadlines count
                missedDeadlines++;
                if (missedDeadlines >= 2) {
                    // Switch to DM if two consecutive deadlines are missed
                    useDM = true;
                    missedDeadlines = 0;
                    //std::cout << "Switching to Deadline Monotonic (DM) scheduling" << std::endl;
                }
            }
        }
    }
    return successfullyScheduledTasks;
}

int scheduleHEFT(std::vector<Task>& tasks, int numProcessors, std::vector<Processor>& processors) {
    int successfullyScheduledTasks = 0;

    for (int i = 0; i < numProcessors; i++) {
        processors.push_back({i, std::vector<int>(1, 0), 0});
    }

    // Step 1: Compute upward ranks and downward ranks for each task
    std::vector<int> upwardRanks(tasks.size(), 0);
    std::vector<int> downwardRanks(tasks.size(), 0);

    // Calculate upward ranks
    for (int i = 0; i < tasks.size(); i++) {
        int maxPredecessorRank = 0;
        for (int pred : tasks[i].dependencies) {
            maxPredecessorRank = std::max(maxPredecessorRank, upwardRanks[pred]);
        }
        upwardRanks[i] = tasks[i].executionTime + maxPredecessorRank;
    }

    // Calculate downward ranks
    for (int i = tasks.size() - 1; i >= 0; i--) {
        int maxSuccessorRank = 0;
        for (int j = 0; j < tasks.size(); j++) {
            if (std::find(tasks[j].dependencies.begin(), tasks[j].dependencies.end(), i) != tasks[j].dependencies.end()) {
                maxSuccessorRank = std::max(maxSuccessorRank, downwardRanks[j]);
            }
        }
        downwardRanks[i] = tasks[i].executionTime + maxSuccessorRank;
    }

    // Step 2: Sort tasks in decreasing order of upward ranks
    std::vector<int> sortedTasks(tasks.size());
    for (int i = 0; i < tasks.size(); i++) {
        sortedTasks[i] = i;
    }
    std::sort(sortedTasks.begin(), sortedTasks.end(), [&upwardRanks](int a, int b) {
        return upwardRanks[a] > upwardRanks[b];
    });

    // Step 3: Schedule tasks on processors
    for (int taskIndex : sortedTasks) {
        int minFinishTime = std::numeric_limits<int>::max();
        int bestProcessor = -1;
        for (int p = 0; p < processors.size(); p++) {
            int finishTime = std::max(processors[p].availableTime[0], upwardRanks[taskIndex]) + tasks[taskIndex].executionTime;
            if (finishTime < minFinishTime) {
                minFinishTime = finishTime;
                bestProcessor = p;
            }
        }
        processors[bestProcessor].availableTime[0] = minFinishTime;
        processors[bestProcessor].totalExecutionTime += tasks[taskIndex].executionTime;
        //std::cout << "Task " << tasks[taskIndex].id << " assigned to Processor " << bestProcessor << " (Finish Time: " << minFinishTime << ")" << std::endl;
        successfullyScheduledTasks++;
    }

    return successfullyScheduledTasks;
}

int scheduleCPOP(std::vector<Task>& tasks, int numProcessors, std::vector<Processor>& processors) {
    int successfullyScheduledTasks = 0;

    for (int i = 0; i < numProcessors; i++) {
        processors.push_back({i, std::vector<int>(tasks.size(), 0), 0});
    }

    // Step 1: Compute upward ranks and downward ranks for each task
    std::vector<int> upwardRanks = computeUpwardRanks(tasks);
    std::vector<int> downwardRanks = computeDownwardRanks(tasks);

    /*std::cout << "Upward Ranks:" << std::endl;
    for (int i = 0; i < tasks.size(); i++) {
        std::cout << "Task " << tasks[i].id << ": " << upwardRanks[i] << std::endl;
    }*/

    /*std::cout << "\nDownward Ranks:" << std::endl;
    for (int i = 0; i < tasks.size(); i++) {
        std::cout << "Task " << tasks[i].id << ": " << downwardRanks[i] << std::endl;
    }*/

    //std::cout << std::endl; //better output readability

    // Step 2: Sort tasks by decreasing priorities
    std::vector<int> sortedTasks(tasks.size());
    for (int i = 0; i < tasks.size(); i++) {
        sortedTasks[i] = i;
    }
    std::sort(sortedTasks.begin(), sortedTasks.end(), [&upwardRanks, &downwardRanks](int a, int b) {
        int priorityA = upwardRanks[a] + downwardRanks[a];
        int priorityB = upwardRanks[b] + downwardRanks[b];
        return priorityA > priorityB;
    });

    /*Check Order of tasks
    std::cout << "\nSorted Task Order:" << std::endl;
    for (int taskIndex : sortedTasks) {
        std::cout << "Task " << tasks[taskIndex].id << std::endl;
    }*/

    // Step 3: Schedule tasks on processors
    for (int taskIndex : sortedTasks) {
        int minFinishTime = std::numeric_limits<int>::max();
        int bestProcessor = -1;
        for (int p = 0; p < processors.size(); p++) {
            int finishTime = *std::max_element(processors[p].availableTime.begin(), processors[p].availableTime.end()) + upwardRanks[taskIndex] + tasks[taskIndex].executionTime;
            if (finishTime < minFinishTime) {
                minFinishTime = finishTime;
                bestProcessor = p;
            }
        }
        processors[bestProcessor].availableTime[taskIndex] = minFinishTime;
        processors[bestProcessor].totalExecutionTime += tasks[taskIndex].executionTime;
        //std::cout << "Task " << tasks[taskIndex].id << " assigned to Processor " << bestProcessor << " (Finish Time: " << minFinishTime << ")" << std::endl;
        successfullyScheduledTasks++;
    }

    return successfullyScheduledTasks;
}

// Task set generation functions
std::vector<Task> generateTaskSet(int numTasks, int minExecTime, int maxExecTime, int minDeadline, int maxDeadline, double dependencyProbability) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> execTimeDist(minExecTime, maxExecTime);
    std::uniform_int_distribution<int> deadlineDist(minDeadline, maxDeadline);
    std::bernoulli_distribution dependencyDist(dependencyProbability);

    std::vector<Task> tasks(numTasks);
    for (int i = 0; i < numTasks; i++) {
        tasks[i].id = i + 1;
        tasks[i].executionTime = execTimeDist(gen);
        tasks[i].deadline = deadlineDist(gen);

        for (int j = 0; j < i; j++) {
            if (dependencyDist(gen)) {
                tasks[i].dependencies.push_back(j);
            }
        }
    }

    return tasks;
}

std::vector<std::vector<Task>> generateTaskSets() {
    std::vector<std::vector<Task>> taskSets;

    // task sets with moderate dependencies
    taskSets.push_back(generateTaskSet(5, 1, 10, 5, 20, 0.3));
    taskSets.push_back(generateTaskSet(10, 1, 10, 5, 20, 0.3));
    taskSets.push_back(generateTaskSet(15, 1, 10, 5, 20, 0.3));
    taskSets.push_back(generateTaskSet(20, 1, 10, 5, 20, 0.3));
    taskSets.push_back(generateTaskSet(25, 1, 10, 5, 20, 0.3));
    taskSets.push_back(generateTaskSet(30, 1, 10, 5, 20, 0.3));
    taskSets.push_back(generateTaskSet(40, 1, 10, 5, 20, 0.3));

    // Task sets with skewed execution times
    //taskSets.push_back(generateTaskSet(20, 1, 50, 10, 100, 0.2));
    //taskSets.push_back(generateTaskSet(30, 5, 80, 20, 150, 0.2));
    //taskSets.push_back(generateTaskSet(40, 10, 100, 30, 200, 0.2));

    // Task sets with tight deadlines
    //taskSets.push_back(generateTaskSet(15, 1, 10, 2, 15, 0.1));
    //taskSets.push_back(generateTaskSet(20, 1, 10, 3, 18, 0.1));
    //taskSets.push_back(generateTaskSet(25, 1, 10, 4, 20, 0.1));

    // Task sets with high priorities for CPOP
    //taskSets.push_back(generateTaskSet(20, 1, 10, 10, 50, 0.2));
    //taskSets.push_back(generateTaskSet(25, 1, 10, 15, 60, 0.2));
    //taskSets.push_back(generateTaskSet(30, 1, 10, 20, 70, 0.2));

    // Task sets with low priorities for CPOP
    //taskSets.push_back(generateTaskSet(20, 10, 50, 20, 100, 0.2));
    //taskSets.push_back(generateTaskSet(25, 15, 60, 30, 120, 0.2));
    //taskSets.push_back(generateTaskSet(30, 20, 70, 40, 140, 0.2));

    // Task sets for homogeneous processors
    //taskSets.push_back(generateTaskSet(15, 1, 10, 5, 20, 0.2));
    //taskSets.push_back(generateTaskSet(20, 1, 10, 5, 20, 0.2));
    //taskSets.push_back(generateTaskSet(25, 1, 10, 5, 20, 0.2));

    // Task sets for moderately heterogeneous processors
    //taskSets.push_back(generateTaskSet(15, 1, 20, 5, 40, 0.2));
    //taskSets.push_back(generateTaskSet(20, 1, 20, 5, 40, 0.2));
    //taskSets.push_back(generateTaskSet(25, 1, 20, 5, 40, 0.2));

    // Task sets for highly heterogeneous processors
    //taskSets.push_back(generateTaskSet(15, 1, 30, 5, 60, 0.2));
    //taskSets.push_back(generateTaskSet(20, 1, 30, 5, 60, 0.2));
    //taskSets.push_back(generateTaskSet(25, 1, 30, 5, 60, 0.2));

    return taskSets;
}

int main() {
    int numProcessors = 2;
    std::vector<Processor> processors;

    std::vector<std::vector<Task>> taskSets = generateTaskSets();

    for (const auto& taskSet : taskSets) {
        std::vector<Task> tasks = taskSet;
        int sequentialExecutionTime = 0;
        for (const Task& task : tasks) {
            sequentialExecutionTime += task.executionTime;
        }

        std::cout << "Task Set: ";
        for (const Task& task : tasks) {
            std::cout << "(" << task.id << ", " << task.executionTime << ", " << task.deadline << ") ";
        }
        std::cout << std::endl;

        processors.clear();

        std::cout << "\nD_EDF Scheduling:" << std::endl;
        int successfullyScheduledTasks = scheduleD_EDF(tasks, numProcessors, processors);
        int totalTasks = tasks.size();
        double efficiency = static_cast<double>(successfullyScheduledTasks) / totalTasks;
        std::cout << "Efficiency: " << efficiency << std::endl;

        int parallelExecutionTime = 0;
        for (const Processor& processor : processors) {
            parallelExecutionTime = std::max(parallelExecutionTime, processor.availableTime[0]);
        }
        double speedup = static_cast<double>(sequentialExecutionTime) / parallelExecutionTime;
        std::cout << "Speedup: " << speedup << std::endl;

        double meanExecutionTime = 0;
        for (const Processor& processor : processors) {
            meanExecutionTime += processor.totalExecutionTime;
        }
        meanExecutionTime /= processors.size();

        double variance = 0;
        for (const Processor& processor : processors) {
            variance += pow(processor.totalExecutionTime - meanExecutionTime, 2);
        }
        variance /= processors.size();

        double loadBalancing = sqrt(variance);
        std::cout << "Load Balancing: " << loadBalancing << std::endl;

        processors.clear();

        std::cout << "\nHEFT Scheduling:" << std::endl;
        successfullyScheduledTasks = scheduleHEFT(tasks, numProcessors, processors);
        efficiency = static_cast<double>(successfullyScheduledTasks) / totalTasks;
        std::cout << "Efficiency: " << efficiency << std::endl;

        parallelExecutionTime = 0;
        for (const Processor& processor : processors) {
            parallelExecutionTime = std::max(parallelExecutionTime, processor.availableTime[0]);
        }
        speedup = static_cast<double>(sequentialExecutionTime) / parallelExecutionTime;
        std::cout << "Speedup: " << speedup << std::endl;

        meanExecutionTime = 0;
        for (const Processor& processor : processors) {
            meanExecutionTime += processor.totalExecutionTime;
        }
        meanExecutionTime /= processors.size();

        variance = 0;
        for (const Processor& processor : processors) {
            variance += pow(processor.totalExecutionTime - meanExecutionTime, 2);
        }
        variance /= processors.size();

        loadBalancing = sqrt(variance);
        std::cout << "Load Balancing: " << loadBalancing << std::endl;

        processors.clear();

        std::cout << "\nCPOP Scheduling:" << std::endl;
        successfullyScheduledTasks = scheduleCPOP(tasks, numProcessors, processors);
        efficiency = static_cast<double>(successfullyScheduledTasks) / totalTasks;
        std::cout << "Efficiency: " << efficiency << std::endl;

        parallelExecutionTime = 0;
        for (const Processor& processor : processors) {
            parallelExecutionTime = std::max(parallelExecutionTime, processor.availableTime[0]);
        }
        speedup = static_cast<double>(sequentialExecutionTime) / parallelExecutionTime;
        std::cout << "Speedup: " << speedup << std::endl;

        meanExecutionTime = 0;
        for (const Processor& processor : processors) {
            meanExecutionTime += processor.totalExecutionTime;
        }
        meanExecutionTime /= processors.size();

        variance = 0;
        for (const Processor& processor : processors) {
            variance += pow(processor.totalExecutionTime - meanExecutionTime, 2);
        }
        variance /= processors.size();

        loadBalancing = sqrt(variance);
        std::cout << "Load Balancing: " << loadBalancing << std::endl;

        std::cout << std::endl;
    }

    return 0;
}