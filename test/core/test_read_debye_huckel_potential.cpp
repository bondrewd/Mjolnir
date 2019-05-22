#define BOOST_TEST_MODULE "test_read_debye_huckel_potential"

#ifdef BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

#include <mjolnir/input/read_global_potential.hpp>

BOOST_AUTO_TEST_CASE(read_debye_huckel_double)
{
    mjolnir::LoggerManager::set_default_logger("test_read_debye_huckel.log");

    using real_type = double;
    constexpr real_type tol = 1e-8;
    {
        using namespace toml::literals;
        const toml::value v = u8R"(
            interaction                     = "Pair"
            potential                       = "DebyeHuckel"
            spatial_partition.type          = "Nothing"
            ignore.molecule                 = "Nothing"
            ignore.particles_within.bond    = 3
            ignore.particles_within.contact = 1
            parameters = [
                {index =   0, charge =   1.0},
                {index =   1, charge =  -1.0},
                {index =   3, charge =   0.3},
                {index =   5, charge =   0.5},
                {index =   7, charge =   0.7},
                {index = 100, charge = 100.0},
            ]
        )"_toml;

        const auto g = mjolnir::read_debye_huckel_potential<real_type>(v);

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

        BOOST_TEST(g.charges().at(  0)  ==   1.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  1)  ==  -1.0, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  3)  ==   0.3, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  5)  ==   0.5, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  7)  ==   0.7, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(100)  == 100.0, boost::test_tools::tolerance(tol));
    }
}

BOOST_AUTO_TEST_CASE(read_debye_huckel_float)
{
    mjolnir::LoggerManager::set_default_logger("test_read_debye_huckel.log");
    using real_type = float;
    constexpr real_type tol = 1e-4;
    {
        using namespace toml::literals;
        const toml::value v = u8R"(
            interaction                     = "Pair"
            potential                       = "DebyeHuckel"
            spatial_partition.type          = "Nothing"
            ignore.molecule                 = "Nothing"
            ignore.particles_within.bond    = 3
            ignore.particles_within.contact = 1
            parameters = [
                {index =   0, charge =   1.0},
                {index =   1, charge =  -1.0},
                {index =   3, charge =   0.3},
                {index =   5, charge =   0.5},
                {index =   7, charge =   0.7},
                {index = 100, charge = 100.0},
            ]
        )"_toml;

        const auto g = mjolnir::read_debye_huckel_potential<real_type>(v);

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

        BOOST_TEST(g.charges().at(  0)  ==   1.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  1)  ==  -1.0f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  3)  ==   0.3f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  5)  ==   0.5f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(  7)  ==   0.7f, boost::test_tools::tolerance(tol));
        BOOST_TEST(g.charges().at(100)  == 100.0f, boost::test_tools::tolerance(tol));
    }}
