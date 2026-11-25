#pragma once

#include <algorithm>
#include <array>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <limits>
#include <random>
#include <span>
#include <vector>

#include "benchmarks/workloads/workloadTemplates.hpp"

namespace key_multitude::benchmark_workload {

struct ProductCatalogWorkload {
private:
    enum class Operation {
        Insert,
        FilterByPrice,
        CountWithinBudget,
        SelectByPriceRank
    };

    struct PriceRange {
        int minimum;
        int maximum;
    };

    struct Request {
        Operation operation;
        int argument;
        int upperBound = 0;
    };

    static constexpr int INSERT_COUNT = 50;
    static constexpr int PRICE_FILTER_COUNT = 750;
    static constexpr int BUDGET_QUERY_COUNT = 150;
    static constexpr int PRICE_RANK_COUNT = 50;
    static constexpr int BATCH_SIZE =
        INSERT_COUNT + PRICE_FILTER_COUNT + BUDGET_QUERY_COUNT + PRICE_RANK_COUNT;
    static constexpr int MAX_HISTORY_SIZE = 1'000'000;
    // Reserve room for a unique listing ID at each price.
    static constexpr int LISTING_ID_STRIDE = 1 << 20;
    static constexpr unsigned SEED = 42;
    static constexpr std::array PRICE_BANDS{
        PriceRange{5, 30},
        PriceRange{31, 75},
        PriceRange{76, 200},
    };
    static constexpr std::array PRICE_FILTERS{
        PriceRange{5, 10},
        PriceRange{10, 20},
        PriceRange{20, 35},
        PriceRange{35, 60},
        PriceRange{60, 100},
    };

    static_assert(MAX_HISTORY_SIZE + INSERT_COUNT <= LISTING_ID_STRIDE);
    static_assert(PRICE_BANDS.back().maximum + 1 <=
                  std::numeric_limits<int>::max() / LISTING_ID_STRIDE);

    // Prices are in hundreds of roubles. Keys sort by price first, then by listing
    // ID.
    static constexpr int firstKeyAtPrice(int price) {
        return price * LISTING_ID_STRIDE;
    }

    static std::vector<int> makeListings(int count, std::mt19937& random) {
        std::discrete_distribution<std::size_t> chooseBand({55, 35, 10});
        std::vector<int> listings;
        listings.reserve(static_cast<std::size_t>(count));

        for (int id = 0; id < count; ++id) {
            const auto band = PRICE_BANDS[chooseBand(random)];
            const int price = std::uniform_int_distribution<int>(
                band.minimum, band.maximum)(random);
            listings.push_back(firstKeyAtPrice(price) + id);
        }
        return listings;
    }

    static std::vector<Request> makeRequests(int startingSize,
                                             std::span<const int> newListings,
                                             std::mt19937& random) {
        std::vector<Operation> operations;
        operations.reserve(BATCH_SIZE);
        operations.insert(operations.end(), INSERT_COUNT, Operation::Insert);
        operations.insert(
            operations.end(), PRICE_FILTER_COUNT, Operation::FilterByPrice);
        operations.insert(
            operations.end(), BUDGET_QUERY_COUNT, Operation::CountWithinBudget);
        operations.insert(
            operations.end(), PRICE_RANK_COUNT, Operation::SelectByPriceRank);
        std::shuffle(operations.begin(), operations.end(), random);

        std::discrete_distribution<std::size_t> chooseFilter({30, 30, 20, 15, 5});
        std::bernoulli_distribution chooseFirstPages(0.85);
        constexpr int FIRST_PAGES_SIZE = 60;
        std::vector<Request> requests;
        requests.reserve(operations.size());
        int currentSize = startingSize;
        auto nextListing = newListings.begin();

        for (const Operation operation : operations) {
            switch (operation) {
                case Operation::Insert:
                    requests.push_back({operation, *nextListing++});
                    ++currentSize;
                    break;
                case Operation::FilterByPrice: {
                    const auto filter = PRICE_FILTERS[chooseFilter(random)];
                    requests.push_back({operation,
                                        firstKeyAtPrice(filter.minimum) - 1,
                                        firstKeyAtPrice(filter.maximum + 1) - 1});
                    break;
                }
                case Operation::CountWithinBudget: {
                    const int budget = PRICE_FILTERS[chooseFilter(random)].maximum;
                    requests.push_back({operation, firstKeyAtPrice(budget + 1)});
                    break;
                }
                case Operation::SelectByPriceRank: {
                    const int lastRank =
                        chooseFirstPages(random)
                            ? std::min(FIRST_PAGES_SIZE, currentSize)
                            : currentSize;
                    const int rank =
                        std::uniform_int_distribution<int>(1, lastRank)(random);
                    requests.push_back({operation, rank});
                    break;
                }
            }
        }
        return requests;
    }

public:
    static constexpr auto NAME = "product_catalog";

    template <KeyMultitudeBackend Backend>
    static void measure(benchmark::State& state) {
        const auto size = state.range(0);
        if (size <= 0 || size > MAX_HISTORY_SIZE) {
            state.SkipWithError(
                "Product catalog size is outside the supported range");
            return;
        }

        const auto startingSize = static_cast<int>(size);
        std::mt19937 random(SEED);
        const auto listings = makeListings(startingSize + INSERT_COUNT, random);
        const std::span<const int> listingKeys{listings};
        const auto history =
            listingKeys.first(static_cast<std::size_t>(startingSize));
        const auto requests =
            makeRequests(startingSize, listingKeys.last(INSERT_COUNT), random);

        measureWithFreshMultitude<Backend>(
            state, history, BATCH_SIZE, [&requests](auto& keys) {
                std::size_t checksum = 0;
                for (const auto& request : requests) {
                    switch (request.operation) {
                        case Operation::Insert:
                            checksum += keys.insertKey(request.argument);
                            break;
                        case Operation::FilterByPrice:
                            checksum += keys.countKeysInRange(request.argument,
                                                              request.upperBound);
                            break;
                        case Operation::CountWithinBudget:
                            checksum += keys.countKeysLessThan(request.argument);
                            break;
                        case Operation::SelectByPriceRank:
                            checksum +=
                                static_cast<std::size_t>(keys.findKthSmallestKey(
                                    static_cast<std::size_t>(request.argument)));
                            break;
                    }
                }
                return checksum;
            });
    }
};

} // namespace key_multitude::benchmark_workload
