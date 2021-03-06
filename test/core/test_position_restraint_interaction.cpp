#define BOOST_TEST_MODULE "test_position_restraint_interaction"

#ifdef BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

#include <mjolnir/core/BoundaryCondition.hpp>
#include <mjolnir/core/SimulatorTraits.hpp>
#include <mjolnir/forcefield/external/PositionRestraintInteraction.hpp>
#include <mjolnir/forcefield/local/HarmonicPotential.hpp>
#include <mjolnir/util/make_unique.hpp>
#include <random>

BOOST_AUTO_TEST_CASE(PositionRestraint_Harmonic)
{
    using traits_type      = mjolnir::SimulatorTraits<double, mjolnir::UnlimitedBoundary>;
    using real_type        = traits_type::real_type;
    using coordinate_type  = traits_type::coordinate_type;
    using boundary_type    = traits_type::boundary_type;
    using system_type      = mjolnir::System<traits_type>;
    using potential_type   = mjolnir::HarmonicPotential<real_type>;
    using interaction_type = mjolnir::PositionRestraintInteraction<traits_type, potential_type>;

    constexpr real_type tol = 1e-8;

    auto normalize = [](const coordinate_type& v){return v / mjolnir::math::length(v);};

    const potential_type pot1{/*k =*/1.0, /*r0 =*/ 0.0};
    const potential_type pot2{/*k =*/1.0, /*r0 =*/10.0};

    interaction_type interaction(
        std::vector<std::tuple<std::size_t, coordinate_type, potential_type>>{
            std::make_tuple(0, coordinate_type{0.0,0.0,0.0}, pot1),
            std::make_tuple(1, coordinate_type{0.0,0.0,0.0}, pot2)
        });

    system_type sys(2, boundary_type{});

    sys.at(0).mass  = 1.0;
    sys.at(1).mass  = 1.0;
    sys.at(0).rmass = 1.0;
    sys.at(1).rmass = 1.0;

    sys.at(0).position = coordinate_type( 0.0,  0.0,  0.0);
    sys.at(1).position = coordinate_type(10.0,  0.0,  0.0);
    sys.at(0).velocity = coordinate_type( 0.0,  0.0,  0.0);
    sys.at(1).velocity = coordinate_type( 0.0,  0.0,  0.0);
    sys.at(0).force    = coordinate_type( 0.0,  0.0,  0.0);
    sys.at(1).force    = coordinate_type( 0.0,  0.0,  0.0);

    sys.at(0).name  = "X";
    sys.at(1).name  = "X";
    sys.at(0).group = "NONE";
    sys.at(1).group = "NONE";

    std::mt19937 rng(123456789);
    std::uniform_real_distribution<real_type> uni(-1.0, 1.0);
    std::normal_distribution<real_type> gauss(0.0, 1.0);

    for(int i = 0; i < 10000; ++i)
    {
        sys.position(0) = coordinate_type(uni(rng), uni(rng), uni(rng));
        sys.force(0)    = coordinate_type(0, 0, 0);

        const auto dist = mjolnir::math::length(sys.position(0));
        const auto deriv = pot1.derivative(dist);
        const auto coef  = std::abs(deriv);

        interaction.calc_force(sys);

        const auto force_strength = mjolnir::math::length(sys.force(0));

        BOOST_TEST(coef == force_strength, boost::test_tools::tolerance(tol));

        // forces always attract the particle to the origin.
        const auto direction = mjolnir::math::dot_product(
            normalize(sys.force(0)), normalize(-sys.position(0)));

        BOOST_TEST(direction == 1e0, boost::test_tools::tolerance(tol));
    }

    for(int i = 0; i < 10000; ++i)
    {
        sys.position(1) = coordinate_type(uni(rng), uni(rng), uni(rng));
        sys.force(1)    = coordinate_type(0, 0, 0);

        const auto offset = 10.0 * normalize(
                coordinate_type(gauss(rng), gauss(rng), gauss(rng)));
        sys.position(1) += offset; // dist ~ 10.0 +- random vector

        const auto dist  = mjolnir::math::length(sys.position(1));
        const auto deriv = pot2.derivative(dist);
        const auto coef  = std::abs(deriv);

        interaction.calc_force(sys);
        const auto force_strength = mjolnir::math::length(sys.force(1));

        BOOST_TEST(coef == force_strength, boost::test_tools::tolerance(tol));

        if(dist < 10.0) // repulsive
        {
            const auto direction = mjolnir::math::dot_product(
                normalize(sys.force(1)), normalize(sys.position(1)));
            BOOST_TEST(direction == 1e0, boost::test_tools::tolerance(tol));
        }
        else // attractive
        {
            const auto direction = mjolnir::math::dot_product(
                normalize(sys.force(1)), normalize(-sys.position(1)));
            BOOST_TEST(direction == 1e0, boost::test_tools::tolerance(tol));
        }
    }
}

