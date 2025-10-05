#pragma once

#include "benchmarks_resourses.h"

namespace benchmark {

template<
    template<typename, typename, typename> class LRUCacheContainer
>
void simple_benchmark(std::string &&output_filename) {

    using UserCache = LRUCacheContainer<
        User,
        indexed_by<
            ordered_unique<tag<id_tag>, member<User, int, &User::id>>,
            ordered_unique<tag<email_tag>, member<User, std::string, &User::email>>,
            ordered_non_unique<tag<name_tag>, member<User, std::string, &User::name>>
        >,
        std::allocator<User>
    >;

    lru_concept_assert_for_one_tag(UserCache, id_tag, int, User);
    lru_concept_assert_for_one_tag(UserCache, email_tag, std::string, User);
    lru_concept_assert_for_one_tag(UserCache, name_tag, std::string, User);

    std::ofstream output_file(output_filename, std::ios::app);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file: " << output_filename << std::endl;
        return;
    }

    output_file << std::left << std::setw(20) << "Operations count" 
                << std::setw(16) << "Cache size" 
                << std::setw(12) << "Time (ms)" 
                << std::endl;
    output_file << std::string(50, '-') << std::endl;

    for (const size_t size : CACHE_SIZES) {
        UserCache cache(size);
        for (size_t i = 0; i < size; ++i) {
            cache.emplace(generator::generate_user());
        }

        size_t reading_operations_number = OPERATIONS_NUMBER * 4 / 5;
        size_t writing_operations_number = OPERATIONS_NUMBER / 5;

        std::vector<std::string> names, emails;
        std::vector<int> ids;
        std::vector<User> users;
        
        for (size_t i = 0; i < reading_operations_number; ++i) {
            names.push_back(generator::generate_name());
            emails.push_back(generator::generate_email());
            ids.push_back(generator::generate_id());
        }

        for (size_t i = 0; i < writing_operations_number; ++i) {
            users.push_back(generator::generate_user());
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < reading_operations_number; ++i) {
            cache.template find<name_tag, std::string>(names[i]);
            cache.template find<email_tag, std::string>(emails[i]);
            cache.template find<id_tag, int>(ids[i]);
        }

        for (size_t i = 0; i < writing_operations_number; ++i) {
            cache.emplace(users[i]);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        output_file << std::left << std::setw(20) << OPERATIONS_NUMBER
                    << std::setw(16) << size 
                    << std::setw(12) << elapsed.count()
                    << std::endl;
        output_file << std::string(50, '-') << std::endl;
    }

    output_file.close();
}

}