// -*- mode: c++ -*-

#ifndef FILE_BACKED_ARRAY_H
#define FILE_BACKED_ARRAY_H

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

// A roughly vector-like class which is to start with stored in
// normal memory. But if the total size of the array grows to
// more than kFlushThreshold bytes, starts instead storing
// the bulk of the data on disk (with access to the data
// provided with mmap).
template<class T,
         // 100M
         size_t kFlushThreshold = 100000000 / sizeof(T)>
class file_backed_mmap_array {
public:
    file_backed_mmap_array() {
    }

    file_backed_mmap_array(const file_backed_mmap_array& other) = delete;

    file_backed_mmap_array(file_backed_mmap_array&& other)
        : buffer_(std::move(other.buffer_)),
          frozen_(other.frozen_),
          size_(other.size_),
          fd_(other.fd_),
          array_(other.array_) {
        other.array_ = NULL;
        other.fd_ = -1;
    }

    void operator=(file_backed_mmap_array&& other) {
        maybe_close();
        buffer_ = std::move(other.buffer_);
        frozen_ = other.frozen_;
        size_ = other.size_;
        fd_ = other.fd_;
        array_ = other.array_;
        other.array_ = NULL;
        other.fd_ = -1;
    }

    ~file_backed_mmap_array() {
        maybe_close();
    }

    // If the total size of the data structure exceeds the
    // flush threshold, writes whatever data we have buffered
    // in memory to disk.
    void flush() {
        if (buffer_.size() && fd_ >= 0) {
            size_t bytes = sizeof(T) * buffer_.size();
            assert(write(fd_, (char*) &buffer_[0], bytes) == bytes);
            buffer_.clear();
        }
    }

    // Returns true iff the is storing no elements.
    bool empty() const { return size_ == 0; }

    // Iterators.
    T* begin() {
        assert(frozen_);
        return array_;
    }
    T* end() {
        assert(frozen_);
        return array_ + size_;
    }

    const T* begin() const {
        assert(frozen_);
        return array_;
    }
    const T* end() const {
        assert(frozen_);
        return array_ + size_;
    }

    T& operator[](size_t index) {
        assert(frozen_);
        return array_[index];
    }

    const T& operator[](size_t index) const {
        assert(frozen_);
        return array_[index];
    }

    // Returns the number of elements in the array.
    size_t size() const { return size_; }

    // Copies "data" into the last element of the array.
    void push_back(const T& data) {
        assert(!frozen_);
        buffer_.push_back(data);
        maybe_flush();
        size_++;
    }

    // Inserts a range of objects from begin to at the end
    // of the array.
    template<class It>
    void insert_back(const It begin, const It end) {
        buffer_.insert(buffer_.end(), begin, end);
        size_ += std::distance(begin, end);
    }

    // Prepares the array for reading. No mutating operations
    // may be excecuted while the array is frozen.
    void freeze() {
        maybe_map(PROT_READ, MAP_SHARED);
    }

    // Prepares the array for writing. No operations that read
    // specific elements may be excecuted while the array is thawed;
    // only pure writes.
    void thaw() {
        assert(frozen_);
        frozen_ = false;
        maybe_unmap(false);
    }

    // Empties an array.
    void reset() {
        assert(frozen_);
        frozen_ = false;
        if (fd_ >= 0) {
            lseek(fd_, 0, SEEK_SET);
        }
        maybe_unmap(true);
        size_ = 0;
        buffer_.clear();
        run_starts_.clear();
        run_ends_.clear();
    }

    // Marks the start of a new run of elements, after the last
    // element currently in the array.
    void start_run() {
        run_starts_.push_back(size_);
    }

    // Marks the current run as ending after the last element
    // currently in the array.
    void end_run() {
        run_ends_.push_back(size_);
    }

    // Returns a range for the i'th run that was recorded for the
    // array.
    std::pair<T*, T*> run(int i) const {
        return std::make_pair(run_starts_[i] + array_,
                              run_ends_[i] + array_);
    }

    // Returns a vector of all the runs that have been recorded
    // for the array.
    using Run = std::pair<const T*, const T*>;
    std::vector<Run> runs() const {
        std::vector<Run> ret;
        for (int i = 0; i < run_count(); ++i) {
            ret.push_back(std::make_pair(begin() + run_starts_[i],
                                         begin() + run_ends_[i]));
        }
        return ret;
    }

    // Returns the number of recorded runs.
    int run_count() const {
        return run_ends_.size();
    }

private:
    // If the array has a backing file, unmaps it. If truncate
    // is additionally truncates the file.
    void maybe_unmap(bool truncate) {
        if (fd_ >= 0 && array_) {
            munmap((void*) array_, size_ * sizeof(T));
            if (truncate) {
                ftruncate(fd_, 0);
            }
        }
        array_ = NULL;
    }

    // If the array has a backing file, unmaps and closes the file.
    void maybe_close() {
        if (fd_ >= 0) {
            maybe_unmap(true);
            close(fd_);
            fd_ = -1;
        }
    }

    // Opens a backing file for this array in the current working
    // directory.
    void open() {
        assert(fd_ == -1);
        char* fname = strdup("file-backed-tmp-XXXXXX");
        fd_ = mkstemp(fname);
        assert(fd_ >= 0);
        unlink(fname);
        free(fname);
    }

    // Flushes the array to disk, if the array is large enough.
    void maybe_flush() {
        if (buffer_.size() >= kFlushThreshold) {
            if (fd_ == -1) {
                open();
            }
            flush();
            buffer_.clear();
        }
    }

    // Freezes the array and sets array_ to point to the backing
    // element storage. Either an mmaped view of the backing file, or
    // a pointer to the start of the backing buffer.
    void maybe_map(int prot, int flags) {
        assert(!frozen_);
        if (fd_ >= 0 && size_ > 0) {
            flush();
            size_t len = sizeof(T) * size_;
            void* map = mmap(NULL, len, prot, flags, fd_, 0);
            if (map == MAP_FAILED) {
                perror("mmap");
                abort();
            }
            array_ = (T*) map;
        } else {
            array_ = &buffer_[0];
        }
        frozen_ = true;
    }

    std::vector<size_t> run_starts_;
    std::vector<size_t> run_ends_;
    // Elements that have been written to end of the array, but
    // not flushed to disk.
    std::vector<T> buffer_;
    bool frozen_ = false;
    // The number of elements in the array.
    size_t size_ = 0;
    // The file descriptor of the backing file; -1 if the array
    // does not yet have a backing file.
    int fd_ = -1;
    // A pointer to the start of the backing store (whether a mmaped
    // view of the backing file, or the in-memory buffer).
    T* array_ = NULL;
};

#endif // FILE_BACKED_ARRAY_H
