template<typename T>
class ThreadSafeQueue {
public:
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(value);
    }

    void push(T&& value) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(std::move(value));
    }

    std::vector<T> drain() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<T> out;
        out.swap(queue);
        return out;
    }

private:
    std::mutex mutex;
    std::vector<T> queue;
};