#ifndef MJOLNIR_GAUSSIAN_POTENTIAL
#define MJOLNIR_GAUSSIAN_POTENTIAL
#include <mjolnir/core/LocalPotentialBase.hpp>

namespace mjolnir
{

/*! @brief gaussian potential for AICG2+                     *
 * V(r)  = epsilon * exp(-(r-r0)^2 / 2W^2)                   *
 * dV/dr = epsilon * (-(r-r0) / W^2) * exp(-(r-r0)^2 / 2W^2) */
template<typename traitsT>
class GaussianPotential : public LocalPotentialBase<traitsT>
{
  public:
    typedef traitsT traits_type;
    typedef typename traits_type::real_type real_type;
    typedef typename traits_type::coordinate_type coordinate_type;

  public:
    GaussianPotential(const real_type e, const real_type w,
                      const real_type native_val)
        : epsilon_(e), inv_w2_(-1./(w*w)), native_val_(native_val)
    {}
    ~GaussianPotential() override = default;

    real_type potential(const real_type val) const override
    {
        const real_type dval = val - this->native_val_;
        return epsilon_ * std::exp(inv_w2 * dval * dval);
    }

    real_type derivative(const real_type val) const override
    {
        const real_type dval = val - this->native_val_;
        return inv_w2 * dval * epsilon_ * std::exp(inv_w2 * dval * dval);
    }

  private:

    const real_type epsilon_, inv_w2_;
    const real_type native_val_;
};

} // mjolnir
#endif /* MJOLNIR_GAUSSIAN_POTENTIAL */