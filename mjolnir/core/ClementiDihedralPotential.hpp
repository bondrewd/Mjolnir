#ifndef MJOLNIR_CLEMENTI_DIHEDRAL_POTENTIAL
#define MJOLNIR_CLEMENTI_DIHEDRAL_POTENTIAL
#include "LocalPotentialBase.hpp"
#include <cmath>

namespace mjolnir
{

template<typename traitsT>
class ClementiDihedralPotential : public LocalPotentialBase<traitsT>
{
  public:
    typedef traitsT traits_type;
    typedef typename traits_type::real_type real_type;
    typedef typename traits_type::coordinate_type coordinate_type;

  public:
    ClementiDihedralPotential(
            const real_type k1, const real_type k3, const real_type native_val)
        : k1_(k1), k3_(k3), native_val_(native_val)
    {}
    ~ClementiDihedralPotential() override = default;

    real_type potential(const real_type val) const override
    {
        const real_type dphi = val - native_val_;
        return k1_ * (1. - cos(dphi)) + k3_ * (1. - cos(3. * dphi));
    }

    real_type derivative(const real_type val) const override
    {
        const real_type dphi = val - native_val_;
        return k1_ * sin(dphi) + 3. * k3_ * sin(3. * dphi);
    }

  private:

    const real_type k1_;
    const real_type k3_;
    const real_type native_val_;
};

}


#endif /* MJOLNIR_CLEMENTI_DIHEDRAL_POTENTIAL */
