#define BOOST_TEST_MODULE "test_debye_huckel_potential"

#ifdef UNITTEST_FRAMEWORK_LIBRARY_EXIST
#include <boost/test/unit_test.hpp>
#else
#define BOOST_TEST_NO_LIB
#include <boost/test/included/unit_test.hpp>
#endif

#include <test/mjolnir/traits.hpp>
#include <mjolnir/potential/global/DebyeHuckelPotential.hpp>
#include <mjolnir/util/make_unique.hpp>

BOOST_AUTO_TEST_CASE(DH_double)
{
    using real_type = double;
    constexpr static std::size_t       N = 10000;
    constexpr static real_type h = 1e-6;

    const real_type charge = 1.0;
    mjolnir::DebyeHuckelPotential<real_type, mjolnir::IgnoreNothing> dh({charge, charge}, {});

    const real_type x_min = 0.5 * dh.debye_length();
    const real_type x_max = dh.max_cutoff_length();
    const real_type dx = (x_max - x_min) / N;

    real_type x = x_min;
    for(std::size_t i=0; i<N; ++i)
    {
        const real_type pot1 = dh.potential(0, 1, x + h);
        const real_type pot2 = dh.potential(0, 1, x - h);
        const real_type dpot = (pot1 - pot2) / (2 * h);
        const real_type deri = dh.derivative(0, 1, x);

        if(std::abs(deri) > h)
        {
            BOOST_CHECK_CLOSE_FRACTION(dpot, deri, h);
        }
        else
        {
            BOOST_CHECK_SMALL(deri, h);
        }
        x += dx;
    }
}

BOOST_AUTO_TEST_CASE(DH_float)
{
    using real_type = float;
    constexpr static std::size_t       N    = 1000;
    constexpr static real_type h    = 1e-2;

    const real_type charge = 1.0;
    mjolnir::DebyeHuckelPotential<real_type, mjolnir::IgnoreNothing> dh({charge, charge}, {});

    const real_type x_min = 0.5 * dh.debye_length();
    const real_type x_max = dh.max_cutoff_length();
    const real_type dx = (x_max - x_min) / N;

    real_type x = x_min;
    for(std::size_t i=0; i<N; ++i)
    {
        const real_type pot1 = dh.potential(0, 1, x + h);
        const real_type pot2 = dh.potential(0, 1, x - h);
        const real_type dpot = (pot1 - pot2) / (2 * h);
        const real_type deri = dh.derivative(0, 1, x);

        if(std::abs(deri) > h)
        {
            BOOST_CHECK_CLOSE_FRACTION(dpot, deri, h);
        }
        else
        {
            BOOST_CHECK_SMALL(deri, h);
        }

        x += dx;
    }
}

