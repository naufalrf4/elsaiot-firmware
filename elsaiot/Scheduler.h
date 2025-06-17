#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

#define MAX_TASKS 8

struct Task {
  String name;
  uint32_t interval;
  uint32_t lastRun;
  void (*callback)();
  bool enabled;
};

Task tasks[MAX_TASKS];
int taskCount = 0;

void initScheduler() {
  taskCount = 0;
  Serial.println("📋 Task scheduler initialized");
}

bool addTask(const String& name, uint32_t intervalMs, void (*callback)()) {
  if (taskCount >= MAX_TASKS) {
    Serial.println("❌ Maximum tasks reached!");
    return false;
  }
  
  tasks[taskCount].name = name;
  tasks[taskCount].interval = intervalMs;
  tasks[taskCount].lastRun = 0;
  tasks[taskCount].callback = callback;
  tasks[taskCount].enabled = true;
  taskCount++;
  
  Serial.printf("✅ Task '%s' added (interval: %d ms)\n", name.c_str(), intervalMs);
  return true;
}

void runTasks() {
  uint32_t now = millis();
  
  for (int i = 0; i < taskCount; i++) {
    if (!tasks[i].enabled) continue;
    
    if (now - tasks[i].lastRun >= tasks[i].interval) {
      tasks[i].lastRun = now;
      tasks[i].callback();
      
      yield();
    }
  }
}

void enableTask(const String& name) {
  for (int i = 0; i < taskCount; i++) {
    if (tasks[i].name == name) {
      tasks[i].enabled = true;
      Serial.printf("✅ Task '%s' enabled\n", name.c_str());
      return;
    }
  }
}

void disableTask(const String& name) {
  for (int i = 0; i < taskCount; i++) {
    if (tasks[i].name == name) {
      tasks[i].enabled = false;
      Serial.printf("⏸️  Task '%s' disabled\n", name.c_str());
      return;
    }
  }
}

void listTasks() {
  Serial.println("📋 Active Tasks:");
  for (int i = 0; i < taskCount; i++) {
    Serial.printf("  %d. %s - %d ms - %s\n", 
                  i + 1, 
                  tasks[i].name.c_str(), 
                  tasks[i].interval,
                  tasks[i].enabled ? "ENABLED" : "DISABLED");
  }
}

#endif