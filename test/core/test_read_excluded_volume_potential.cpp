#define BOOST_TEST_MODULE "test_read_excluded_volume_potential"

#ifdef BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

#include <mjolnir/input/read_global_potential.hpp>

BOOST_AUTO_TEST_CASE(read_excluded_volume_double)
{
    mjolnir::LoggerManager::set_default_logger("test_read_excluded_volume.log");

    using real_type = double;
    constexpr real_type tol = 1e-8;
    {
        using namespace toml::literals;
        const toml::value v = u8R"(
            interaction             = "Pair"
            potential               = "ExcludedVolume"
            spatial_partition.type  = "Nothing"
            epsilon                 = 3.14
            ignore.molecule         = "Nothing"
            ignore.particles_within.bond    = 3
            ignore.particles_within.contact = 1
            parameters  = [
                {index =   0, radius =   2.0},
                {index =   1, radius =   2.0},
                {index =   3, radius =   3.0},
                {index =   5, radius =   5.0},
                {index =   7, radius =   7.0},
                {index = 100, radius = 100.0},
            ]
        )"_toml;

        const auto g = mjolnir::read_excluded_volume_potential<real_type>(v);

        const auto ignore_within = g.ignore_within();
        const std::map<std::string, std::size_t> within(
                ignore_within.begin(), ignore_within.end());

        BOOST_TEST(g.ignore_within().size() == 2u);
        BOOST_TEST(within.at("bond")    == 3ul);
        BOOST_TEST(within.at("contact") == 1ul);

        BOOST_TEST(g.participants().size() ==   6u);
        BOOST_TEST(g.participants().at(0)  ==   0u);
        BOOST_TEST(g.participants().at(1)  ==   1u);
        BOOST_TEST(g.participants().at(2)  ==   3u);
        BOOST_TEST(g.participants().at(3)  ==   5u);
        BOOST_TEST(g.participants().at(4)  ==   7u);
        BOOST_TEST(g.participants().at(5)  == 100u);

        BOOST_TEST(g.parameters().at(  0)  ==   2.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  1)  ==   2.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  3)  ==   3.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  5)  ==   5.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  7)  ==   7.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(100)  == 100.0, boost::test_tools::tolerance(tol));

        BOOST_TEST(g.epsilon() == 3.14, boost::test_tools::tolerance(tol));
    }
}

BOOST_AUTO_TEST_CASE(read_excluded_volume_float)
{
    mjolnir::LoggerManager::set_default_logger("test_read_excluded_volume.log");
    using real_type = float;
    constexpr real_type tol = 1e-4;

    {
        using namespace toml::literals;
        const toml::value v = u8R"(
            interaction             = "Pair"
            potential               = "ExcludedVolume"
            spatial_partition.type  = "Nothing"
            epsilon                 = 3.14
            ignore.molecule         = "Nothing"
            ignore.particles_within.bond    = 3
            ignore.particles_within.contact = 1
            parameters  = [
                {index =   0, radius =   2.0},
                {index =   1, radius =   2.0},
                {index =   3, radius =   3.0},
                {index =   5, radius =   5.0},
                {index =   7, radius =   7.0},
                {index = 100, radius = 100.0},
            ]
        )"_toml;

        const auto g = mjolnir::read_excluded_volume_potential<real_type>(v);

        const auto ignore_within = g.ignore_within();
        const std::map<std::string, std::size_t> within(
                ignore_within.begin(), ignore_within.end());

        BOOST_TEST(g.ignore_within().size() == 2u);
        BOOST_TEST(within.at("bond")    == 3ul);
        BOOST_TEST(within.at("contact") == 1ul);

        BOOST_TEST(g.participants().size() ==   6u);
        BOOST_TEST(g.participants().at(0)  ==   0u);
        BOOST_TEST(g.participants().at(1)  ==   1u);
        BOOST_TEST(g.participants().at(2)  ==   3u);
        BOOST_TEST(g.participants().at(3)  ==   5u);
        BOOST_TEST(g.participants().at(4)  ==   7u);
        BOOST_TEST(g.participants().at(5)  == 100u);

        BOOST_TEST(g.parameters().at(  0)  ==   2.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  1)  ==   2.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  3)  ==   3.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  5)  ==   5.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(  7)  ==   7.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.parameters().at(100)  == 100.0f, boost::test_tools::tolerance(tol));

        BOOST_TEST(g.epsilon() == 3.14f, boost::test_tools::tolerance(tol));
    }}