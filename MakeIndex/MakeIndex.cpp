#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

auto RealMain(int /*argc*/, char** /*argv*/) -> int
{
    return 0;
}

auto main(int argc, char** argv) -> int
{
    try
    {
        std::vector<std::string> args(argv + 1, argv + argc);

        auto it = std::ranges::find(args, "--selftest");
        bool runTests = it != args.end();

        if (runTests)
        {
            args.erase(it);

            std::vector<const char*> docTestArgv;
            docTestArgv.push_back(argv[0]);
            for (auto& s : args)
                docTestArgv.push_back(s.c_str());

            doctest::Context ctx;
            ctx.applyCommandLine(static_cast<int>(docTestArgv.size()), const_cast<char**>(docTestArgv.data()));
            return ctx.run();
        }

        return RealMain(argc, argv);
    }
    catch (...)
    {
        std::cerr << "An unexpected error occurred in main function" << std::endl;
        return 1;
    }
}

// This is just a small dummy test to make sure we actualy have a test and can verify that the testing system works
// We can remove this test once we start having actual tests
TEST_CASE("Empty dummy test")
{
}
