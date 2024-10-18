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

        std::mutex* waiting_mutex;
        TaskID curr_run_id;
        std::vector<Waiting_Run> waiting_vec;
        std::condition_variable* waiting_sleep;
        std::vector<Executing_Run*> executing_vec;

        int num_of_threads;
        bool work_exists;
        std::thread* thread_pool;

        std::mutex* executing_mutex;

        TaskID latest_finished;
        std::mutex* finished;
        std::condition_variable* syncing_sleep;
        bool thread_sleeping;


        Executing_Run* update_executing_vec();
};

#endif
