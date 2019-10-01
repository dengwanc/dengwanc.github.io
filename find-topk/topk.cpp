
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <list>
#include <iterator>
#include <map>
#include <queue>
#include <sys/stat.h>
#include <ctime>
#include <iomanip>
#include <functional>
#include "topk.h"

using namespace std;

/**
 * Type modules
 */

/// 被分块的文件
struct Reducer {
    string filename;
    unsigned int rows;
};

/// 频次统计信息
template <typename T>
struct FrequencyStats {
    T key;
    unsigned int count;
};

template <typename T>
class Compare {
public: bool operator() (FrequencyStats<T> a, FrequencyStats<T> b) { return a.count > b.count; }
};

/// 最小堆
template<typename T>
using minheap = priority_queue<FrequencyStats<T>, std::vector<FrequencyStats<T>>, Compare<T>>;

/**
 * helper modules
 */
string errmsg(const char *msg, int code)
{
    std::ostringstream oss;
    oss << msg << "::code=" << code;
    return oss.str();
}

long long getFileSize(string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    if (rc == 0) {
        return stat_buf.st_size;
    } else {
        throw std::runtime_error(errmsg("stat error", rc));
    }
};

string genfilename(string org, int lv)
{
    std::ostringstream oss;
    oss << org << "_" << lv;
    return oss.str();
};


/**
 * CONST modules
 */
const auto RECORD_SIZE = 32; // bytes per url
const auto BLOCK_SIZE = 5 * 10000; // rows number

/**
 * Interfaces
 */
unsigned int getReducerNumber(long long input_bytes)
{
    return (unsigned int)(input_bytes / RECORD_SIZE / BLOCK_SIZE + 1);
};

template <typename T>
list<Reducer> hashAndPartition(string filename, unsigned int reducer_num)
{
    list<Reducer> result;
    
    // open every file and set count=0
    unsigned int counts[reducer_num];
    ofstream wstreams[reducer_num];
    for (int i = 0; i < reducer_num; i ++) {
        wstreams[i].open(genfilename(filename, i));
        counts[i] = 0;
    }
    
    std::hash<T> hashx; // use STL lib hash function
    string line;
    ifstream infile(filename);
    while (getline(infile, line)) {
        istringstream iss(line);
        T key;
        iss >> key;
        auto reducer_id = hashx(key) % reducer_num;
        wstreams[reducer_id] << key << endl;
        counts[reducer_id] += 1;
    }
    
    // release resources and construct return list
    infile.close();
    for (int i = 0; i < reducer_num; i++) {
        wstreams[i].close();
        if (counts[i]) {
            cout << "[info] " << genfilename(filename, i) << ": " << counts[i] << endl; // log hashed file info
            result.push_back((Reducer){genfilename(filename, i), counts[i]});
        }
    }
    
    return result;
};

/// keep top K max value by using minheap
template <typename T>
void keepTopK(FrequencyStats<T> val, minheap<T>& h, unsigned k)
{
    if (h.size() < k) {
        h.push(val);
    } else {
        auto x = h.top();
        if (val.count > x.count) {
            h.pop();
            h.push(val);
        }
    }
}

/// if rows overflow throw error simplely
template <typename T>
minheap<T> findTopK(string filename, unsigned int k)
{
    // read file and construct hashmap
    map<T, unsigned int> keymap;
    string line;
    ifstream infile(filename);
    while (getline(infile, line)) {
        if (keymap.size() > BLOCK_SIZE) {
            throw std::range_error(errmsg("memory overflow", BLOCK_SIZE));
        }
        istringstream iss(line);
        T key;
        iss >> key;
        if (keymap.find(key) == keymap.end()) {
            keymap[key] = 1;
        } else {
            keymap[key]++;
        }
    }
    
    // select the top k max frequency key
    minheap<T> result;
    int c = 0;
    for(auto iter = keymap.begin(); iter != keymap.end(); ++iter) {
        T key =  iter->first;
        auto count = iter->second;
        c += count;
        keepTopK((FrequencyStats<T>){key, count}, result, k);
    }
    
    return result;
};

/// pre is actually modifyed
template <typename T>
minheap<T> mergeTopK(minheap<T>& pre, minheap<T> cur, unsigned int k)
{
    while (!cur.empty()) {
        keepTopK(cur.top(), pre, k);
        cur.pop();
    }
    return pre;
};


/// should in the header file
template <typename T>
minheap<T> runJob(string filename, unsigned int k);

/// most brilliant and difficult part
template <typename T>
minheap<T> tryFindTopK(string filename, unsigned int k)
{
    try {
        return findTopK<T>(filename, k); // memory usage here
    } catch(std::range_error e) {
        cout << filename << ": " << e.what() << endl;
        return runJob<T>(filename, k); // if too many rows just rerun job again
    }
};

/**
 * find topK Job entry point
 */
template <typename T>
minheap<T> runJob(string filename, unsigned int k)
{
    cout << "\n[info] starting hash and partition...\n" << endl;
    const auto reducer_num = getReducerNumber(getFileSize(filename));
    const auto reducers = hashAndPartition<T>(filename, reducer_num);
    minheap<T> pre;
    minheap<T> cur;
    cout << "\n[info] starting find top k...\n" << endl;
    for (auto reducer: reducers) {
        cur = tryFindTopK<T>(reducer.filename, k);
        if (pre.empty()) {
            pre = cur;
        } else {
            pre = mergeTopK(pre, cur, k);
        }
    }
    return pre;
};

template <typename T>
void printTopK(minheap<T> finalTopK) 
{
    int i = 0;
    while (!finalTopK.empty() && i++ < 100) {
        auto item = finalTopK.top();
        cout << "[info] " << item.key << ": " << item.count << endl;
        finalTopK.pop();
    }
}

int main() 
{
    auto finalTopK = runJob<string>("/Users/dengwanc/Desktop/c-lang/c-lang/output/test_url", 100);
    printTopK(finalTopK);
}

// todo
// 1. minheap2array
// 2. 测试 10 GB 数据
// 3. 测试 Int 数据
// 4. 支持 main 参数