#ifndef MJOLNIR_COSINE_POTENTIAL
#define MJOLNIR_COSINE_POTENTIAL
#include <cmath>

namespace mjolnir
{

/*! @brief cosine-potential for 3SPN.1 dihedral potential *
 *  V(phi) = k * (1 - cos(phi - phi0))                    *
 * dV(phi) = k * sin(phi - phi0))                         */
template<typename traitsT>
class CosinePotential
{
  public:
    typedef traitsT traits_type;
    typedef typename traits_type::real_type real_type;
    typedef typename traits_type::coordinate_type coordinate_type;

  public:
    CosinePotential(const real_type k, const real_type phi0)
        : k_(k), phi0_(phi0)
    {}
    ~CosinePotential() = default;

    real_type potential(const real_type phi) const
    {
        return this->k_ *(1 - std::cos(phi - phi0_));
    }

    real_type derivative(const real_type phi) const
    {
        return this->k_ * std::sin(phi - phi0_);
    }

  private:

    const real_type k_;
    const real_type phi0_;
};

} // mjolnir
#endif /* MJOLNIR_COSINE_POTENTIAL */