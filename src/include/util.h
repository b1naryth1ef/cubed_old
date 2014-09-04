#pragma once

#include "global.h"

using namespace rapidjson;

static std::vector<std::string> &splitString(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


static std::vector<std::string> splitString(const std::string &s, char delim) {
    std::vector<std::string> elems;
    splitString(s, delim, elems);
    return elems;
}

class Timer {
    public:
        std::chrono::high_resolution_clock::time_point t0, t1;

        void start() {
            this->t0 = std::chrono::high_resolution_clock::now();
        }

        long int end() {
            this->t1 = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds>
                (this->t1 - this->t0);
            return ms.count();
        }
};

class Ticker {
    private:
        int diff, per;
        bool first;
        std::chrono::high_resolution_clock::time_point t0, t1;

    public:
        int rate;

        Ticker(int rate_per_second) {
            this->rate = rate_per_second;
            this->t0 = std::chrono::high_resolution_clock::now();
            this->t1 = std::chrono::high_resolution_clock::now();
            this->first = true;

            this->per = (1.0 / this->rate) * 1000;
        }

        // Called after this tick, before next
        bool next() {
            if (this->first) {
                this->first = false;
                return true;
            }

            bool value;

            // End current tick
            this->t1 = std::chrono::high_resolution_clock::now();

            // Calculate difference
            std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds>
                (this->t1 - this->t0);

            // In milliseconds plz
            diff = ms.count();

            long int time_wait = this->per - diff;

            // If we run slow, skip sleep, otherwise sleep for the rest of the tick
            if (diff > this->per) {
                value = false;
            } else {
                value = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(time_wait));
            }

            // Start next tick
            this->t0 = std::chrono::high_resolution_clock::now();
            return value;
        }
};

enum CStorageType {
    STORAGE_INT,
    STORAGE_DOUBLE,
    STORAGE_STRING
};

class Container {
    private:
        void ensureType(CStorageType t) {
            if (this->type != t) {
                throw Exception("Invalid Type for container!");
            }
        }

        union {
            int i;
            double d;
        } value;

        std::string s;

    public:
        CStorageType type;

        Container *set(int i) {
            this->type = STORAGE_INT;
            this->value.i = i;
            return this;
        }

        Container *set(double d) {
            this->type = STORAGE_DOUBLE;
            this->value.d = d;
            return this;
        }

        Container *set(std::string s) {
            this->type = STORAGE_STRING;
            this->s = s;
            return this;
        }

        int getInt() {
            this->ensureType(STORAGE_INT);
            return this->value.i;
        }

        double getDouble() {
            this->ensureType(STORAGE_DOUBLE);
            return this->value.d;
        }

        std::string getString() {
            this->ensureType(STORAGE_STRING);
            return this->s;
        }

};

class Dict {
    private:
        std::map<std::string, Container*> data;

    public:
        Container *get(std::string k) {
            if (data.count(k)) {
                return data[k];
            }

            data[k] = new Container();
            return data[k];
        }

        int getInt(std::string k) {
            return get(k)->getInt();
        }

        Container *setInt(std::string k, int v) {
            Container *c = new Container();
            c->set(v);
            data[k] = c;
            return c;
        };

        double getDouble(std::string k) {
            return get(k)->getDouble();
        }

        Container *setDouble(std::string k, double v) {
            Container *c = new Container();
            c->set(v);
            data[k] = c;
            return c;
        }

        std::string getString(std::string k) {
            return get(k)->getString();
        }

        Container *setString(std::string k, std::string v) {
            Container *c = new Container();
            c->set(v);
            data[k] = c;
            return c;
        }

        Document *toJSON() {
            Document *result = new Document;
            result->SetObject();
            
            for (auto &kv : this->data) {
                Value k(kv.first.c_str(), result->GetAllocator());

                if (kv.second->type == STORAGE_INT) {
                    Value v(kv.second->getInt());
                    result->AddMember(k, v, result->GetAllocator());
                } else if (kv.second->type == STORAGE_STRING) {
                    Value v(kv.second->getString().c_str(), result->GetAllocator());
                    result->AddMember(k, v, result->GetAllocator());
                } else if (kv.second->type == STORAGE_DOUBLE) {
                    Value v(kv.second->getDouble());
                    result->AddMember(k, v, result->GetAllocator());
                }
            }

            return result;
        }

        bool fromJSON(Document *d) {
            return false;
        }
};