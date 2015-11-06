#pragma once

#include <pthread.h>
#include <list>

template <typename T> class Channel {
    private:
        std::list<T> queue;
        pthread_mutex_t mutex;
        pthread_cond_t condv;

    public:
        bool closed = false;

        Channel() {
            pthread_mutex_init(&this->mutex, NULL);
            pthread_cond_init(&this->condv, NULL);
        }

        ~Channel() {
            pthread_mutex_destroy(&this->mutex);
            pthread_cond_destroy(&this->condv);
        }

        void close() {
            pthread_mutex_lock(&this->mutex);
            this->closed = true;
            pthread_mutex_unlock(&this->mutex);
            pthread_cond_broadcast(&this->condv);
        }

        void put(T item) {
            pthread_mutex_lock(&this->mutex);
            this->queue.push_back(item);
            pthread_cond_signal(&this->condv);
            pthread_mutex_unlock(&this->mutex);
        }

        T get() {
            pthread_mutex_lock(&this->mutex);

            while (this->queue.size() == 0) {
                pthread_cond_wait(&this->condv, &this->mutex);
            }

            T item = this->queue.front();
            this->queue.pop_front();
            pthread_mutex_unlock(&this->mutex);
            return item;
        }

        size_t size() {
            pthread_mutex_lock(&this->mutex);
            size_t sz = this->queue.size();
            pthread_mutex_unlock(&this->mutex);
            return sz;
        }
};
