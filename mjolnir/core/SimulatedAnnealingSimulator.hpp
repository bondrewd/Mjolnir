#ifndef MJOLNIR_SIMULATED_ANNEALING_SIMULATOR_HPP
#define MJOLNIR_SIMULATED_ANNEALING_SIMULATOR_HPP
#include "SimulatorBase.hpp"
#include "System.hpp"
#include "ForceField.hpp"
#include "Observer.hpp"

namespace mjolnir
{

template<typename realT>
struct linear_schedule
{
    using real_type = realT;

    linear_schedule(const real_type first, const real_type last)
        : first_(first), last_(last)
    {}
    ~linear_schedule() = default;

    linear_schedule(const linear_schedule&) = default;
    linear_schedule(linear_schedule&&)      = default;
    linear_schedule& operator=(const linear_schedule&) = default;
    linear_schedule& operator=(linear_schedule&&)      = default;

    real_type current(const real_type ratio) const noexcept
    {
        assert(0.0 <= ratio && ratio <= 1.0);
        return this->first_ * (1.0 - ratio) + this->last_ * ratio;
    }

  private:
    real_type first_, last_;
};

// XXX : currently, it's not a `CORRECT` simulated annealing because it does not
//       affect to ForceField parameters like DebyeHuckel's debye length.
template<typename traitsT, typename integratorT,
         template<typename> class scheduleT>
class SimulatedAnnealingSimulator final : public SimulatorBase
{
  public:
    typedef traitsT     traits_type;
    typedef integratorT integrator_type;
    typedef System<traits_type>     system_type;
    typedef ForceField<traits_type> forcefield_type;
    typedef Observer<traits_type>   observer_type;
    typedef typename traits_type::real_type       real_type;
    typedef typename traits_type::coordinate_type coordinate_type;
    typedef scheduleT<real_type> scheduler_type;

    SimulatedAnnealingSimulator(
            const std::size_t tstep,   scheduler_type&& scheduler,
            system_type&&     sys,     forcefield_type&& ff,
            integrator_type&& integr,  observer_type&&   obs)
    : total_step_(tstep), step_count_(0), time_(0.), r_total_step_(1.0 / tstep),
      scheduler_(scheduler), system_(std::move(sys)), ff_(std::move(ff)),
      integrator_(std::move(integr)), observer_(std::move(obs))
    {}
    ~SimulatedAnnealingSimulator() override = default;

    void initialize() override;
    bool step()       override;
    void finalize()   override;

    real_type calc_energy() const {return this->ff_.calc_energy(this->system_);}

    system_type&       system()       noexcept {return system_;}
    system_type const& system() const noexcept {return system_;}

    ForceField<traitsT>&       forcefields()       noexcept {return ff_;}
    ForceField<traitsT> const& forcefields() const noexcept {return ff_;}

    real_type& time()       noexcept {return time_;}
    real_type  time() const noexcept {return time_;}

  protected:
    std::size_t     total_step_;
    std::size_t     step_count_;
    real_type       time_;
    real_type       r_total_step_;
    scheduler_type  scheduler_;
    system_type     system_;
    forcefield_type ff_;
    integrator_type integrator_;
    observer_type   observer_;
};

template<typename traitsT, typename integratorT,
         template<typename> class scheduleT>
inline void
SimulatedAnnealingSimulator<traitsT, integratorT, scheduleT>::initialize()
{
    this->ff_.initialize(this->system_, integrator_.delta_t());
    this->integrator_.initialize(this->system_, this->ff_);

    observer_.initialize(this->system_, this->ff_);
    observer_.output(0., this->system_, this->ff_);
    return;
}

template<typename traitsT, typename integratorT,
         template<typename> class scheduleT>
inline bool SimulatedAnnealingSimulator<traitsT, integratorT, scheduleT>::step()
{
    MJOLNIR_SET_DEFAULT_LOGGER();
    MJOLNIR_SCOPE(SimulatedAnnealingSimulator::step, 0);

    integrator_.step(this->time_, system_, ff_);
    ++step_count_;
    this->time_ = this->step_count_ * integrator_.delta_t();

    system_.attribute("temperature") =
        this->scheduler_.current(step_count_ * r_total_step_);

    MJOLNIR_LOG_DEBUG("T = ", system_.attribute("temperature"));

    this->integrator_.update(system_);

    if(observer_.is_output_time())
    {
        observer_.output(this->time_, this->system_, this->ff_);
    }
    return step_count_ < total_step_;
}

template<typename traitsT, typename integratorT,
         template<typename> class scheduleT>
inline void
SimulatedAnnealingSimulator<traitsT, integratorT, scheduleT>::finalize()
{
    return;
}

} // mjolnir
#endif // MJOLNIR_SIMULATED_ANNEALING_SIMULATOR_HPP
