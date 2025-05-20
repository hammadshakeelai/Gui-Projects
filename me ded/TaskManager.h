// -----------------------------
// File: TaskManager.h
// OOP logic for managing tasks
// -----------------------------
#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <string>
#include <vector>

class TaskManager {
public:
    // Add a new task (initially not done)
    void addTask(const std::wstring& task);

    // Remove task at given index
    void removeTask(int index);

    // Toggle done/undone status
    void toggleTask(int index);

    // Get number of tasks
    int getTaskCount() const;

    // Get text of task at index
    std::wstring getTask(int index) const;

    // Check if task is completed
    bool isCompleted(int index) const;

    // Load tasks from "tasks.txt"
    void loadFromFile();

    // Save tasks to "tasks.txt"
    void saveToFile() const;

private:
    struct Task {
        std::wstring text;
        bool done;
    };
    std::vector<Task> tasks;
};

#endif