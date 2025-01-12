#include <mach-o/dyld.h>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <sstream>


#define ASLR_BIAS _dyld_get_image_vmaddr_slide

template <typename T>
struct monoArray {
    void *klass;
    void *monitor;
    void *bounds;
    int32_t capacity;
    T m_Items[0];

    int32_t getCapacity() {
        return capacity; 
    }
    T *getPointer() {
        return m_Items; 
    }
    std::vector<T> toCPPlist() {
        std::vector<T> ret;
        for (int i = 0; i < capacity; i++) ret.push_back(m_Items[i]);
        return std::move(ret);
    }
    bool copyFrom(const std::vector<T> &vec) {
        return copyFrom((T *)vec.data(), (int)vec.size());
    }
    bool copyFrom(T *arr, int size) {
        if (size < capacity) return false;
        memcpy(m_Items, arr, size * sizeof(T));
        return true;
    }
    void copyTo(T *arr) {
        if (!CheckObj(m_Items)) return;
        memcpy(arr, m_Items, sizeof(T) * capacity);
    }
    T &operator[](int index) {
        if (getCapacity() < index) {
            T a{};
            return a;
        }
        return m_Items[index];
    }
    T &at(int index) {
        if (getCapacity() <= index || empty()) {
            throw std::out_of_range("Index out of range");
        }
        return m_Items[index];
    }
    bool empty() {
        return getCapacity() <= 0;
    }
    static monoArray<T> *Create(int capacity) {
        auto monoArr = (monoArray<T> *)malloc(sizeof(monoArray) + sizeof(T) * capacity);
        monoArr->capacity = capacity;
        return monoArr;
    }
    static monoArray<T> *Create(const std::vector<T> &vec) { return Create(vec.data(), vec.size()); }
    static monoArray<T> *Create(T *arr, int size) {
        monoArray<T> *monoArr = Create(size);
        monoArr->copyFrom(arr, size);
        return monoArr;
    }
};

typedef struct _monoString {
    void *klass;
    void *monitor;
    int length;
    char chars[1];
    int getLength() { return length; }
    char *getChars() { return chars; }
    NSString *toNSString() { return [[NSString alloc] initWithBytes:(const void *)(chars) length:(NSUInteger)(length * 2) encoding:(NSStringEncoding)NSUTF16LittleEndianStringEncoding]; }

    char *toCString() {
        NSString *v1 = toNSString();
        return (char *)([v1 UTF8String]);
    }
    std::string toCPPString() { return std::string(toCString()); }
} monoString;

monoString *il2cpp_string_new(const char *str) {
    monoString *(*String_CreateString)(void *instance, const char *str, int start, int len) = (monoString * (*)(void *, const char *, int, int)) getRealOffset(ENCRYPTOFFSET("0x3424C24"));
    return String_CreateString(nullptr, str, 0, strlen(str));
}

template <typename T>
struct monoList {
    void *unk0;
    void *unk1;
    monoArray<T> *items;
    int size;
    int version;

    T getItems() { return items->getPointer(); }

    int getSize() { return size; }

    int getVersion() { return version; }
};

template <typename TKey, typename TValue>
struct monoDictionary {
    struct Entry {
        int hashCode, next;
        TKey key;
        TValue value;
    };
    void *klass;
    void *monitor;
    monoArray<int> *buckets;
    monoArray<Entry> *entries;
    int count;
    int version;
    int freeList;
    int freeCount;
    void *comparer;
    monoArray<TKey> *keys;
    monoArray<TValue> *values;
    void *syncRoot;
    std::map<TKey, TValue> toMap() {
        std::map<TKey, TValue> ret;
        for (auto it = (Entry *)&entries->m_Items; it != ((Entry *)&entries->m_Items + count); ++it) 
            ret.emplace(std::make_pair(it->key, it->value));
        return std::move(ret);
    }
    std::vector<TKey> getKeys() {
        std::vector<TKey> ret;
        for (int i = 0; i < count; ++i) ret.emplace_back(entries->at(i).key);
        return std::move(ret);
    }
    std::vector<TValue> getValues() {
        std::vector<TValue> ret;
        for (int i = 0; i < count; ++i) ret.emplace_back(entries->at(i).value);
        return std::move(ret);
    }
    int getSize() { return count; }
    int getVersion() { return version; }
    TValue Get(TKey key) {
        TValue ret;
        if (TryGet(key, ret)) return ret;
        return {};
    }
    TValue operator[](TKey key) { return Get(key); }
};