#define BOOST_TEST_MODULE "test_read_local_forcefield"

#ifdef BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

#include <mjolnir/core/SimulatorTraits.hpp>
#include <mjolnir/input/read_local_forcefield.hpp>

#include <typeindex>
#include <typeinfo>

BOOST_AUTO_TEST_CASE(read_empty_local_forcefield)
{
    mjolnir::LoggerManager::set_default_logger("test_read_local_forcefield.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    {
        const toml::array v{};
        const auto ff = mjolnir::read_local_forcefield<traits_type>(v, "./");
        BOOST_TEST(ff.empty());
        BOOST_TEST(ff.size() == 0u);
    }
}

BOOST_AUTO_TEST_CASE(read_local_forcefield)
{
    mjolnir::LoggerManager::set_default_logger("test_read_local_forcefield.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    {
        using namespace toml::literals;
        const toml::array v{u8R"(
                interaction = "BondAngle"
                potential   = "Harmonic"
                topology    = "none"
                parameters  = []
            )"_toml
        };

        const auto lff = mjolnir::read_local_forcefield<traits_type>(v, "./");
        BOOST_TEST(!lff.empty());
        BOOST_TEST(lff.size() == 1u);

        const auto& interaction_ptr = *lff.begin();
        BOOST_TEST(static_cast<bool>(interaction_ptr));

        const auto bond_angle_ptr  = dynamic_cast<mjolnir::BondAngleInteraction<
            traits_type, mjolnir::HarmonicPotential<real_type>>*
            >(interaction_ptr.get());
        BOOST_TEST(static_cast<bool>(bond_angle_ptr));
    }
}

BOOST_AUTO_TEST_CASE(read_several_local_forcefield)
{
    mjolnir::LoggerManager::set_default_logger("test_read_local_forcefield.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    {
        using namespace toml::literals;
        const toml::array v{u8R"(
                interaction = "BondAngle"
                potential   = "Harmonic"
                topology    = "none"
                parameters  = []
            )"_toml, u8R"(
                interaction = "BondLength"
                potential   = "Harmonic"
                topology    = "bond"
                parameters  = []
            )"_toml
        };

        const auto lff = mjolnir::read_local_forcefield<traits_type>(v, "./");
        BOOST_TEST(!lff.empty());
        BOOST_TEST(lff.size() == 2u);

        using bond_length_interaction = mjolnir::BondLengthInteraction<
            traits_type, mjolnir::HarmonicPotential<real_type>>;
        using bond_angle_interaction  = mjolnir::BondAngleInteraction<
            traits_type, mjolnir::HarmonicPotential<real_type>>;

        std::map<std::type_index, bool> found;
        found[typeid(bond_length_interaction)] = false;
        found[typeid(bond_angle_interaction)]  = false;

        for(const auto& interaction_ptr : lff)
        {
            BOOST_TEST(static_cast<bool>(interaction_ptr));
            const auto& deref = *interaction_ptr;
            found[typeid(deref)] = true;
        }
        BOOST_TEST(found[typeid(bond_length_interaction)]);
        BOOST_TEST(found[typeid(bond_angle_interaction)]);
    }
}
