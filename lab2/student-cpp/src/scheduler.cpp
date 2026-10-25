// Этап 1–2: реализация Scheduler, point() и await().
// Сейчас все методы бросают исключение «TODO»: заполните их по заданию этапов 1 и 2.
// Деструктор должен дождаться (join) всех запущенных потоков, в том числе после deadlock.
#include "scheduler.hpp"

#include <format>

#include "sched_mutex.hpp"

namespace sched {

Scheduler::Scheduler() {
    unfinished = 0;
}

Scheduler::~Scheduler() {
}

void Scheduler::spawn(std::function<void()> body) {
    ctx_map.emplace(id_counter, std::make_shared<ThreadContext>(id_counter, this, ThreadState::WAITING, std::make_unique<std::binary_semaphore>(0)));
    threads.emplace(id_counter, std::thread([body](const std::shared_ptr<ThreadContext> &ctx_ptr) {
        ctx = ctx_ptr;
        ctx->sem->acquire();
        try {
            body();
            ctx->state = ThreadState::FINISHED;
            unfinished--;
            ctx->scheduler->paused.release();
        } catch (...) {}
    }, ctx_map.at(id_counter)));
    unfinished++;
    ++id_counter;
}

void Scheduler::run(Strategy& strategy) {
    for (char id = 'A'; id < id_counter; ++id) {
        ctx_map.at(id)->state = ThreadState::ACTIVE;
        ctx_map.at(id)->sem->release();
    }
    char last = -1;
    for (char i = 'A'; i < id_counter - 1; i++) {
        paused.acquire();
    }
    while (true) {
        paused.acquire();
        enabled.clear();
        for (char id = 'A'; id < id_counter; ++id) {
            if (ctx_map.at(id)->state == ThreadState::WAITING) {
                enabled.push_back(id);
            }
        }
        if (enabled.empty()) {
            break;
        }
        last = strategy.pick(enabled, last);
        if (last == -1) {
            for (char id = 'A'; id < id_counter; ++id) {
                ctx_map.at(id)->deadlocked = true;
                ctx_map.at(id)->sem->release();
                threads.at(id).detach();
            }
            throw std::runtime_error("Scheduler::run(): last not found");
        }
        trace.push_back(last);
        ctx_map.at(last)->state = ThreadState::ACTIVE;
        paused.release();
        ctx_map.at(last)->sem->release();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    bool locked = false;
    std::vector<sched_mutex*> held_mutexes;
    for (auto mutex : mutexes) {
        if (mutex->held_by != 0) {
            locked = true;
            held_mutexes.push_back(mutex);
        }
    }
    if (locked) {
        deadlock = "held={";
        for (size_t i = 0; i < held_mutexes.size() - 1; ++i) {
            deadlock += std::format("{}:{}, ", held_mutexes.at(i)->name, held_mutexes.at(i)->held_by);
        }
        deadlock += std::format("{}:{}", held_mutexes.back()->name, held_mutexes.back()->held_by);
        deadlock += '}';
    }
    for (char id = 'A'; id < id_counter; ++id) {
        if (ctx_map.at(last)->state == ThreadState::FINISHED) {
            threads.at(id).join();
            threads.at(id).~thread();
        } else {
            ctx_map.at(id)->deadlocked = true;
            threads.at(id).join();
            threads.at(id).~thread();
        }
    }
}

std::string Scheduler::trace_string() const {
    return trace;
}

void point(bool actual_lock, bool no_log) {
    if (ctx->scheduler->logging && !no_log) {
        std::string log_line = std::format("{} window", ctx->id);
        if (ctx->scheduler->log.empty() || ctx->scheduler->log.back() != log_line) {
            ctx->scheduler->log.push_back(log_line);
        }
    }

    if (actual_lock) {
        ctx->state = ThreadState::WAITING;
        ctx->scheduler->paused.release();
        ctx->sem->acquire();
    }

    if (ctx->deadlocked) {
        throw std::runtime_error("deadlocked");
    }
}

void await(std::function<bool()> cond) {
    ctx->state = ThreadState::LOCKED;
    if (!cond())
        ctx->scheduler->paused.release();
    while (!cond()) {
        if (ctx->deadlocked) {
            throw std::runtime_error("deadlocked");
        }
    }
    ctx->state = ThreadState::WAITING;
    ctx->scheduler->paused.release();
    ctx->sem->acquire();
    if (ctx->deadlocked) {
        throw std::runtime_error("deadlocked");
    }
    if (ctx->scheduler->logging) {
        std::string log_line = std::format("{} await done", ctx->id);
        if (ctx->scheduler->log.empty() || ctx->scheduler->log.back() != log_line) {
            ctx->scheduler->log.push_back(log_line);
        }
    }
}

}  // namespace sched
