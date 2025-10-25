#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>

#include "../lru_container_concept.h" 

USERVER_NAMESPACE_BEGIN

namespace benchmarks {

const std::vector<long long> CACHE_SIZES = {1000, 10000, 100000};
const size_t OPERATIONS_NUMBER = 100000;
const int MAX_ID_SIZE = 50000;

struct id_tag {};
struct email_tag {};
struct name_tag {};

struct User {
    int id;
    std::string email;
    std::string name;
    
    bool operator==(const User& other) const {
        return id == other.id && email == other.email && name == other.name;
    }
};

namespace generator {
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> action_dist(0.0, 1.0);
std::uniform_int_distribution<int> id_dist(0, MAX_ID_SIZE);

User generate_user() {
    std::string email = "email" + std::to_string(id_dist(gen));
    std::string name = "name" + std::to_string(id_dist(gen));
    return User{id_dist(gen), email, name};
}

int generate_id() {
    return id_dist(gen);
}

std::string generate_name() {
    return "name" + std::to_string(id_dist(gen));
}

std::string generate_email() {
    return "email" + std::to_string(id_dist(gen));
}
} // generator
} // benchmarks
USERVER_NAMESPACE_END