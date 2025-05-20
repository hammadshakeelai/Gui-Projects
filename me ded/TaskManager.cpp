// -----------------------------
// File: TaskManager.cpp
// Implementation of TaskManager methods
// -----------------------------
#include "TaskManager.h"
#include <fstream>

void TaskManager::addTask(const std::wstring& task) {
    tasks.push_back({task, false});
}

void TaskManager::removeTask(int index) {
    if (index >= 0 && index < static_cast<int>(tasks.size())) {
        tasks.erase(tasks.begin() + index);
    }
}

void TaskManager::toggleTask(int index) {
    if (index >= 0 && index < static_cast<int>(tasks.size())) {
        tasks[index].done = !tasks[index].done;
    }
}

int TaskManager::getTaskCount() const {
    return static_cast<int>(tasks.size());
}

std::wstring TaskManager::getTask(int index) const {
    if (index >= 0 && index < static_cast<int>(tasks.size())) {
        return tasks[index].text;
    }
    return L"";
}

bool TaskManager::isCompleted(int index) const {
    if (index >= 0 && index < static_cast<int>(tasks.size())) {
        return tasks[index].done;
    }
    return false;
}

void TaskManager::saveToFile() const {
    std::wofstream file(L"tasks.txt");
    for (const auto& t : tasks) {
        file << (t.done ? L"1" : L"0") << L"|" << t.text << L"\n";
    }
}

void TaskManager::loadFromFile() {
    tasks.clear();
    std::wifstream file(L"tasks.txt");
    std::wstring line;
    while (std::getline(file, line)) {
        if (line.size() < 2) continue;
        bool done = (line[0] == L'1');
        std::wstring text = line.substr(2);
        tasks.push_back({text, done});
    }
}