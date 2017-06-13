#define BOOST_TEST_MODULE "test_excluded_volume_potential"

#ifdef UNITTEST_FRAMEWORK_LIBRARY_EXIST
#include <boost/test/unit_test.hpp>
#else
#define BOOST_TEST_NO_LIB
#include <boost/test/included/unit_test.hpp>
#endif

#include <mjolnir/potential/ImplicitMembranePotential.hpp>
#include <mjolnir/core/BoundaryCondition.hpp>
#include <mjolnir/core/SimulatorTraits.hpp>
#include <mjolnir/util/make_unique.hpp>

#include <fstream>

BOOST_AUTO_TEST_CASE(IM_double)
{
    typedef mjolnir::SimulatorTraitsBase<double, mjolnir::UnlimitedBoundary> traits;
    constexpr static std::size_t       N = 10000;
    constexpr static traits::real_type h = 1e-6;

    mjolnir::ImplicitMembranePotential<traits> im;

    const traits::real_type thickness = 10.0;
    const traits::real_type interaction_magnitude = 1.0;
    const traits::real_type bend = 1.5;
    const std::vector<traits::real_type> hydrophobicities{1., 0.};
    
    im.half_thick() = thickness * 0.5;
    im.interaction_magnitude() = interaction_magnitude;
    im.bend() = bend;
    im.set_hydrophobicities(hydrophobicities);
    
    const traits::real_type cutoff_length = im.max_cutoff_length();
    
    const traits::real_type z_min = -1 * cutoff_length;
    const traits::real_type z_max = cutoff_length;
    const traits::real_type dz = (z_max - z_min) / N;

    std::ofstream ofs("testlog.dat");
    
    traits::real_type z = z_min;
    for(std::size_t i = 0; i < N; ++i)
    {
	const traits::real_type pot1 = im.potential(0, z + h);
	const traits::real_type pot2 = im.potential(0, z - h);
	const traits::real_type dpot = (pot1 - pot2) / (2 * h);
	const traits::real_type deri = im.derivative(0, z);

	ofs << z << " " << dz << " " << dpot << " " << deri << std::endl;
	
	if(std::abs(z) < h)
	    continue;
	else if(std::abs(deri) > h)
	    BOOST_CHECK_CLOSE_FRACTION(dpot, deri, h);
	else
	    BOOST_CHECK_SMALL(deri, h);

/*	const traits::real_type pot0 = im.potential(1, z);
	const traits::real_type deri0 = im.derivative(1, z);

	BOOST_CHECK_SMALL(pot0, h);
	BOOST_CHECK_SMALL(deri0, h);*/

	z += dz;
    }
    ofs.close();
}
/*
BOOST_AUTO_TEST_CASE(IM_float)
{
    typedef mjolnir::SimulatorTraitsBase<float, mjolnir::UnlimitedBoundary> traits;
    constexpr static std::size_t       N = 10000;
    constexpr static traits::real_type h = 1e-6;

    mjolnir::ImplicitMembranePotential<traits> im;

    const traits::real_type thickness = 10.0;
    const traits::real_type interaction_magnitude = 1.0;
    const traits::real_type bend = 1.5;
    const traits::real_type cutoff_length = im.max_cutoff_length();
    const std::vector<traits::real_type> hydrophobicities{1., 0.};
    
    im.thickness() = thickness;
    im.interaction_magnitude() = interaction_magnitude;
    im.bend() = bend;

    im.set_hydrophobicities(hydrophobicities);
    
    const traits::real_type z_min = -1 * cutoff_length;
    const traits::real_type z_max = cutoff_length;
    const traits::real_type dz = (z_max - z_min) / N;

    traits::real_type z = z_min;
    for(std::size_t i = 0; i < N; ++i)
    {
	const traits::real_type pot1 = im.potential(0, z);
	const traits::real_type pot2 = im.potential(0, z);
	const traits::real_type dpot = (pot1 - pot2) / (2 * h);
	const traits::real_type deri = im.derivative(0, z);

	if(std::abs(deri) > h)
	    BOOST_CHECK_CLOSE_FRACTION(dpot, deri, h);
	else
	    BOOST_CHECK_SMALL(deri, h);

	const traits::real_type pot0 = im.potential(1, z);
	const traits::real_type deri0 = im.derivative(1, z);

	BOOST_CHECK_SMALL(pot0, h);
	BOOST_CHECK_SMALL(deri0, h);

	z += dz;
    }
}

*/
