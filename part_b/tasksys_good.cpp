#include "tasksys.h"


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

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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
    curr_run_id = 0;
    latest_finished = -1;
    num_of_threads = num_threads;
    work_exists = true;
    waiting_mutex = new std::mutex();
    executing_mutex = new std::mutex();
    finished = new std::mutex();
    waiting_sleep = new std::condition_variable();
    syncing_sleep = new std::condition_variable();
    thread_pool = new std::thread[num_threads];
    for(int i = 0; i<num_threads; i++){
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::worker_thread, this);
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
    //sync();
    {
        std::lock_guard<std::mutex> lock(*waiting_mutex);
        waiting_sleep->notify_all();
    }
    for(int i = 0; i<num_of_threads; i++){
        thread_pool[i].join();
    }
    delete[] thread_pool;
    delete executing_mutex;
    delete waiting_mutex;
    delete waiting_sleep;
    delete syncing_sleep;
    delete finished;
}
Executing_Run* TaskSystemParallelThreadPoolSleeping::update_executing_vec(){
    for (auto it = executing_vec.begin(); it != executing_vec.end(); ) {
        if ((*it)->count >= (*it)->num_total_tasks) {
            // Remove the element and get the iterator to the next element.
            it = executing_vec.erase(it);
        } else {
            return *it;
        }
    }
    return nullptr;
}
void TaskSystemParallelThreadPoolSleeping::worker_thread(){
    Executing_Run* local_pt = nullptr;
    bool killed = true;
    Executing_Run local(nullptr,-1,-1,-1);
    while(work_exists){
        if(killed){
            executing_mutex->lock();
            if(!executing_vec.empty()){
                local_pt = update_executing_vec();
                if(local_pt!=nullptr){
                    killed = false;
                    local = *local_pt;
                    local_pt->count++;
                    local_pt->num_of_threads++;
                }
                executing_mutex->unlock();
            }
            else{
                std::unique_lock<std::mutex> lock(*waiting_mutex);
                if(!waiting_vec.empty()){
                    for(auto it = waiting_vec.begin(); it != waiting_vec.end();){
                        if(it->dependency<=latest_finished){
                            executing_vec.push_back(new Executing_Run(it->runnable,it->num_total_tasks,0,it->myID));
                            waiting_vec.erase(it);
                        }
                        else{
                            break;
                        }
                    }
                    lock.unlock();
                    if(!executing_vec.empty()){
                        killed = false;
                        local_pt = executing_vec.front();
                        local = *local_pt;
                        local_pt->count++;
                        local_pt->num_of_threads++;
                    }
                    executing_mutex->unlock();
                }
                else{
                    executing_mutex->unlock();
                    if(work_exists){
                        waiting_sleep->wait(lock);
                    }
                }
            }
        }
        if(!killed){
            local.runnable->runTask(local.count, local.num_total_tasks);
            killed = false;
            finished->lock();
            local_pt->tasks_finished--;
            if(local_pt->tasks_finished == 0){
                latest_finished=latest_finished+1;
                if(curr_run_id==latest_finished+1){
                    syncing_sleep->notify_one();
                }
            }
            finished->unlock();
            executing_mutex->lock();
            if(local_pt->count < local_pt->num_total_tasks){
                local = *local_pt;
                local_pt->count++;
                executing_mutex->unlock();
            }
            else{
                local_pt->num_of_threads--;
                if(local_pt->num_of_threads==0){
                    executing_mutex->unlock();
                    delete local_pt;
                }
                else{
                    executing_mutex->unlock();
                }
                killed = true;
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
    std::vector<TaskID> noDeps;
    runAsyncWithDeps(runnable, num_total_tasks, noDeps);
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    TaskID latest_dependency = -1;
    //bool local_thread_sleeping = false;
    if(!deps.empty()){
        latest_dependency = *std::max_element(deps.begin(),deps.end());
    }
    Waiting_Run task(runnable,num_total_tasks,latest_dependency,curr_run_id);
    waiting_mutex->lock();
    waiting_vec.push_back(task);
    waiting_mutex->unlock();
    waiting_sleep->notify_all();


    return curr_run_id++;
}


void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    std::unique_lock<std::mutex> lock(*finished);
    if(latest_finished + 1 == curr_run_id){
        return;
    }
    else{
        syncing_sleep->wait(lock);
    }
    return;
}