BOOST_AUTO_TEST_CASE(PositionRestraint_numerical_differentiation)
{
    using traits_type      = mjolnir::SimulatorTraits<double, mjolnir::UnlimitedBoundary>;
    using real_type        = traits_type::real_type;
    using coordinate_type  = traits_type::coordinate_type;
    using boundary_type    = traits_type::boundary_type;
    using system_type      = mjolnir::System<traits_type>;
    using potential_type   = mjolnir::HarmonicPotential<real_type>;
    using interaction_type = mjolnir::PositionRestraintInteraction<traits_type, potential_type>;

    const potential_type pot1{/*k =*/1.0, /*r0 =*/ 0.0};
    const potential_type pot2{/*k =*/1.0, /*r0 =*/10.0};

    interaction_type interaction(
        std::vector<std::tuple<std::size_t, coordinate_type, potential_type>>{
            std::make_tuple(0, coordinate_type{0.0,0.0,0.0}, pot1),
            std::make_tuple(1, coordinate_type{0.0,0.0,0.0}, pot2)
        });


    std::mt19937 rng(123456789);
    std::uniform_real_distribution<real_type> uni(-1.0, 1.0);

    for(std::size_t trial = 0; trial < 1000; ++trial)
    {
        system_type sys(2, boundary_type{});

        sys.at(0).mass  = 1.0;
        sys.at(1).mass  = 1.0;
        sys.at(0).rmass = 1.0;
        sys.at(1).rmass = 1.0;

        sys.at(0).position = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(1).position = coordinate_type(10.0,  0.0,  0.0);
        sys.at(0).velocity = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(1).velocity = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(0).force    = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(1).force    = coordinate_type( 0.0,  0.0,  0.0);

        sys.at(0).name  = "X";
        sys.at(1).name  = "X";
        sys.at(0).group = "NONE";
        sys.at(1).group = "NONE";

        sys.position(0) += coordinate_type(uni(rng), uni(rng), uni(rng));
        sys.position(1) += coordinate_type(uni(rng), uni(rng), uni(rng));

        const auto init = sys;
        interaction.initialize(init);

        constexpr real_type tol = 1e-3;
        constexpr real_type dr  = 1e-4;
        for(std::size_t idx=0; idx<sys.size(); ++idx)
        {
            {
                // ----------------------------------------------------------------
                // reset positions
                sys = init;

                // calc U(x-dx)
                const auto E0 = interaction.calc_energy(sys);

                mjolnir::math::X(sys.position(idx)) += dr;

                // calc F(x)
                interaction.calc_force(sys);

                mjolnir::math::X(sys.position(idx)) += dr;

                // calc U(x+dx)
                const auto E1 = interaction.calc_energy(sys);

                // central difference
                const auto dE = (E1 - E0) * 0.5;

                BOOST_TEST(-dE == dr * mjolnir::math::X(sys.force(idx)),
                           boost::test_tools::tolerance(tol));
            }
            {
                // ----------------------------------------------------------------
                // reset positions
                sys = init;

                // calc U(x-dx)
                const auto E0 = interaction.calc_energy(sys);

                mjolnir::math::Y(sys.position(idx)) += dr;

                // calc F(x)
                interaction.calc_force(sys);

                mjolnir::math::Y(sys.position(idx)) += dr;

                // calc U(x+dx)
                const auto E1 = interaction.calc_energy(sys);

                // central difference
                const auto dE = (E1 - E0) * 0.5;

                BOOST_TEST(-dE == dr * mjolnir::math::Y(sys.force(idx)),
                           boost::test_tools::tolerance(tol));
            }
            {
                // ----------------------------------------------------------------
                // reset positions
                sys = init;

                // calc U(x-dx)
                const auto E0 = interaction.calc_energy(sys);

                mjolnir::math::Z(sys.position(idx)) += dr;

                // calc F(x)
                interaction.calc_force(sys);

                mjolnir::math::Z(sys.position(idx)) += dr;

                // calc U(x+dx)
                const auto E1 = interaction.calc_energy(sys);

                // central difference
                const auto dE = (E1 - E0) * 0.5;

                BOOST_TEST(-dE == dr * mjolnir::math::Z(sys.force(idx)),
                           boost::test_tools::tolerance(tol));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(PositionRestraint_force_and_energy)
{
    using traits_type      = mjolnir::SimulatorTraits<double, mjolnir::UnlimitedBoundary>;
    using real_type        = traits_type::real_type;
    using coordinate_type  = traits_type::coordinate_type;
    using boundary_type    = traits_type::boundary_type;
    using system_type      = mjolnir::System<traits_type>;
    using potential_type   = mjolnir::HarmonicPotential<real_type>;
    using interaction_type = mjolnir::PositionRestraintInteraction<traits_type, potential_type>;

    const potential_type pot1{/*k =*/1.0, /*r0 =*/ 0.0};
    const potential_type pot2{/*k =*/1.0, /*r0 =*/10.0};

    interaction_type interaction(
        std::vector<std::tuple<std::size_t, coordinate_type, potential_type>>{
            std::make_tuple(0, coordinate_type{0.0,0.0,0.0}, pot1),
            std::make_tuple(1, coordinate_type{0.0,0.0,0.0}, pot2)
        });


    std::mt19937 rng(123456789);
    std::uniform_real_distribution<real_type> uni(-1.0, 1.0);

    for(std::size_t trial = 0; trial < 1000; ++trial)
    {
        system_type sys(2, boundary_type{});

        sys.at(0).mass  = 1.0;
        sys.at(1).mass  = 1.0;
        sys.at(0).rmass = 1.0;
        sys.at(1).rmass = 1.0;

        sys.at(0).position = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(1).position = coordinate_type(10.0,  0.0,  0.0);
        sys.at(0).velocity = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(1).velocity = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(0).force    = coordinate_type( 0.0,  0.0,  0.0);
        sys.at(1).force    = coordinate_type( 0.0,  0.0,  0.0);

        sys.at(0).name  = "X";
        sys.at(1).name  = "X";
        sys.at(0).group = "NONE";
        sys.at(1).group = "NONE";

        sys.position(0) += coordinate_type(uni(rng), uni(rng), uni(rng));
        sys.position(1) += coordinate_type(uni(rng), uni(rng), uni(rng));

        interaction.initialize(sys);
        system_type ref_sys = sys;

        constexpr real_type tol = 1e-3;

        const auto energy = interaction.calc_force_and_energy(sys);
        const auto ref_energy = interaction.calc_energy(ref_sys);
        interaction.calc_force(ref_sys);
        BOOST_TEST(ref_energy == energy, boost::test_tools::tolerance(tol));

        for(std::size_t idx=0; idx<sys.size(); ++idx)
        {
            BOOST_TEST(mjolnir::math::X(sys.force(idx)) == mjolnir::math::X(ref_sys.force(idx)), boost::test_tools::tolerance(tol));
            BOOST_TEST(mjolnir::math::Y(sys.force(idx)) == mjolnir::math::Y(ref_sys.force(idx)), boost::test_tools::tolerance(tol));
            BOOST_TEST(mjolnir::math::Z(sys.force(idx)) == mjolnir::math::Z(ref_sys.force(idx)), boost::test_tools::tolerance(tol));
        }
    }
}
