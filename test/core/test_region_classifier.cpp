// Unit tests for core::RegionClassifier
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

import core.region_classifier;

using core::RegionClassifier;

TEST(RegionClassifierTest, CreateForCurrentProcess) {
    auto classifierExp = RegionClassifier::create(::getpid());
    ASSERT_TRUE(classifierExp.has_value()) << "Failed to create classifier";
}

TEST(RegionClassifierTest, ClassifyStackAndHeap) {
    auto classifierExp = RegionClassifier::create(::getpid());
    ASSERT_TRUE(classifierExp.has_value());
    auto classifier = std::move(classifierExp.value());

    // Stack address
    int localVar = 123;
    auto stackStr =
        classifier.classify(reinterpret_cast<std::uintptr_t>(&localVar));
    // Expect either exact "stack" or not unknown
    EXPECT_NE(stackStr, std::string("unk"));

    // Heap address
    void* heapPtr = std::malloc(64);
    ASSERT_NE(heapPtr, nullptr);
    auto heapStr =
        classifier.classify(reinterpret_cast<std::uintptr_t>(heapPtr));
    EXPECT_NE(heapStr, std::string("unk"));
    std::free(heapPtr);
}
