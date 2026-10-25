#pragma once
// Стратегии выбора следующего потока.
// FirstAlive — этап 1, Replay — этап 2, RandomPick — этап 3, Dfs — этап 4 (ограничение bound — этап 5).
// enabled — номера потоков, которые могут сделать шаг, по возрастанию; last — прошлый выбранный (-1 в начале).
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "scheduler.hpp"
#include "strategy.hpp"

namespace sched {

// Всегда первый поток из enabled.
struct FirstAlive : Strategy {
    char pick(const std::vector<char> &enabled, char) override {
        return enabled.at(0);
    }
};

// Проигрывает трассу: на i-м решении выбирает поток trace[i] ('A' -> 0, 'B' -> 1).
struct Replay : Strategy {
    std::string trace;
    size_t index;
    explicit Replay(std::string new_trace) : trace(new_trace), index(0) {}
    // Сбросить позицию в трассе перед прогоном.
    void begin_run() override {
        index = 0;
    }
    char pick(const std::vector<char>&, char) override {
        if (index >= trace.size()) {
            std::cout << "Replay::pick: index exceeds trace." << std::endl;
        }
        return trace.at(index++);
    }
};

// Случайный поток: enabled[rnd() % enabled.size()], rnd — std::mt19937(seed).
struct RandomPick : Strategy {
    std::mt19937 engine;
    explicit RandomPick(uint32_t seed) {
        engine.seed(seed);
    }
    char pick(const std::vector<char>& enabled, char) override {
        return enabled.at(engine() % enabled.size());
    }
};

// Stateless DFS по дереву выборов: каждый прогон с нуля, повторяет префикс прошлого и берёт следующий вариант.
// bound — максимум вытеснений (этап 5); для dfs runner передаёт INT_MAX.
struct Dfs : Strategy {

    struct dfs_tree_node {
        char id;
        bool visited;
        std::vector<std::shared_ptr<dfs_tree_node>> children;
        dfs_tree_node* parent;

        void traverse (const std::string &trace) const {
            const std::string new_trace = trace + id;
            std::cout << new_trace << std::endl;
            for (const auto child : children) {
                child->traverse(new_trace);
            }
        }
    };

    int counter = 0;
    int bound;
    std::shared_ptr<dfs_tree_node> root = std::make_shared<dfs_tree_node>(0, false, std::vector<std::shared_ptr<dfs_tree_node>>(), nullptr);
    std::shared_ptr<dfs_tree_node> current;

    explicit Dfs(int bound) : bound(bound) {}
    // Подготовиться к новому прогону (позиция в пути, счётчик вытеснений).
    void begin_run() override {
        counter = 0;
        current = root;
    }
    char pick(const std::vector<char>& enabled, char last) override {
        if (current->children.size() == 0) {
            for (auto id : enabled) {
                if (counter < bound || id == last || last == -1 || unfinished == 1) {
                    current->children.push_back(std::make_shared<dfs_tree_node>(
                        id, false, std::vector<std::shared_ptr<dfs_tree_node>>(), current.get()
                        ));
                }
            }
        }
        if (current->children.size() == 0) {
            current->visited = true;
            dfs_tree_node* runner = current->parent;
            while (runner) {
                bool all_visited = true;
                for (const auto &child : runner->children) {
                    if (!child->visited) {
                        all_visited = false;
                        break;
                    }
                }
                if (all_visited) {
                    runner->visited = true;
                    runner = runner->parent;
                } else {
                    break;
                }
            }
            return -1;
        }
        char result = -1;
        for (const auto &child : current->children) {
            if (!child->visited) {
                result = child->id;
                current = child;
                if (child->id != last && last != -1)
                    counter++;
                break;
            }
        }
        if (result == -1) {
            if (current->children.size() == 1) {
                result = current->children.front()->id;
            } else {
                for (const auto id : enabled) {
                    if (id == last) {
                        result = last;
                        break;
                    }
                }
            }
            current->visited = true;
            dfs_tree_node* runner = current->parent;
            while (runner) {
                bool all_visited = true;
                for (const auto &child : runner->children) {
                    if (!child->visited) {
                        all_visited = false;
                        break;
                    }
                }
                if (all_visited) {
                    runner->visited = true;
                    runner = runner->parent;
                } else {
                    break;
                }
            }
        }
        return result;
    }
    // Выбрать следующий путь; false — дерево обойдено.
    bool end_run() override {
        current->visited = true;
        dfs_tree_node* runner = current->parent;
        while (runner) {
            bool all_visited = true;
            for (const auto &child : runner->children) {
                if (!child->visited) {
                    all_visited = false;
                    break;
                }
            }
            if (all_visited) {
                runner->visited = true;
                runner = runner->parent;
            } else {
                break;
            }
        }
        if (root->visited) {
            return false;
        }
        return true;
    }
};

}  // namespace sched
