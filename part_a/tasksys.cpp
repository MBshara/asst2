#include "tasksys.h"
#include <thread>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() { //printf("%s deleting\n", name());
}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    TaskSystemParallelSpawn::num_threads = num_threads;
    threads = new std::thread[num_threads];
    global_mutex = new std::mutex();
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    //printf("%s deleting\n", name());
    delete global_mutex;
    delete[] threads;
}

void TaskSystemParallelSpawn::run_thread_static(IRunnable* runnable, int step, int start, int num_total_tasks) {
    for (int i = start; i < step+start; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}
void TaskSystemParallelSpawn::run_thread_dynamic(IRunnable* runnable, int* count, int num_total_tasks) {
    int index = -1;
    while(index < num_total_tasks){
        global_mutex->lock();
        index = *count;
        *count += 1;
        global_mutex->unlock();
        if (index >=num_total_tasks){
            break;
        }
        runnable->runTask(index, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    int* count = new int;
    *count = 0;
    //int step = num_total_tasks / num_threads;
    // if (step<1){
    //     step = 1;
    // }
    for (int i = 0; i < num_threads; i++) {
        //threads[i] = std::thread(&TaskSystemParallelSpawn::run_thread_static, this, runnable, step, step*i, num_total_tasks);
        threads[i] = std::thread(&TaskSystemParallelSpawn::run_thread_dynamic, this, runnable, count, num_total_tasks);
    }
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    delete count;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

    TaskSystemParallelThreadPoolSpinning::num_threads = num_threads;
    thread_pool = new std::thread[num_threads];
    condition_variable = new std::condition_variable();
    mutex = new std::mutex();
    mutex2 = new std::mutex();
    task_mutex = new std::mutex();
    tasks_left = new int;
    *tasks_left = -1;
    total_tasks = -1;
    count = new int;
    *count = 0;
    work_exists = true;
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::run_polling, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    //printf("waiting_to_join"); //DEBUG
    work_exists = false;
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i].join();
    }
    //printf("joined"); //DEBUG
    delete[] thread_pool;
    delete tasks_left;
    delete condition_variable;
    delete count;
    delete mutex;
    delete mutex2;
    delete task_mutex;
}

void TaskSystemParallelThreadPoolSpinning::run_polling(){
    int index = -1;
    int curr_total = -1;
    while(work_exists){ // While the object still exists
        mutex2->lock(); // Something needs to be running or else work_exists doesn't update properly and thread never joins
        index = *count;  // Obtain the current count value
        *count += 1; 
        curr_total = total_tasks;
        if(*count>curr_total){ // If overflow occurs fix that
            *count -= 1;
        }
        mutex2->unlock();
        if(index<curr_total){ // If the index "can be run"
            // then access that work
            running->runTask(index,curr_total);  // running that work
            task_mutex->lock(); 
            *tasks_left-=1; //Update the amount of tasks left to do
            if(*tasks_left == 0){ // If no tasks are left, ONLY RUNS ONCE
                task_mutex->unlock();
                {
                    std::lock_guard<std::mutex> lk(*mutex); // make sure that thread 1 is sleeping
                } // Unlocks the lock
                condition_variable->notify_all(); // notify thread 1 to wake up 
            }
            else{
                task_mutex->unlock();
            }
        }
    }
}
void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*mutex);
    task_mutex -> lock();
    *tasks_left = num_total_tasks;
    task_mutex -> unlock();
    mutex2->lock();
    running = runnable;
    *count = 0;
    total_tasks = num_total_tasks;
    mutex2->unlock();
    //printf("Entered\n"); //DEBUG
    condition_variable->wait(lk);
    //printf("Exited\n"); //DEBUG
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    num_of_threads = num_threads;
    total_tasks = 0;
    work_exists = true;
    tasks_left = new int;
    *tasks_left = -1;
    count = new int;
    *count = 0;
    finish_signal = new std::condition_variable();
    wake_signal = new std::condition_variable();
    done = new std::mutex();
    alarm = new std::mutex();
    global_mutex = new std::mutex();
    thread_pool = new std::thread[num_of_threads];

    for(int i = 0; i<num_of_threads; i++){
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::run_sleeping, this);
    }

}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    work_exists = false;
    {
         std::unique_lock<std::mutex> lock(*global_mutex);
         total_tasks = -1;
    }
    wake_signal->notify_all();
    for(int i = 0; i<num_of_threads; i++){
        thread_pool[i].join();
    }
    delete[] thread_pool;
    delete count;
    delete alarm;
    delete done;
    delete tasks_left;
    delete finish_signal;
    delete wake_signal;
    delete global_mutex;
}

void TaskSystemParallelThreadPoolSleeping::run_sleeping() {
    // While the object still exists
    int index = -1;
    int curr_total = -1;
    while(work_exists){
        // Obtain unit of work to do
        //global_mutex->lock();
        {
            std::unique_lock<std::mutex> lock(*global_mutex);
            if(*count==total_tasks){// Check if that unit of work is doable
                wake_signal->wait(lock); //sleeo
            }
            index = *count;
            *count += 1;
            curr_total = total_tasks;
            // if(*count>total_tasks){// Check if that unit of work is doable
            //     *count -= 1;
            //     wake_signal->wait(lock); //sleeo
            // }
        }
        //global_mutex->unlock();
        if(index<curr_total){
            running->runTask(index,curr_total);  // running that work
            alarm->lock(); 
            *tasks_left-=1; //Update the amount of tasks left to do
            if(*tasks_left == 0){ // If no tasks are left, ONLY RUNS ONCE
                alarm->unlock();
                {
                    std::lock_guard<std::mutex> lk2(*done); // make sure that thread 1 is sleeping
                } // Unlocks the lock
                finish_signal->notify_one(); // notify thread 1 to wake up 
            }
            else{
                alarm->unlock();
            }
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*done);
    alarm->lock();
    *tasks_left = num_total_tasks;
    alarm->unlock();

    global_mutex->lock();
    running = runnable;
    *count = 0;
    total_tasks = num_total_tasks;
    global_mutex->unlock();
    
    wake_signal->notify_all();
    finish_signal->wait(lk);
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
