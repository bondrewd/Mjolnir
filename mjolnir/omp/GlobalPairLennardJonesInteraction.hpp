#ifndef MJOLNIR_OMP_GLOBAL_PAIR_LENNARD_JONES_INTEARACTION_HPP
#define MJOLNIR_OMP_GLOBAL_PAIR_LENNARD_JONES_INTEARACTION_HPP
#include <mjolnir/omp/OpenMPSimulatorTraits.hpp>
#include <mjolnir/omp/System.hpp>
#include <mjolnir/forcefield/global/GlobalPairLennardJonesInteraction.hpp>

namespace mjolnir
{

// specialization for GlobalPair<LennardJones>
template<typename realT, template<typename, typename> class boundaryT>
class GlobalPairInteraction<
    OpenMPSimulatorTraits<realT, boundaryT>,
    LennardJonesPotential<OpenMPSimulatorTraits<realT, boundaryT>>
    > final : public GlobalInteractionBase<OpenMPSimulatorTraits<realT, boundaryT>>
{
  public:

    using traits_type     = OpenMPSimulatorTraits<realT, boundaryT>;
    using base_type       = GlobalInteractionBase<traits_type>;
    using real_type       = typename base_type::real_type;
    using coordinate_type = typename base_type::coordinate_type;
    using system_type     = typename base_type::system_type;
    using topology_type   = typename base_type::topology_type;
    using boundary_type   = typename base_type::boundary_type;
    using potential_type  = LennardJonesPotential<traits_type>;
    using partition_type  = SpatialPartition<traits_type, potential_type>;

  public:
    GlobalPairInteraction()  = default;
    ~GlobalPairInteraction() override {}

    GlobalPairInteraction(potential_type&& pot, partition_type&& part)
        : potential_(std::move(pot)), partition_(std::move(part))
    {}

    /*! @brief initialize spatial partition (e.g. CellList)                   *
     *  @details before calling `calc_(force|energy)`, this should be called. */
    void initialize(const system_type& sys, const topology_type& topol) override
    {
        MJOLNIR_GET_DEFAULT_LOGGER();
        MJOLNIR_LOG_FUNCTION();
        MJOLNIR_LOG_INFO("potential is ", this->name());
        this->potential_.initialize(sys, topol);
        this->partition_.initialize(sys, this->potential_);
    }

    /*! @brief update parameters (e.g. temperature, ionic strength, ...)  *
     *  @details A method that change system parameters (e.g. Annealing), *
     *           the method is bound to call this function after changing *
     *           parameters.                                              */
    void update(const system_type& sys, const topology_type& topol) override
    {
        MJOLNIR_GET_DEFAULT_LOGGER();
        MJOLNIR_LOG_FUNCTION();
        MJOLNIR_LOG_INFO("potential is ", this->name());
        this->potential_.update(sys, topol);
        // potential update may change the cutoff length!
        this->partition_.initialize(sys, this->potential_);
    }

    void reduce_margin(const real_type dmargin, const system_type& sys) override
    {
        this->partition_.reduce_margin(dmargin, sys, this->potential_);
        return;
    }
    void scale_margin(const real_type scale, const system_type& sys) override
    {
        this->partition_.scale_margin(scale, sys, this->potential_);
        return;
    }

    void calc_force(system_type& sys) const noexcept override
    {
        const auto  cutoff_ratio    = potential_.cutoff_ratio();
        const auto  cutoff_ratio_sq = cutoff_ratio * cutoff_ratio;

        const auto leading_participants = this->potential_.leading_participants();
#pragma omp parallel for
        for(std::size_t idx=0; idx < leading_participants.size(); ++idx)
        {
            const auto i = leading_participants[idx];
            for(const auto& ptnr : this->partition_.partners(i))
            {
                const auto  j     = ptnr.index;
                const auto& param = ptnr.parameter();

                const coordinate_type rij =
                    sys.adjust_direction(sys.position(i), sys.position(j));
                const real_type l_sq = math::length_sq(rij);

                const real_type sigma_sq = param.first * param.first;
                if(sigma_sq * cutoff_ratio_sq < l_sq) {continue;}

                const real_type epsilon = param.second;

                const real_type rcp_l_sq = 1 / l_sq;
                const real_type s2l2 = sigma_sq * rcp_l_sq;
                const real_type s6l6 = s2l2 * s2l2 * s2l2;

                const coordinate_type f = rij *
                    (24 * epsilon * (s6l6 - 2 * s6l6 * s6l6) * rcp_l_sq);

                const std::size_t thread_id = omp_get_thread_num();
                sys.force_thread(thread_id, i) += f;
                sys.force_thread(thread_id, j) -= f;

                sys.virial_thread(thread_id) += math::tensor_product(rij, -f);
            }
        }
        return ;
    }

    real_type calc_energy(const system_type& sys) const noexcept override
    {
        real_type E(0);

        const auto  cutoff_ratio    = potential_.cutoff_ratio();
        const auto  cutoff_ratio_sq = cutoff_ratio * cutoff_ratio;
        const auto  coef_at_cutoff  = potential_.coef_at_cutoff();

        const auto leading_participants = this->potential_.leading_participants();
#pragma omp parallel for reduction(+:E)
        for(std::size_t idx=0; idx < leading_participants.size(); ++idx)
        {
            const auto i = leading_participants[idx];
            for(const auto& ptnr : this->partition_.partners(i))
            {
                const auto  j     = ptnr.index;
                const auto& param = ptnr.parameter();

                const coordinate_type rij =
                    sys.adjust_direction(sys.position(i), sys.position(j));
                const real_type l_sq = math::length_sq(rij);

                const real_type sigma_sq = param.first * param.first;
                if(sigma_sq * cutoff_ratio_sq < l_sq) {continue;}

                const real_type epsilon = param.second;

                const real_type s2l2 = sigma_sq / l_sq;
                const real_type s6l6 = s2l2 * s2l2 * s2l2;

                E += 4 * epsilon * (s6l6 * s6l6 - s6l6 - coef_at_cutoff);
            }
        }
        return E;
    }

    real_type calc_force_and_energy(system_type& sys) const noexcept override
    {
        const auto cutoff_ratio    = potential_.cutoff_ratio();
        const auto cutoff_ratio_sq = cutoff_ratio * cutoff_ratio;
        const auto coef_at_cutoff  = potential_.coef_at_cutoff();

        real_type energy = 0;
        const auto leading_participants = this->potential_.leading_participants();
#pragma omp parallel for reduction(+:energy)
        for(std::size_t idx=0; idx < leading_participants.size(); ++idx)
        {
            const auto i = leading_participants[idx];
            for(const auto& ptnr : this->partition_.partners(i))
            {
                const auto  j     = ptnr.index;
                const auto& param = ptnr.parameter();

                const coordinate_type rij =
                    sys.adjust_direction(sys.position(i), sys.position(j));
                const real_type l_sq = math::length_sq(rij);

                const real_type sigma_sq = param.first * param.first;
                if(sigma_sq * cutoff_ratio_sq < l_sq) {continue;}

                const real_type epsilon = param.second;

                const real_type rcp_l_sq = 1 / l_sq;
                const real_type s2l2 = sigma_sq * rcp_l_sq;
                const real_type s6l6 = s2l2 * s2l2 * s2l2;

                energy += 4 * epsilon * (s6l6 * s6l6 - s6l6 - coef_at_cutoff);

                const coordinate_type f = rij *
                    (24 * epsilon * (s6l6 - 2 * s6l6 * s6l6) * rcp_l_sq);

                const std::size_t thread_id = omp_get_thread_num();
                sys.force_thread(thread_id, i) += f;
                sys.force_thread(thread_id, j) -= f;

                sys.virial_thread(thread_id) += math::tensor_product(rij, -f);
            }
        }
        return energy;
    }


    std::string name() const override {return "GlobalPairLennardJones";}

    base_type* clone() const override
    {
        return new GlobalPairInteraction(
                potential_type(potential_), partition_type(partition_));
    }

  private:

    potential_type potential_;
    partition_type partition_;
};

} // mjolnir

#ifdef MJOLNIR_SEPARATE_BUILD
#include <mjolnir/core/BoundaryCondition.hpp>

namespace mjolnir
{
extern template class GlobalPairInteraction<OpenMPSimulatorTraits<double, UnlimitedBoundary>       , LennardJonesPotential<OpenMPSimulatorTraits<double, UnlimitedBoundary>       >>;
extern template class GlobalPairInteraction<OpenMPSimulatorTraits<float,  UnlimitedBoundary>       , LennardJonesPotential<OpenMPSimulatorTraits<float,  UnlimitedBoundary>       >>;
extern template class GlobalPairInteraction<OpenMPSimulatorTraits<double, CuboidalPeriodicBoundary>, LennardJonesPotential<OpenMPSimulatorTraits<double, CuboidalPeriodicBoundary>>>;
extern template class GlobalPairInteraction<OpenMPSimulatorTraits<float,  CuboidalPeriodicBoundary>, LennardJonesPotential<OpenMPSimulatorTraits<float,  CuboidalPeriodicBoundary>>>;
} // mjolnir
#endif // MJOLNIR_SEPARATE_BUILD

#endif /* MJOLNIR_GLOBAL_PAIR_INTEARACTION */
