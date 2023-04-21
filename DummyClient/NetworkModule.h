#pragma once

void InitializeNetwork();
void GetPointCloud(int* size, float** points);
void ShutdownNetwork();

extern int global_delay;
extern std::atomic_int active_clients;