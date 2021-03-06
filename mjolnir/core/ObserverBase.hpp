#ifndef MJOLNIR_CORE_OBSERVER_BASE_HPP
#define MJOLNIR_CORE_OBSERVER_BASE_HPP
#include <mjolnir/core/System.hpp>
#include <mjolnir/core/ForceField.hpp>
#include <mjolnir/util/binary_io.hpp>
#include <string>

namespace mjolnir
{

template<typename traitsT>
class ObserverBase
{
  public:
    using traits_type     = traitsT;
    using real_type       = typename traits_type::real_type;
    using coordinate_type = typename traits_type::coordinate_type;
    using system_type     = System<traits_type>;
    using forcefield_type = std::unique_ptr<ForceFieldBase<traits_type>>;

  public:
    ObserverBase() = default;
    virtual ~ObserverBase() {}

    // open files, write header and so on.
    virtual void initialize(const std::size_t total_step,
                            const std::size_t save_interval, const real_type dt,
                            const system_type&, const forcefield_type&) = 0;
    // call if system or forcefield is changed.
    virtual void update    (const std::size_t step,       const real_type dt,
                            const system_type&, const forcefield_type&) = 0;
    // output the current state.
    virtual void output    (const std::size_t step,       const real_type dt,
                            const system_type&, const forcefield_type&) = 0;
    // update header, or something that required to be finalized
    virtual void finalize  (const std::size_t total_step, const real_type dt,
                            const system_type&, const forcefield_type&) = 0;

    // for testing purpose.
    virtual std::string const& prefix() const noexcept = 0;
};

#ifdef MJOLNIR_SEPARATE_BUILD
extern template class ObserverBase<SimulatorTraits<double, UnlimitedBoundary>       >;
extern template class ObserverBase<SimulatorTraits<float,  UnlimitedBoundary>       >;
extern template class ObserverBase<SimulatorTraits<double, CuboidalPeriodicBoundary>>;
extern template class ObserverBase<SimulatorTraits<float,  CuboidalPeriodicBoundary>>;
#endif

} // mjolnir
#endif// MJOLNIR_CORE_OBSERVER_BASE_HPP
