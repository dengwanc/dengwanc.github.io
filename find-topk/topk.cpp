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

int str2int(string str)
{
    try {
        return std::stoi(str);
    } catch(std::invalid_argument& e){
        throw std::invalid_argument("[error] please type int value");
    } catch(std::out_of_range& e){
        throw std::out_of_range("[error] value is out of int range");
    } catch(std::exception& e) {
        throw e;
    }
}

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
        throw std::runtime_error(errmsg("[error] stat no such file", rc));
    }
};

template <typename T>
vector<FrequencyStats<T>> heap2vector(minheap<T> h, bool should_reverse=false)
{
    vector<FrequencyStats<T>> arr;
    while (!h.empty()) {
        auto item = h.top();
        arr.push_back(item);
        h.pop();
    }
    
    if (should_reverse) {
        std::reverse(arr.begin(), arr.end());
    }
    
    return arr;
}

template <typename T>
void printTopK(minheap<T> finalTopK)
{
    cout << "\n[info] result as follows: " << endl;
    for (auto& item: heap2vector(finalTopK, true)) {
        cout << "[info] " << item.key << " => " << item.count << endl;
    }
}

/**
 * CONST Modules
 */

static auto memory_parameter = 1024; // for command line control

/// use define here so that lazying the actual caculate
#define RECORD_SIZE 32 // bytes per url approx
#define MEMORY_LIMIT (1024 * 1024 * memory_parameter)
#define BLOCK_SIZE (MEMORY_LIMIT / RECORD_SIZE) // rows number for a task
#define ROWS_FOR_REPORT (BLOCK_SIZE/5) // log when processed a batch of rows

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
    auto i = 0;
    while (infile >> key) { // if empty line happened, will be skipped
        auto reducer_id = hashx(key) % reducer_num;
        wstreams[reducer_id] << key << '\n';
        counts[reducer_id] += 1;
        ++i;
        if (i % ROWS_FOR_REPORT == 0) {
            cout << "[info] partition task " << filename << " " << i <<" rows processed" << endl;
        }
    }
    
    // release resources and construct return list
    infile.close();
    for (int i = 0; i < reducer_num; i++) {
        wstreams[i].close();
        if (counts[i]) {
            cout << "[info] partition result " << genfilename(filename, i) << "=" << counts[i] << " rows" << endl; // log hashed file info
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
    auto i = 0;
    while (infile >> key) {
        if (keymap.size() > BLOCK_SIZE) {
            throw std::range_error(errmsg("memory overflow", BLOCK_SIZE));
        }
        
        if (keymap.find(key) == keymap.end()) {
            keymap[key] = 1;
        } else {
            keymap[key]++;
        }
        i++;
        if (i % ROWS_FOR_REPORT == 0) {
            cout << "[info] topk task " << filename << " " << i <<" rows processed" << endl;
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
    cout << "\n[info] starting hash and partition..." << endl;
    const auto reducer_num = getReducerNumber(getFileSize(filename));
    cout << "[info] reducer_number=" << reducer_num << endl;
    const auto reducers = hashAndPartition<T>(filename, reducer_num);
    
    // step 2 find the top k values for every small file
    cout << "\n[info] starting find top k..." << endl;
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

int main(int argc, char** argv)
{
    if (argc == 1) {
        cout << "topk tools usage::" << endl
             << "    ./topk [$K] $filename [$number_of_MB]" << endl
             << "      default $K=100" << endl
             << "      default $number_of_MB=1024" << endl
             << "examples::" << endl
             << "    ./topk 100 test_url 10" <<endl
             << "    ./topk test" <<endl;
        return 0;
    }
    
    string filename;     // the input file
    unsigned thek = 100; // the K of topk problem
    
    // only pass the filename
    if (argc == 2) {
        filename = argv[1];
    }
    
    // pass the k and filename
    if (argc == 3) {
        thek = str2int(argv[1]);
        filename = argv[2];
    }
    
    // pass the k and filename and memory limit
    if (argc >= 4) {
        thek = str2int(argv[1]);
        filename = argv[2];
        memory_parameter = str2int(argv[3]);
    }
    
    printTopK(runJob<string>(filename, thek));
    
    return 0;
}
