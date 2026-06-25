#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <algorithm>
using namespace std;

class CacheLevel {
public:
    string name;
    int capacity;
    int latency;
    deque<string> blocks; 
    
    CacheLevel(string n, int c, int l) : name(n), capacity(c), latency(l) {
    }
    
    bool contains(string block) {
        for (int i = 0; i < (int)blocks.size(); i++) {
            if (blocks[i] == block) {
                return true;
            }
        }
        return false;
    }
    
    string promote(string block) {
        string evicted = "";
        int currentSize = blocks.size();

        if (currentSize >= capacity) {
            evicted = blocks.front();
            blocks.pop_front();
        }

        string temp = block;
        blocks.push_back(temp);

        return evicted;
    }
    
    string to_string() {
        string s = "";
        s += "[";

        for (int i = 0; i < (int)blocks.size(); i++) {
            s = s + blocks[i];

            if (i != (int)blocks.size() - 1) {
                s = s + ", ";
            }
        }

        s += "]";
        return s;
    }
};

class MemoryHierarchy {
public:
    CacheLevel L1;
    CacheLevel L2;
    CacheLevel L3;
    int ram_accesses;
    
    MemoryHierarchy() 
        : L1("L1", 32, 4), L2("L2", 128, 12), L3("L3", 512, 40), ram_accesses(0) {
    }
        
    int access(const string& block) {
        if (L1.contains(block)) {
            cout << "  L1: " << L1.to_string() << " -> HIT (" << L1.latency << " cycles)\n";
            return L1.latency;
        }
        cout << "  L1: " << L1.to_string() << " >> MISS\n";
        
        if (L2.contains(block)) {
            cout << "  L2: " << L2.to_string() << " -> HIT (" << L2.latency << " cycles)\n";
            promote_to_l1(block);
            return L2.latency;
        }
        cout << "  L2: " << L2.to_string() << " >> MISS\n";
        
        if (L3.contains(block)) {
            cout << "  L3: " << L3.to_string() << " -> HIT (" << L3.latency << " cycles)\n";
            promote_to_l1(block);
            return L3.latency;
        }
        cout << "  L3: " << L3.to_string() << " >> MISS\n";
        
        cout << "  Fetching from RAM (200 cycles)\n";
        ram_accesses++;
        promote_to_l1(block);
        return 200;
    }
    
private:
    void promote_to_l1(const string& block) {
        cout << "  Promoting " << block << " -> L1\n";
        string evicted_l1 = L1.promote(block);
        
        if (!evicted_l1.empty()) {
            cout << "  (" << evicted_l1 << " evicted from L1 -> dropping to L2)\n";
            string evicted_l2 = L2.promote(evicted_l1);
            
            if (!evicted_l2.empty()) {
                cout << "  (" << evicted_l2 << " evicted from L2 -> dropping to L3)\n";
                L3.promote(evicted_l2); 
            }
        }
        cout << "  L1 State: " << L1.to_string() << "\n";
    }
};

struct Task {
    string id;
    int remaining_burst;
    queue<string> mem_requests;
};

class Simulator {
private:
    MemoryHierarchy mem;
    queue<Task> ready_queue;
    int quantum;
    int total_cycles;
    int tasks_completed;
    int sim_step;
    
public:
    Simulator(int q) : quantum(q), total_cycles(0), tasks_completed(0), sim_step(1) {
    }
    void load_tasks(const string& filename) {
        ifstream file(filename);
        string line;
        if (!file.is_open()) {
            cerr << "Error: Could not open " << filename << "\n";
            exit(1);
        }
        while (getline(file, line)) {
            stringstream ss(line);
            string token;
            Task t;   
            ss >> token >> t.id; 
            ss >> token >> t.remaining_burst; 
            ss >> token;
            while (ss >> token) {
                t.mem_requests.push(token);
            }
            ready_queue.push(t);
        }
    }

    void run() {
        cout << "=== Starting Simulation ===\n";
        while (!ready_queue.empty()) {
            Task current = ready_queue.front();
            ready_queue.pop();

            int executed = 0;
            while (executed < quantum && current.remaining_burst > 0) {
                cout << "\nCycle " << sim_step++ << " - Running: " << current.id;
                if (!current.mem_requests.empty()) {
                    string block = current.mem_requests.front();
                    current.mem_requests.pop();
                    cout << " Requesting: " << block << "\n";
                    total_cycles += mem.access(block);
                } else {
                    cout << " (CPU Execution Only)\n";
                    total_cycles += 1;
                }
                executed++;
                current.remaining_burst--;
            }
            if (current.remaining_burst > 0) {
                ready_queue.push(current);
            } else {
                tasks_completed++;
            }
        }
        print_summary();
    }
    
    void print_summary() {
        cout << "\n=== Final Results ===\n";
        cout << "Total Cycles: " << total_cycles << "\n";
        cout << "Tasks Completed: " << tasks_completed << "\n";
        cout << "Scheduler: Round Robin (quantum = " << quantum << ")\n";
        cout << "RAM Accesses: " << mem.ram_accesses << "\n";
    }
};

int main() {
    Simulator sys(3); 
    sys.load_tasks("tasks.txt");
    sys.run();
    return 0;
}