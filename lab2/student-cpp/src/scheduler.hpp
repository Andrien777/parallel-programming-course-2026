#pragma once
// Этап 1: планировщик, который ведёт настоящие потоки строго по одному (передача хода через семафоры).
// Этап 2: обнаружение deadlock в главном цикле и корректное завершение застрявших потоков.
// Внутреннее устройство (поля, семафоры, вспомогательные функции для shared и sched_mutex) придумываете сами.
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <semaphore>

#include "strategy.hpp"

namespace sched {
    class sched_mutex;
    class Scheduler;

enum ThreadState {
    ACTIVE, WAITING, LOCKED, FINISHED
};

struct ThreadContext {
    char id;
    Scheduler *scheduler;
    ThreadState state;
    std::unique_ptr<std::binary_semaphore> sem;
    bool deadlocked = false;
};

inline thread_local std::shared_ptr<ThreadContext> ctx;
inline int unfinished;
class Scheduler {
private:
    std::string trace{""};
    std::vector<char> enabled{};
    std::unordered_map<char, std::thread> threads{};
    std::unordered_map<char, std::shared_ptr<ThreadContext>> ctx_map{};
    char id_counter = 'A';

public:
    Scheduler();
    ~Scheduler();

    std::counting_semaphore<256> paused{0};
    std::vector<sched_mutex*> mutexes{};
    // Зарегистрировать поток. Потоки получают буквы A, B, C… в порядке вызова spawn.
    void spawn(std::function<void()> body);
    // Запустить потоки и вести их по одному: на каждой паузе спросить strategy.pick(enabled, last).
    void run(Strategy& strategy);
    // Трасса прогона: буквы потоков, выбранных во всех решениях подряд, например "AAAABB".
    std::string trace_string() const;

    // Если true, каждая операция с общими данными добавляет строку в log. Выставляет runner.
    bool logging = false;
    // Лог прогона: строки "<поток>  <операция>", например "A  lock mu", "A  balance -> 100".
    std::vector<std::string> log;
    // Пустая строка, если deadlock не было; иначе "held={a:A, b:B}" (этап 2).
    std::string deadlock;
};

// window() из тестовых программ: пауза перед следующей операцией с общими данными обязательна.
void point(bool actual_lock = false, bool no_log = false);
// Ждать, пока cond не станет true. До этого поток не попадает в enabled.
void await(std::function<bool()> cond);

}  // namespace sched
