#define BOOST_TEST_MODULE "test_read_simulator"

#include <boost/test/included/unit_test.hpp>
#include <test/util/make_empty_input.hpp>
#include <mjolnir/input/read_simulator.hpp>

BOOST_AUTO_TEST_CASE(read_newtonian_molecular_dynamics_simulator)
{
    mjolnir::LoggerManager::set_default_logger("test_read_simulator.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    constexpr real_type tol = 1e-8;
    auto root = mjolnir::test::make_empty_input();
    {
        const toml::table v = toml::table{
            {"type",          toml::value("Molecular Dynamics")},
            {"integrator",    toml::value("Newtonian")},
            {"precision",     toml::value("double")},
            {"boundary_type", toml::value("Unlimited")},
            {"delta_t",       toml::value(0.1)},
            {"total_step",    toml::value(100)},
            {"save_step",     toml::value(10)}
        };
        root["simulator"] = v;
        const auto sim = mjolnir::read_simulator_from_table<traits_type>(root, v);
        BOOST_TEST(static_cast<bool>(sim));

        const auto mdsim = dynamic_cast<mjolnir::MolecularDynamicsSimulator<
            traits_type, mjolnir::VelocityVerletStepper<traits_type>>*>(sim.get());
        BOOST_TEST(static_cast<bool>(mdsim));

        sim->initialize();
        for(std::size_t i=0; i<99; ++i)
        {
            BOOST_TEST(mdsim->time() == i * 0.1, boost::test_tools::tolerance(tol));
            BOOST_TEST(sim->step()); // check it can step
        }
        // at the last (100-th) step, it returns false to stop the simulation.
        BOOST_TEST(!sim->step());
        sim->finalize();
    }
}

BOOST_AUTO_TEST_CASE(read_langevin_molecular_dynamics_simulator)
{
    mjolnir::LoggerManager::set_default_logger("test_read_simulator.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    constexpr real_type tol = 1e-8;
    auto root = mjolnir::test::make_empty_input();

    {
        const toml::table v{
            {"type",          toml::value("Molecular Dynamics")},
            {"integrator",    toml::value("Underdamped Langevin")},
            {"precision",     toml::value("double")},
            {"boundary_type", toml::value("Unlimited")},
            {"total_step",    toml::value(100)},
            {"save_step",     toml::value(10)},
            {"delta_t",       toml::value(0.1)},
            {"seed",          toml::value(12345)},
            {"parameters",    toml::array{}}
        };
        root["simulator"] = v;
        const auto sim = mjolnir::read_simulator_from_table<traits_type>(root, v);
        BOOST_TEST(static_cast<bool>(sim));

        const auto mdsim = dynamic_cast<mjolnir::MolecularDynamicsSimulator<
            traits_type, mjolnir::UnderdampedLangevinStepper<traits_type>>*>(sim.get());
        BOOST_TEST(static_cast<bool>(mdsim));

        sim->initialize();
        for(std::size_t i=0; i<99; ++i)
        {
            BOOST_TEST(mdsim->time() == i * 0.1, boost::test_tools::tolerance(tol));
            BOOST_TEST(sim->step()); // check it can step
        }
        // at the last (100-th) step, it returns false to stop the simulation.
        BOOST_TEST(!sim->step());
        sim->finalize();
    }
}

BOOST_AUTO_TEST_CASE(read_steepest_descent_simulator)
{
    mjolnir::LoggerManager::set_default_logger("test_read_simulator.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    constexpr real_type tol = 1e-8;
    auto root = mjolnir::test::make_empty_input();
    {
        const toml::table v{
            {"type",          toml::value("Steepest Descent")},
            {"precision",     toml::value("double")},
            {"boundary_type", toml::value("Unlimited")},
            {"step_limit",    toml::value(100)},
            {"save_step",     toml::value(10)},
            {"delta",         toml::value(0.1)},
            {"threshold",     toml::value(0.0)} // it never ends until hit the limit
        };
        root["simulator"] = v;
        const auto sim = mjolnir::read_simulator_from_table<traits_type>(root, v);
        BOOST_TEST(static_cast<bool>(sim));

        const auto sdsim = dynamic_cast<
            mjolnir::SteepestDescentSimulator<traits_type>*>(sim.get());
        BOOST_TEST(static_cast<bool>(sdsim));

        sim->initialize();
        for(std::size_t i=0; i<99; ++i)
        {
            BOOST_TEST(sim->step()); // check it can step
        }
        // at the last (100-th) step, it returns false to stop the simulation.
        BOOST_TEST(!sim->step());
        sim->finalize();
    }
}

BOOST_AUTO_TEST_CASE(read_simulated_annealing_simulator)
{
    mjolnir::LoggerManager::set_default_logger("test_read_simulator.log");

    using real_type = double;
    using traits_type = mjolnir::SimulatorTraits<real_type, mjolnir::UnlimitedBoundary>;
    constexpr real_type tol = 1e-8;
    auto root = mjolnir::test::make_empty_input();
    {
        const toml::table v{
            {"type",          toml::value("Simulated Annealing")},
            {"integrator",    toml::value("Underdamped Langevin")},
            {"precision",     toml::value("double")},
            {"boundary_type", toml::value("Unlimited")},
            {"total_step",    toml::value(100)},
            {"save_step",     toml::value(10)},
            {"schedule",      toml::value("linear")},
            {"T_begin",       toml::value(300.0)},
            {"T_end",         toml::value( 10.0)},
            {"each_step",     toml::value( 1)},
            {"delta_t",       toml::value(0.1)},
            {"seed",          toml::value(12345)},
            {"parameters",    toml::array{}}
        };
        root["simulator"] = v;
        const auto sim = mjolnir::read_simulator_from_table<traits_type>(root, v);
        BOOST_TEST(static_cast<bool>(sim));

        const auto sasim = dynamic_cast<mjolnir::SimulatedAnnealingSimulator<
            traits_type, mjolnir::UnderdampedLangevinStepper<traits_type>,
            mjolnir::linear_schedule>*>(sim.get());
        BOOST_TEST(static_cast<bool>(sasim));

        sim->initialize();
        for(std::size_t i=0; i<99; ++i)
        {
            BOOST_TEST(sasim->system().attribute("temperature") ==
                       300.0 * ((100-i) / 100.0) + 10.0 * (i / 100.0),
                       boost::test_tools::tolerance(tol));
            BOOST_TEST(sim->step()); // check it can step
        }
        // at the last (100-th) step, it returns false to stop the simulation.
        BOOST_TEST(!sim->step());
        sim->finalize();
    }
}