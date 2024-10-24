#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>
#include <deque>
#include <queue>
struct Waiting_Run{
    IRunnable* runnable;
    int num_total_tasks;
    TaskID dependency;
    TaskID myID;

    Waiting_Run();
    Waiting_Run(IRunnable* r, int t, TaskID d, TaskID m):
        runnable(r), num_total_tasks(t), dependency(d), myID(m)
        {};
};

struct Executing_Run{
    IRunnable* runnable;
    int num_total_tasks;
    int count;
    TaskID myID;
    int tasks_finished;
    int num_of_threads;

    Executing_Run() {};
    Executing_Run(IRunnable* r, int t, int c, TaskID m):
        runnable(r), num_total_tasks(t), count(c), myID(m), tasks_finished(t), num_of_threads(0)
        {};
};

struct Task_Element{
    IRunnable* runnable;
    int num_total_tasks;
    int count;
    TaskID myID;
    int tasks_finished;
    int num_of_threads;
    Task_Element(): myID(-1) {};
    Task_Element(IRunnable* r, int t, int c, TaskID m):
        runnable(r), num_total_tasks(t), count(c), myID(m), tasks_finished(t), num_of_threads(0)
        {};
};

class TaskManager{
public:
    std::mutex* waiting_mutex;
    std::unordered_map<TaskID,std::unordered_set<TaskID>> dependency_map; // who is dependent on [x]
    std::unordered_map<TaskID, int> dependency_count; // reporpose
    std::unordered_map<TaskID, Task_Element> waiting_vec;

    std::mutex* executing_mutex;
    std::queue<Task_Element> executing_vec;

    std::mutex* run_finished;
    std::unordered_set<TaskID> runs_finished;

    std::mutex* task_finished;
    std::unordered_map<TaskID, int> working_rn; // reporpose

    std::mutex* sleeping_mutex;
    int latest_finished;
    std::condition_variable* finishing;

    std::condition_variable* sleeping_thread;
    TaskManager(){
        latest_finished = -1;
        finishing = new std::condition_variable();
        waiting_mutex = new std::mutex();
        task_finished = new std::mutex();
        run_finished = new std::mutex();
        executing_mutex = new std::mutex();
        sleeping_mutex = new std::mutex();
        sleeping_thread = new std::condition_variable();
    }
    ~TaskManager(){
        delete sleeping_mutex;
        delete sleeping_thread;
        delete waiting_mutex;
        delete executing_mutex;
        delete finishing;
        delete task_finished;
        delete run_finished;
    }

    Task_Element fetch_work(){
        while (!executing_vec.empty()) {
            Task_Element elem = executing_vec.front();
            if(elem.count<elem.num_total_tasks){
                executing_vec.front().count++;
                return elem;
            }
            executing_vec.pop();
        }
        return Task_Element();
    }
};

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void worker_thread();
        void sync();

        TaskManager* thread_manager;
        TaskID curr_run_id;

        int num_of_threads;
        bool work_exists;
        std::thread* thread_pool;
};

#endif
