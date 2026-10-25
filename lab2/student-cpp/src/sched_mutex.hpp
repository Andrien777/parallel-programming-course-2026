#pragma once
// Этап 2: лок, о котором знает планировщик. Поток, ждущий занятый лок, не попадает в enabled;
// обычный std::mutex здесь нельзя — поток уснёт с ходом в руках и всё зависнет.
// Лог: "lock <имя>", "unlock <имя>". Планировщик должен знать все локи, чтобы печатать held={a:A, b:B}.
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>

#include "scheduler.hpp"

namespace sched {

class sched_mutex {
public:
    // Создаётся в setup() тестовой программы; зарегистрируйте лок в планировщике s.
    sched_mutex(Scheduler& s, std::string name) : name(std::move(name)) {
        held_by = 0;
        s.mutexes.push_back(this);
    }
    sched_mutex(const sched_mutex&) = delete;
    char held_by;

    // Пауза; если лок занят — ждать, не участвуя в выборе; затем стать владельцем.
    void lock() {
        point(true, true);
        if (held_by) {
            await([this] {return this->held_by == 0;});
        }
        if (ctx->scheduler->logging) {
            ctx->scheduler->log.push_back(std::format("{} lock {}", ctx->id, name));
        }
        held_by = ctx->id;
    }
    // Пауза; освободить лок, ждавшие потоки снова могут быть выбраны.
    void unlock() {
        if (!ctx->deadlocked) {
            point(true, true);
            if (ctx->scheduler->logging) {
                ctx->scheduler->log.push_back(std::format("{} unlock {}", ctx->id, name));
            }
        }
        held_by = 0;
    }

    const std::string name;
};

// RAII: lock сейчас, unlock на закрывающей скобке. Выдан готовым.
class sched_guard {
public:
    explicit sched_guard(sched_mutex& m) : m_(m) { m_.lock(); }
    ~sched_guard() noexcept(false) { m_.unlock(); }
    sched_guard(const sched_guard&) = delete;

private:
    sched_mutex& m_;
};

}  // namespace sched
