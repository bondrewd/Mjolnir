#ifndef MJOLNIR_INPUT_READ_GLOBAL_INTERACTION_HPP
#define MJOLNIR_INPUT_READ_GLOBAL_INTERACTION_HPP
#include <extlib/toml/toml.hpp>
#include <mjolnir/interaction/global/GlobalPairInteraction.hpp>
#include <mjolnir/interaction/global/GlobalPairLennardJonesInteraction.hpp>
#include <mjolnir/interaction/global/GlobalPairUniformLennardJonesInteraction.hpp>
#include <mjolnir/interaction/global/GlobalPairExcludedVolumeInteraction.hpp>
#include <mjolnir/interaction/global/ThreeSPN2BaseBaseInteraction.hpp>
#include <mjolnir/util/make_unique.hpp>
#include <mjolnir/util/throw_exception.hpp>
#include <mjolnir/util/logger.hpp>
#include <mjolnir/input/read_global_potential.hpp>
#include <mjolnir/input/read_spatial_partition.hpp>
#include <memory>

namespace mjolnir
{

// ----------------------------------------------------------------------------
// global interaction
// ----------------------------------------------------------------------------

template<typename traitsT>
std::unique_ptr<GlobalInteractionBase<traitsT>>
read_global_pair_interaction(const toml::value& global)
{
    MJOLNIR_GET_DEFAULT_LOGGER();
    MJOLNIR_LOG_FUNCTION();
    using real_type = typename traitsT::real_type;

    const auto potential = toml::find<std::string>(global, "potential");

    if(potential == "ExcludedVolume")
    {
        MJOLNIR_LOG_NOTICE("-- potential function is Excluded Volume.");
        using potential_t = ExcludedVolumePotential<real_type>;

        return read_spatial_partition<traitsT, potential_t>(
            global, read_excluded_volume_potential<real_type>(global));
    }
    else if(potential == "DebyeHuckel")
    {
        MJOLNIR_LOG_NOTICE("-- potential function is Debye-Huckel.");
        using potential_t = DebyeHuckelPotential<real_type>;

        return read_spatial_partition<traitsT, potential_t>(
            global, read_debye_huckel_potential<real_type>(global));
    }
    else if(potential == "LennardJones")
    {
        MJOLNIR_LOG_NOTICE("-- potential function is Lennard-Jones.");
        using potential_t = LennardJonesPotential<real_type>;

        return read_spatial_partition<traitsT, potential_t>(
            global, read_lennard_jones_potential<real_type>(global));
    }
    else if(potential == "UniformLennardJones")
    {
        MJOLNIR_LOG_NOTICE("-- potential function is Uniform Lennard-Jones.");
        using potential_t = UniformLennardJonesPotential<real_type>;

        return read_spatial_partition<traitsT, potential_t>(
            global, read_uniform_lennard_jones_potential<real_type>(global));
    }
    else if(potential == "3SPN2ExcludedVolume")
    {
        MJOLNIR_LOG_NOTICE("-- potential function is 3SPN2ExcludedVolume.");
        using potential_t = ThreeSPN2ExcludedVolumePotential<real_type>;

        return read_spatial_partition<traitsT, potential_t>(
            global, read_3spn2_excluded_volume_potential<real_type>(global));
    }
    else
    {
        throw_exception<std::runtime_error>(toml::format_error("[error] "
            "mjolnir::read_global_pair_interaction: invalid potential",
            toml::find<toml::value>(global, "potential"), "here", {
            "expected value is one of the following.",
            "- \"ExcludedVolume\"       : repulsive r^12 potential",
            "- \"DebyeHuckel\"          : Debye-Huckel type electrostatic potential",
            "- \"LennardJones\"         : famous r^12 - r^6 potential",
            "- \"UniformLennardJones\"  : famous r^12 - r^6 potential with uniform parameters",
            "- \"3SPN2ExcludedVolume\"  : excluded volume for 3SPN2 DNA model"
            }));
    }
}

// ----------------------------------------------------------------------------
// 3SPN2 Base-Base Interaction

template<typename traitsT>
std::unique_ptr<GlobalInteractionBase<traitsT>>
read_global_3spn2_base_base_interaction(const toml::value& global)
{
    MJOLNIR_GET_DEFAULT_LOGGER();
    MJOLNIR_LOG_FUNCTION();
    using real_type           = typename traitsT::real_type;
    using base_kind           = parameter_3SPN2::base_kind;
    using potential_type      = ThreeSPN2BaseBaseInteractionPotential<real_type>;
    using parameter_type      = typename potential_type::parameter_type;
    using pair_parameter_type = typename potential_type::pair_parameter_type;

    // [[forcefields.global]]
    // interaction = "3SPNBaseBase"
    // spatial_partition = {type = "CellList", margin = 1.0}
    // parameters = [
    // {nucleotide_index = 0, S = 0, B = 1, base = "A", B5 = "none", B3 = 4},
    // # ...
    // ]

    // ------------------------------------------------------------------------
    // read parameters

    const auto& env    = global.as_table().count("env") == 1 ?
                         global.as_table().at("env") : toml::value{};

    const auto& ps = toml::find<toml::array>(global, "parameters");
    MJOLNIR_LOG_INFO(ps.size(), " parameters are found");

    std::vector<std::pair<std::size_t, parameter_type>> params;
    params.reserve(ps.size());
    for(const auto& param : ps)
    {
        const auto B    = find_parameter<std::size_t>(param, env, "B");
        const auto base = find_parameter<std::string>(param, env, "base");
        if(base != "A" && base != "T" && base != "G" && base != "C")
        {
            throw_exception<std::runtime_error>(toml::format_error("[error] "
                "mjolnir::read_global_3spn2_base_base_interaction: invalid base",
                find_parameter<toml::value>(param, env, "base"),
                "expected A, T, C, G"));
        }
        parameter_type p;
        p.S_idx        = find_parameter<std::size_t>(param, env, "S");
        p.strand_index = find_parameter<std::size_t>(param, env, "nucleotide_index");
        switch(base.front())
        {
            case 'A': p.base = base_kind::A; break;
            case 'T': p.base = base_kind::T; break;
            case 'G': p.base = base_kind::G; break;
            case 'C': p.base = base_kind::C; break;
            default: assert(false);
        }
        const auto B5 = toml::find(param, "B3");
        const auto B3 = toml::find(param, "B5");
        if(B5.is_string() && B5.as_string(std::nothrow) == "none")
        {
            p.B5_idx = potential_type::invalid();
        }
        else
        {
            p.B5_idx = toml::get<std::size_t>(B5);
        }
        if(B3.is_string() && B3.as_string(std::nothrow) == "none")
        {
            p.B3_idx = potential_type::invalid();
        }
        else
        {
            p.B3_idx = toml::get<std::size_t>(B3);
        }
        MJOLNIR_LOG_INFO("Base idx = ", B, ", base = ", base, ", Sugar idx = ",
            p.S_idx, ", 5' adjacent = ", p.B5_idx, ", 3' adjacent", p.B3_idx);

        params.emplace_back(B, p);
    }

    const auto& ignore = toml::find<toml::value>(global, "ignore");
    potential_type potential(std::move(params), read_ignored_group(ignore));

    // -----------------------------------------------------------------------
    // read spatial partition

    const auto& sp   = toml::find(global, "spatial_partition");
    const auto  type = toml::find<std::string>(sp, "type");
    if(type == "CellList")
    {
        using boundary_type = typename traitsT::boundary_type;
        using dispatcher    = celllist_dispatcher<boundary_type, traitsT,
                                                  pair_parameter_type>;
        using celllist_type = typename dispatcher::type;

        const auto margin = toml::find<real_type>(sp, "margin");
        MJOLNIR_LOG_NOTICE("-- Spatial Partition is CellList "
                           "with relative margin = ", margin);
        return make_unique<
            ThreeSPN2BaseBaseInteraction<traitsT, celllist_type>
            >(std::move(potential), dispatcher::invoke(margin));
    }
    else if(type == "VerletList")
    {
        using verlet_list_type = VerletList<traitsT, pair_parameter_type>;

        const auto margin = toml::find<real_type>(sp, "margin");
        MJOLNIR_LOG_NOTICE("-- Spatial Partition is VerletList "
                           "with relative margin = ", margin);
        return make_unique<
            ThreeSPN2BaseBaseInteraction<traitsT, verlet_list_type>
            >(std::move(potential), verlet_list_type(margin));
    }
    else if(type == "Naive" || type == "Nothing")
    {
        MJOLNIR_LOG_NOTICE("-- No Spatial Partition. Calculate all the possible pairs.");
        using naive_pair_type = NaivePairCalculation<traitsT, pair_parameter_type>;

        return make_unique<
            ThreeSPN2BaseBaseInteraction<traitsT, naive_pair_type>
            >(std::move(potential), naive_pair_type());
    }
    else
    {
        throw std::runtime_error(toml::format_error("[error] "
            "mjolnir::read_global_3spn2_base_base_interaction: unknown option",
            toml::find(sp, "type"), "expected \"CellList\" or \"VerletList\""));
    }
}

// ----------------------------------------------------------------------------
// general read_global_interaction function
// ----------------------------------------------------------------------------

template<typename traitsT>
std::unique_ptr<GlobalInteractionBase<traitsT>>
read_global_interaction(const toml::value& global)
{
    MJOLNIR_GET_DEFAULT_LOGGER();
    MJOLNIR_LOG_FUNCTION();
    const auto interaction = toml::find<std::string>(global, "interaction");

    if(interaction == "Pair")
    {
        MJOLNIR_LOG_NOTICE("Pair interaction found.");
        return read_global_pair_interaction<traitsT>(global);
    }
    if(interaction == "3SPN2BaseBase")
    {
        MJOLNIR_LOG_NOTICE("3SPN2BaseBaseInteraction found.");
        return read_global_3spn2_base_base_interaction<traitsT>(global);
    }
    else
    {
        throw std::runtime_error(toml::format_error("[error] "
            "mjolnir::read_global_interaction: invalid interaction",
            toml::find<toml::value>(global, "interaction"), "here", {
            "expected value is one of the following.",
            "- \"Pair\": well-known pair interaction depends only on the distance",
            "- \"3SPN2BaseBase\": Base-Base interaction for 3SPN2 DNA model"
            }));
    }
}

#ifdef MJOLNIR_SEPARATE_BUILD
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<double, UnlimitedBoundary>       >> read_global_interaction(const toml::value& global);
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<float,  UnlimitedBoundary>       >> read_global_interaction(const toml::value& global);
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<double, CuboidalPeriodicBoundary>>> read_global_interaction(const toml::value& global);
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<float,  CuboidalPeriodicBoundary>>> read_global_interaction(const toml::value& global);

extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<double, UnlimitedBoundary>       >> read_global_pair_interaction(const toml::value& global);
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<float,  UnlimitedBoundary>       >> read_global_pair_interaction(const toml::value& global);
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<double, CuboidalPeriodicBoundary>>> read_global_pair_interaction(const toml::value& global);
extern template std::unique_ptr<GlobalInteractionBase<SimulatorTraits<float,  CuboidalPeriodicBoundary>>> read_global_pair_interaction(const toml::value& global);
#endif

} // mjolnir
#endif// MJOLNIR_READ_GLOBAL_INTERACTION_HPP
