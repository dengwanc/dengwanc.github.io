#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <list>
#include <map>
#include <queue>
#include <sys/stat.h>

using namespace std;

/**
 * Type Modules
 */

/// actually split file
struct SplitFile {
    string filename;
    unsigned int rows;
};

/// frequency for the key
template <typename T>
struct FrequencyStats {
    T key;
    unsigned int count;
};

// define minheap comparation
template <typename T>
class Compare {
public: bool operator() (FrequencyStats<T> a, FrequencyStats<T> b) { return a.count > b.count; }
};

/// minheap data-struct implement by priority_queue
template<typename T>
using minheap = priority_queue<FrequencyStats<T>, std::vector<FrequencyStats<T>>, Compare<T>>;

/**
 * Helper Modules
 */

/// concat str1 + str2 + str(int)
string concatssi(string s1, string s2, int code)
{
    std::ostringstream oss;
    oss << s1 << s2 << code;
    return oss.str();
};

string errmsg(const char *msg, int code) { return concatssi(msg, "::code=", code); }
string genfilename(string org, int lv) { return concatssi(org, "_", lv); };

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


/**
 * CONST Modules
 */

const auto MEMORY_LIMIT = 1024 * 1024 * 10;
const auto RECORD_SIZE = 32; // bytes per url approx
const auto BLOCK_SIZE = MEMORY_LIMIT / RECORD_SIZE; // rows number

/**
 * Application Modules
 */

unsigned int getReducerNumber(long long input_bytes)
{
    return (unsigned int)(input_bytes / RECORD_SIZE / BLOCK_SIZE + 1);
};

template <typename T>
list<SplitFile> hashAndPartition(string filename, unsigned int reducer_num)
{
    list<SplitFile> result;
    
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
    T key;
    while (infile >> key) { // if empty line happened, will be skipped
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
            result.push_back((SplitFile){genfilename(filename, i), counts[i]});
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
    T key;
    while (infile >> key) {
        if (keymap.size() > BLOCK_SIZE) {
            throw std::range_error(errmsg("memory overflow", BLOCK_SIZE));
        }
        
        if (keymap.find(key) == keymap.end()) {
            keymap[key] = 1;
        } else {
            keymap[key]++;
        }
    }
    
    // select the top k max frequency key
    minheap<T> result;
    int c = 0;
    for (const auto& iter: keymap) {
        T key =  iter.first;
        auto count = iter.second;
        c += count;
        keepTopK((FrequencyStats<T>){key, count}, result, k);
    }
    
    return result;
};

/// pre is actually modified
template <typename T>
minheap<T> mergeTopK(minheap<T>& pre, minheap<T> cur, unsigned int k)
{
    while (!cur.empty()) {
        keepTopK(cur.top(), pre, k);
        cur.pop();
    }
    return pre;
};


/// hoisting function define
template <typename T>
minheap<T> runJob(string filename, unsigned int k);

/// most brilliant and difficult part
template <typename T>
minheap<T> tryFindTopK(string filename, unsigned int k)
{
    try {
        // main memory usage and 1 G limitation here
        // many uniq values hashed to a same file is extremely low probability event
        // if the file is too big, it must be lots of same data
        // so we just try to read it into memory
        return findTopK<T>(filename, k);
    } catch(std::range_error e) {
        cout << filename << ": " << e.what() << endl;
        return runJob<T>(filename, k); // if too many rows just rerun job again
    }
};

/// EntryPoint
template <typename T>
minheap<T> runJob(string filename, unsigned int k)
{
    // step 1 hash big file to small file
    cout << "\n[info] starting hash and partition...\n" << endl;
    const auto reducer_num = getReducerNumber(getFileSize(filename));
    const auto reducers = hashAndPartition<T>(filename, reducer_num);
    
    // step 2 find the top k values for every small file
    cout << "\n[info] starting find top k...\n" << endl;
    minheap<T> pre;
    minheap<T> cur;
    for (auto reducer: reducers) {
        // step 3 if the "small" file is big enough
        // take the file as big file too, run another Job
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
// 5. 这个代码在生产环境中敢不敢用
// 6. 这个代码在分布式系统中可不可复用
